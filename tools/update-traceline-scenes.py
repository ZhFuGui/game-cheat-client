"""Admit an explicit reviewed DLL and complete cache inventory through its public ABI."""
import argparse
import ctypes as c
import hashlib
import json
from pathlib import Path
import re
import shutil


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def write(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')


class SceneInfo(c.Structure):
    _fields_ = [(name, c.c_uint32) for name in
                ('struct_size', 'flags', 'triangles', 'solids', 'closedSolids', 'nodes')] + [('fingerprint', c.c_uint64)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--delivery', type=Path, required=True)
    parser.add_argument('--inventory', type=Path, required=True)
    parser.add_argument('--include-dir', type=Path, required=True)
    parser.add_argument('--map-root', type=Path, required=True)
    parser.add_argument('--expected-dll-sha256', required=True)
    parser.add_argument('--backup', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    bundle = root/'data/games/cs2/cover'
    public = root/'third_party/traceline'
    stage = root/'out/traceline-native-update/staged-bundle'
    backup = args.backup.resolve()
    assert backup.is_relative_to(root/'.migration') and not backup.exists()
    assert bundle.resolve() == root/'data/games/cs2/cover' and not stage.exists()
    report = json.loads(args.delivery.read_text(encoding='utf-8-sig'))
    inventory = json.loads(args.inventory.read_text(encoding='utf-8-sig'))
    dll = Path(report['dll']['path'])
    expected = args.expected_dll_sha256.lower()
    assert digest(dll) == report['dll']['sha256'] == expected
    assert dll.stat().st_size == report['dll']['bytes'] and report['dll']['abi'] == 1
    assert Path(report['maps']['inventory']).resolve() == args.inventory.resolve()
    old = json.loads((bundle/'manifest.json').read_text(encoding='utf-8-sig'))
    lock = json.loads((public/'manifest.json').read_text(encoding='utf-8-sig'))
    assert digest(bundle/'manifest.json') == lock['catalogSha256']
    assert digest(bundle/old['library']['file']) == lock['librarySha256']
    profile = old['ballistics']
    assert digest(bundle/profile['file']) == profile['sha256']
    old_maps = {item['name']: item for item in old['maps']}
    assert len(inventory) == len({item['map'] for item in inventory}) == report['maps']['count']
    assert set(old_maps) == {item['map'] for item in inventory}
    assert sum(item['bytes'] for item in inventory) == report['maps']['bytes']
    map_root = args.map_root.resolve()
    for item in inventory:
        name = item['map']
        assert re.fullmatch('[a-z0-9_]+', name) and item['version'] == 4
        source = Path(item['cache']).resolve()
        assert source == map_root/name/'scene.cs2tl'
        assert source.stat().st_size == item['bytes'] and digest(source) == item['sha256']
    headers = [(args.include_dir/name, public/'include/cs2-external-autowall'/name)
               for name in ('api.h', 'types.h')]
    assert all(source.is_file() for source, _ in headers)
    api = c.CDLL(str(dll))
    api.cs2tl_get_abi_version.restype = c.c_uint32
    assert api.cs2tl_get_abi_version() == old['abi'] == lock['abi'] == 1
    api.cs2tl_load_scene.argtypes = [c.c_char_p, c.POINTER(c.c_void_p), c.c_void_p]
    api.cs2tl_get_scene_info.argtypes = [c.c_void_p, c.POINTER(SceneInfo), c.c_void_p]
    api.cs2tl_destroy_scene.argtypes = [c.c_void_p]
    maps = []
    for item in inventory:
        source = Path(item['cache'])
        relative = Path('maps')/item['map']/'scene.cs2tl'
        target = stage/relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
        assert digest(target) == item['sha256'], 'Cache changed while staging'
        scene = c.c_void_p()
        assert api.cs2tl_load_scene(str(target).encode(), c.byref(scene), None) == 0 and scene.value
        try:
            info = SceneInfo(c.sizeof(SceneInfo))
            assert api.cs2tl_get_scene_info(scene, c.byref(info), None) == 0
            assert not info.flags & 1 and info.closedSolids <= info.solids
            entry = dict(variant='intact', file=relative.as_posix(), bytes=item['bytes'], sha256=item['sha256'],
                         fingerprint=str(info.fingerprint), triangles=info.triangles, solids=info.solids,
                         closedSolids=info.closedSolids, nodes=info.nodes, flags=info.flags)
            maps.append(dict(name=item['map'], resource=old_maps[item['map']]['resource'], scenes=[entry]))
        finally:
            api.cs2tl_destroy_scene(scene)
        print(f"Verified {item['map']}", flush=True)
    library = stage/old['library']['file']
    library.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(dll, library)
    assert digest(library) == expected
    parameter_target = stage/profile['file']
    parameter_target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(bundle/profile['file'], parameter_target)
    assert digest(parameter_target) == profile['sha256']
    catalog = dict(schema=1, abi=1, scope='static-physics', breakables='intact',
                   library=dict(file=old['library']['file'], bytes=library.stat().st_size, sha256=expected),
                   ballistics=profile, maps=maps, upstreamUpdateManifestSha256=digest(args.delivery),
                   upstreamInventorySha256=digest(args.inventory))
    write(stage/'manifest.json', catalog)
    backup.mkdir(parents=True)
    shutil.copytree(public, backup/'public')
    shutil.copy2(args.delivery, backup/'new-delivery.json')
    shutil.copy2(args.inventory, backup/'new-inventory.json')
    # Only these previously checked workspace directories are moved. No deletion.
    bundle.rename(backup/'bundle')
    try:
        stage.rename(bundle)
        for source, target in headers:
            shutil.copy2(source, target)
        lock.update(librarySha256=expected, catalogSha256=digest(bundle/'manifest.json'),
                    upstreamCommit=report['commit'],
                    headers=[dict(file=target.relative_to(public).as_posix(), sha256=digest(target))
                             for _, target in headers])
        write(public/'manifest.json', lock)
    except Exception:
        if bundle.exists():
            bundle.rename(stage)
        (backup/'bundle').rename(bundle)
        for _, target in headers:
            shutil.copy2(backup/'public'/target.relative_to(public), target)
        shutil.copy2(backup/'public/manifest.json', public/'manifest.json')
        raise
    write(root/'out/traceline-native-update/staging.json',
          dict(staged=True, commit=report['commit'], librarySha256=expected, maps=len(maps),
               cacheBytes=sum(item['bytes'] for item in inventory), profileUnchanged=profile,
               backup=str(backup), originalSourcesCopied=False))


if __name__ == '__main__':
    main()

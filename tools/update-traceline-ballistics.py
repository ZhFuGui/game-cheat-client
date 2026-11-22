"""Stage reviewed DLL/profile or paired scene updates without reimporting maps."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def verified_file(entry):
    path = Path(entry['path'])
    assert path.is_file() and path.stat().st_size == entry['bytes']
    assert digest(path) == entry['sha256']
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--delivery', type=Path, required=True)
    parser.add_argument('--include-dir', type=Path, required=True)
    parser.add_argument('--expected-dll-sha256', required=True)
    parser.add_argument('--backup', type=Path, required=True)
    parser.add_argument('--game-build', type=int)
    parser.add_argument('--scene-map', help='Existing catalog map for an explicitly paired scene update')
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    bundle = root/'data/games/cs2/cover'
    public = root/'third_party/traceline'
    delivery = json.loads(args.delivery.read_text(encoding='utf-8-sig'))
    dll = verified_file(delivery['dll'])
    assert digest(dll) == args.expected_dll_sha256
    catalog = json.loads((bundle/'manifest.json').read_text(encoding='utf-8-sig'))
    lock = json.loads((public/'manifest.json').read_text(encoding='utf-8-sig'))
    assert digest(bundle/'manifest.json') == lock['catalogSha256']
    assert delivery.get('abi_version', catalog['abi']) == catalog['abi'] == lock['abi']
    profile_entry = delivery.get('profile_package')
    profile = verified_file(profile_entry) if profile_entry else bundle/catalog['ballistics']['file']
    if profile_entry:
        assert args.game_build and args.game_build > 0, 'A profile update requires the reviewed game build'
    else:
        # A scene patch cannot silently replace the already reviewed weapon data.
        assert delivery['parameters']['sha256'] == catalog['ballistics']['sha256'] == digest(profile)
        assert delivery['parameters']['bytes'] == profile.stat().st_size
    scene_entry = None
    scene = None
    if 'scene' in delivery:
        assert args.scene_map and delivery['consumer_must_replace_dll_and_scene_together']
        scene = verified_file(delivery['scene'])
        selected = [m for m in catalog['maps'] if m['name'] == args.scene_map]
        assert len(selected) == 1, 'Choose one existing map explicitly'
        scene_entry = next(s for s in selected[0]['scenes'] if s['variant'] == 'intact')
        assert int(scene_entry['fingerprint']) == delivery['scene_source_fingerprint']
    else:
        assert not args.scene_map, 'No paired scene was supplied'
    headers = [(args.include_dir/name, public/'include/cs2-external-autowall'/name)
               for name in ('api.h', 'types.h')]
    assert all(source.is_file() for source, _ in headers)
    assert not args.backup.exists(), 'Keep the previous reviewed bundle in a new backup'
    previous = [(bundle/'manifest.json', 'manifest.json'),
                (bundle/catalog['library']['file'], catalog['library']['file'])]
    if scene_entry:
        previous.append((bundle/scene_entry['file'], scene_entry['file']))
    if catalog.get('ballistics'):
        previous.append((bundle/catalog['ballistics']['file'], catalog['ballistics']['file']))
    for source, relative in previous:
        target = args.backup/relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
    shutil.copytree(public, args.backup/'public')
    shutil.copy2(args.delivery, args.backup/'new-delivery.json')
    shutil.copy2(dll, bundle/catalog['library']['file'])
    catalog['library'].update(sha256=digest(dll), bytes=dll.stat().st_size)
    if profile_entry:
        relative = f"profiles/build-{profile_entry['steam_build_id']}.cs2tlp"
        target = bundle/relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(profile, target)
        catalog['ballistics'] = dict(file=relative, bytes=profile.stat().st_size,
                                    sha256=digest(profile), gameBuild=args.game_build)
    if scene_entry:
        shutil.copy2(scene, bundle/scene_entry['file'])
        scene_entry.update(bytes=scene.stat().st_size, sha256=digest(scene))
    catalog['upstreamUpdateManifestSha256'] = digest(args.delivery)
    lock['headers'] = []
    for source, target in headers:
        if source.read_bytes() != target.read_bytes():
            shutil.copy2(source, target)
        assert digest(source) == digest(target)
        lock['headers'].append(dict(file=target.relative_to(public).as_posix(), sha256=digest(target)))
    (bundle/'manifest.json').write_text(json.dumps(catalog, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    lock.update(librarySha256=digest(dll), catalogSha256=digest(bundle/'manifest.json'))
    if delivery.get('commit'):
        lock['upstreamCommit'] = delivery['commit']
    (public/'manifest.json').write_text(json.dumps(lock, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(staged=True, collisionMapsChanged=bool(scene_entry), sceneMap=args.scene_map,
                          dll=digest(dll), profile=digest(profile))))


if __name__ == '__main__':
    main()

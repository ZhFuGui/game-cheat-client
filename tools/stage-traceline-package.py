"""Pin public C headers, an explicit DLL and collision caches only. No source build."""
import argparse
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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dll', type=Path, required=True)
    parser.add_argument('--include-dir', type=Path, required=True)
    parser.add_argument('--map-root', type=Path, required=True)
    parser.add_argument('--expected-dll-sha256', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    target = root/'data/games/cs2/cover'
    public = root/'third_party/traceline'
    assert not target.exists(), 'Use a fresh destination; keep the previous version for rollback'
    assert digest(args.dll) == args.expected_dll_sha256.lower(), 'DLL changed from reviewed artifact'
    upstream_path = args.map_root/'manifest.json'
    upstream = json.loads(upstream_path.read_text(encoding='utf-8-sig'))
    maps = []; expected_files = set()
    for item in upstream['exports']:
        name = item['map']
        assert re.fullmatch('[a-z0-9_]+', name), name
        assert item['status'] == item['cache_status'] == 'ok', name
        scenes = []
        for key, variant in [('cache_result', 'intact'), ('cache_without_breakables', 'ignore-breakables')]:
            if key not in item:
                continue
            entry = item[key]; filename = entry['file']; info = entry['scene']
            assert entry['status'] == 'ok' and filename in ('scene.cs2tl', 'scene.no-breakables.cs2tl')
            assert not info['flags'] & 1, 'Approximate collision data is not accepted'
            source = args.map_root/name/filename
            assert source.stat().st_size == entry['bytes'] and digest(source) == entry['sha256'], source
            relative = Path('maps')/name/filename
            scenes.append({'variant': variant, 'file': relative.as_posix(), 'bytes': entry['bytes'],
                           'sha256': entry['sha256'], 'fingerprint': str(info['source_fingerprint']),
                           'triangles': info['triangle_count'], 'solids': info['solid_count'],
                           'closedSolids': info['closed_solid_count'], 'nodes': info['bvh_node_count'],
                           'flags': info['flags']})
            expected_files.add(source.resolve())
        assert scenes and scenes[0]['variant'] == 'intact'
        maps.append({'name': name, 'resource': 'maps/'+name+'.vpk', 'scenes': scenes})
    assert len(maps) == len({m['name'] for m in maps}) and maps
    assert expected_files == {p.resolve() for p in args.map_root.rglob('*.cs2tl')}, 'Unidentified collision cache'
    header_files = []
    for name in ('api.h', 'types.h'):
        source = args.include_dir/name
        header_files.append({'file': 'include/cs2-external-autowall/'+name, 'sha256': digest(source)})
    manifest = {'schema': 1, 'abi': 1, 'scope': 'static-physics', 'breakables': 'intact',
                'library': {'file': 'bin/win-x64/cs2-external-autowall.dll', 'bytes': args.dll.stat().st_size,
                            'sha256': args.expected_dll_sha256.lower()},
                'upstreamManifestSha256': digest(upstream_path), 'maps': maps}
    for item in maps:
        for entry in item['scenes']:
            destination = target/entry['file']; destination.parent.mkdir(parents=True, exist_ok=True)
            source = args.map_root/item['name']/Path(entry['file']).name
            shutil.copy2(source, destination)
            assert digest(source) == digest(destination) == entry['sha256'], 'Scene changed during staging'
    library = target/manifest['library']['file']; library.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(args.dll, library)
    assert digest(library) == digest(args.dll) == manifest['library']['sha256']
    for entry in header_files:
        source = args.include_dir/Path(entry['file']).name
        destination = public/entry['file']; destination.parent.mkdir(parents=True, exist_ok=True)
        assert not destination.exists()
        shutil.copy2(source, destination)
        assert digest(destination) == entry['sha256']
    write(target/'manifest.json', manifest)
    write(public/'manifest.json', {'schema': 1, 'abi': 1, 'headers': header_files,
                                 'librarySha256': manifest['library']['sha256'],
                                 'catalogSha256': digest(target/'manifest.json')})
    print(json.dumps({'maps': len(maps), 'cacheFiles': len(expected_files),
                      'cacheBytes': sum(s['bytes'] for m in maps for s in m['scenes']),
                      'librarySha256': manifest['library']['sha256'], 'onlyConvertedMapCaches': True}))


if __name__ == '__main__':
    main()

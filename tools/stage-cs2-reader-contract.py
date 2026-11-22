"""Stage a reviewed CS2 Reader delivery, preserving the prior consumer files.

This does not launch, stop, replace or connect to any running Reader/desktop.
"""
from pathlib import Path
import argparse
import hashlib
import json
import shutil

parser = argparse.ArgumentParser()
parser.add_argument('--package', type=Path, required=True)
parser.add_argument('--backup', type=Path, required=True)
parser.add_argument('--fixtures', action='store_true')
args = parser.parse_args()
root = Path(__file__).resolve().parent.parent
package = args.package.resolve()
backup = args.backup.resolve()
assert not backup.exists(), 'Backup must be a new directory'
assert not package.is_relative_to(backup) and not backup.is_relative_to(package)
delivery = json.loads((package/'delivery.json').read_text(encoding='utf-8-sig'))
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
def copy_changed(source, target):
    # Preserve timestamps for an unchanged frozen evaluator/header so MSBuild
    # does not rebuild unrelated consumers during a small additive delivery.
    if not target.exists() or sha(source) != sha(target):
        shutil.copy2(source, target)
    assert sha(source) == sha(target)
hashes = delivery.get('files')
if hashes is None:
    hashes = json.loads((package/delivery['file_hashes']).read_text(encoding='utf-8-sig'))
for relative, digest in hashes.items():
    file = (package/relative).resolve()
    assert file.is_relative_to(package) and sha(file) == digest, relative
contract_root = root/'protocol/sources/cs2_reader'
old = json.loads((contract_root/'manifest.json').read_text(encoding='utf-8-sig'))
previous = delivery.get('previous_schema_sha256', delivery.get('production', {}).get('schema_sha256'))
assert old['schema_sha256'] == previous, 'Delivery does not follow current frozen contract'
assert sha(package/'protocol/reader/v1/reader-v1.descriptor.pb') == delivery['schema_sha256']
recoil = root/old['recoil_evaluator']['path']
assert sha(recoil) == sha(package/'sdk/include/cs2_reader/v1/recoil.hpp') == old['recoil_evaluator']['sha256']
for path in ('protocol/sources/cs2_reader', 'third_party/cs2-reader/include'):
    shutil.copytree(root/path, backup/path)
if args.fixtures:
    shutil.copytree(root/'tests/fixtures/cs2-reader-v1', backup/'tests/fixtures/cs2-reader-v1')
files = []
for name in ('reader.proto', 'snapshot.proto', 'model.proto', 'contract.json', 'reader-v1.descriptor.pb'):
    relative = f'reader/v1/{name}' if name != 'reader-v1.descriptor.pb' else name
    source = package/'protocol/reader/v1'/name
    target = contract_root/relative
    assert target.resolve().is_relative_to(root)
    target.parent.mkdir(parents=True, exist_ok=True)
    copy_changed(source, target)
    files.append({'path':relative,'sha256':sha(target)})
sdk_files = []
for name in ('recoil.hpp', 'record.hpp', 'schema.hpp'):
    source = package/'sdk/include/cs2_reader/v1'/name
    target = root/'third_party/cs2-reader/include/cs2_reader/v1'/name
    copy_changed(source,target)
    sdk_files.append({'path':target.relative_to(root).as_posix(),'sha256':sha(target)})
contract = json.loads((package/'protocol/reader/v1/contract.json').read_text(encoding='utf-8-sig'))
manifest = {'protocol':contract['protocol'],'schema_sha256':delivery['schema_sha256'], 'files':files,
            'recoil_evaluator':old['recoil_evaluator'],'sdk_files':sdk_files}
(contract_root/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
if args.fixtures:
    target = root/'tests/fixtures/cs2-reader-v1'
    for source in (package/'fixtures').iterdir():
        assert source.is_file(), 'Unexpected fixture subdirectory'
        copy_changed(source, target/source.name)
    copy_changed(package/delivery['official_fixture_manifest'], target/'UPSTREAM_MANIFEST.json')
(backup/'stage-receipt.json').write_text(json.dumps({'previous':old['schema_sha256'],'next':delivery['schema_sha256'],
    'manifest':manifest,'runningProcessesChanged':False},indent=2)+'\n',encoding='utf-8')
print(json.dumps({'ok':True,'schema':delivery['schema_sha256'],'backup':str(backup),'runningProcessesChanged':False}))

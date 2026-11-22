"""Verify and stage an upstream frozen consumer bundle; never modify the live pin."""
import argparse
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import re
import stat
import zipfile

ROOT = Path(__file__).resolve().parents[1]
MAX_FILES = 512
MAX_BYTES = 64 * 1024 * 1024


def digest(data):
    return hashlib.sha256(data).hexdigest()


def unique_object(pairs):
    result = {}
    for name, value in pairs:
        if name in result:
            raise ValueError(f"Duplicate JSON key: {name}")
        result[name] = value
    return result


def decode_json(data):
    return json.loads(data.decode("utf-8-sig"), object_pairs_hook=unique_object)


def relative_path(name):
    path = PurePosixPath(name)
    if (not name or "\\" in name or ":" in name or re.search(r'[\x00-\x1f<>"|?*]', name) or path.is_absolute()
            or any(part in ("", ".", "..") for part in name.split("/"))):
        raise ValueError(f"Invalid bundle-relative path: {name!r}")
    # Prevent Win32 normalization/aliasing and reserved devices as well as ../.
    for part in path.parts:
        if part.endswith((".", " ")) or re.match(r"(?i)^(con|prn|aux|nul|com[1-9]|lpt[1-9])(?:\.|$)", part):
            raise ValueError(f"Invalid Windows bundle path: {name!r}")
    return path


def validate(raw, expected_archive, expected_descriptor):
    for value in (expected_archive, expected_descriptor):
        if not re.fullmatch(r"[0-9a-fA-F]{64}", value):
            raise ValueError("Expected hashes must contain exactly 64 hexadecimal digits")
    if digest(raw) != expected_archive.lower():
        raise ValueError("Archive SHA256 does not match the upstream handoff")
    files, aliases, total = {}, set(), 0
    with zipfile.ZipFile(io.BytesIO(raw)) as archive:
        if len(archive.infolist()) > MAX_FILES:
            raise ValueError("Consumer bundle exceeds the entry limit")
        for entry in archive.infolist():
            name = entry.filename
            relative_path(name.rstrip("/") if entry.is_dir() else name)
            if stat.S_ISLNK(entry.external_attr >> 16):
                raise ValueError("Consumer bundle contains a symbolic link")
            alias = name.rstrip("/").casefold()
            if alias in aliases:
                raise ValueError(f"Duplicate/case-aliased ZIP entry: {name}")
            aliases.add(alias)
            if entry.is_dir():
                continue
            total += entry.file_size
            if entry.file_size > 16 * 1024 * 1024 or total > MAX_BYTES:
                raise ValueError("Consumer bundle exceeds the uncompressed size limit")
            files[name] = archive.read(entry)
    required = {"consumer-sources.txt", "protocol-schema-manifest.json",
                "protocol-descriptor.pb", "src/synchronization/codec/schema.hpp"}
    if not required <= files.keys():
        raise ValueError("Consumer bundle is missing required metadata or codec schema")
    manifests = {"consumer-manifest.json", "UPSTREAM_MANIFEST.json"} & files.keys()
    if len(manifests) != 1:
        raise ValueError("Consumer bundle requires one unambiguous upstream manifest")
    manifest_name = next(iter(manifests))
    manifest = decode_json(files[manifest_name])
    declared = manifest["files"]
    if set(files) != set(declared) | {manifest_name}:
        raise ValueError("ZIP contents differ from the complete consumer manifest")
    for name, expected in declared.items():
        relative_path(name)
        if not isinstance(expected, str) or digest(files[name]) != expected.lower():
            raise ValueError(f"Consumer file hash mismatch: {name}")
    descriptor = expected_descriptor.lower()
    schema = decode_json(files["protocol-schema-manifest.json"])
    if (manifest["descriptor_sha256"].lower() != descriptor
            or schema["descriptor_sha256"].lower() != descriptor
            or digest(files["protocol-descriptor.pb"]) != descriptor):
        raise ValueError("Descriptor hashes disagree with the expected frozen contract")
    header = files["src/synchronization/codec/schema.hpp"].decode("utf-8-sig")
    header_hashes = re.findall(r"CodecSchemaDescriptorSha256:\s*([0-9a-fA-F]{64})", header)
    if len(header_hashes) != 1 or header_hashes[0].lower() != descriptor:
        raise ValueError("Consumer codec schema does not match the descriptor")
    schemas = schema["schemas"]
    paths = ["protocol/" + item["path"] for item in schemas]
    if len(paths) != len(set(paths)) or set(paths) != {p for p in files if p.endswith(".proto")}:
        raise ValueError("Proto file list differs from schema manifest")
    for item, name in zip(schemas, paths):
        relative_path(name)
        if digest(files[name]) != item["sha256"].lower():
            raise ValueError(f"Proto schema hash mismatch: {name}")
    source_files = manifest["source_files"]
    actual = {p for p in files if p.startswith("src/") and p.endswith(".cpp")}
    listed = files["consumer-sources.txt"].decode("utf-8-sig").splitlines()
    if (type(manifest["source_count"]) is not int or manifest["source_count"] != len(actual)
            or len(source_files) != len(set(source_files)) or set(source_files) != actual
            or len(listed) != len(set(listed)) or set(listed) != actual):
        raise ValueError("Consumer translation-unit lists disagree")
    if manifest_name == "UPSTREAM_MANIFEST.json":
        source_manifest = decode_json(files["SOURCE_MANIFEST.json"])
        expected = {p.removeprefix("src/"): digest(data)
                    for p, data in files.items() if p.startswith("src/")}
        source_paths = [entry["path"] for entry in source_manifest]
        if (len(source_paths) != len(set(source_paths))
                or {entry["path"]: entry["sha256"] for entry in source_manifest} != expected
                or manifest["consumer_source_count"] != len(expected)
                or manifest["proto_count"] != len(paths)):
            raise ValueError("Upstream source/proto manifest counts or hashes disagree")
    return files, schema, source_files


def stage(archive, output, expected_archive, expected_descriptor, external_output=False):
    output = output.resolve()
    allowed = (ROOT / "out").resolve()
    if external_output:
        if output.is_relative_to(ROOT) or ROOT.is_relative_to(output):
            raise ValueError("External staging output must be outside the repository and its ancestors")
    elif output == allowed or not output.is_relative_to(allowed):
        raise ValueError("Staging output must be a fresh directory beneath this repository's out/")
    if output.exists():
        raise ValueError("Refusing to replace an existing staging directory")
    if archive.stat().st_size > MAX_BYTES:
        raise ValueError("Consumer archive exceeds the input size limit")
    raw = archive.read_bytes()
    files, schema, sources = validate(raw, expected_archive, expected_descriptor)
    # Validation happens before creating output. receipt.json is written last;
    # an interrupted write leaves an inspectable, unaccepted partial stage.
    output.mkdir(parents=True)
    for name, data in files.items():
        target = output / "package" / str(relative_path(name))
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
        if digest(target.read_bytes()) != digest(data):
            raise OSError(f"Staged file verification failed: {name}")
    source_manifest = [{"path": p.removeprefix("src/"), "sha256": digest(data)}
                       for p, data in sorted(files.items()) if p.startswith("src/")]
    (output / "SOURCE_MANIFEST.json").write_text(json.dumps(source_manifest, indent=2), encoding="utf-8")
    receipt = dict(archive=str(archive.resolve()), archiveSha256=digest(raw),
                   descriptorSha256=expected_descriptor.lower(), packageFiles=len(files),
                   consumerFiles=len(source_manifest), translationUnits=len(sources), protoFiles=len(schema["schemas"]),
                   hashesVerified=True, protobufDecoded=False, semanticValidated=False,
                   productionModified=False, readerDeployed=False, output=str(output))
    (output / "receipt.json").write_text(json.dumps(receipt, indent=2), encoding="utf-8")
    return receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archive", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--archive-sha256", required=True)
    parser.add_argument("--descriptor-sha256", required=True)
    parser.add_argument("--external-output", action="store_true",
                        help="Use a fresh staging directory outside this repository")
    args = parser.parse_args()
    try:
        print(json.dumps(stage(args.archive, args.output, args.archive_sha256, args.descriptor_sha256,
                               args.external_output), indent=2))
    except (ValueError, KeyError, TypeError, OSError, zipfile.BadZipFile) as error:
        parser.exit(1, f"Reader contract staging failed: {error}\n")


if __name__ == "__main__":
    main()

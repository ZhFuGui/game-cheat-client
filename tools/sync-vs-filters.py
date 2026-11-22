"""Regenerate Visual Studio virtual folders from actual project input globs."""
from pathlib import Path
import uuid
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parent.parent
NS = "http://schemas.microsoft.com/developer/msbuild/2003"
ET.register_namespace("", NS)


def sync(project: Path) -> None:
    source = ET.parse(project).getroot()
    inputs = [source]
    for item in source.findall(f"{{{NS}}}Import"):
        name = item.get("Project", "").replace("\\", "/")
        if name.endswith("/client-sources.props") and "$" not in name:
            imported = (project.parent / name).resolve()
            if not imported.is_relative_to(ROOT):
                raise ValueError("Reader compilation items escape the repository")
            inputs.append(ET.parse(imported).getroot())
    entries = set()
    for tag in ("ClCompile", "ClInclude", "ResourceCompile"):
        for item in (item for tree in inputs for item in tree.iter(f"{{{NS}}}{tag}")):
            pattern = item.get("Include", "")
            if not pattern.startswith("$(RepoRoot)"):
                continue
            pattern = pattern.removeprefix("$(RepoRoot)").replace("\\", "/")
            excluded = set()
            for exclusion in item.get("Exclude", "").split(";"):
                if exclusion.startswith("$(RepoRoot)"):
                    excluded.update(ROOT.glob(exclusion.removeprefix("$(RepoRoot)").replace("\\", "/")))
            for file in ROOT.glob(pattern):
                if file.is_file() and file not in excluded:
                    entries.add((tag, file.relative_to(ROOT).as_posix()))
    output = ET.Element(f"{{{NS}}}Project", {"ToolsVersion": "4.0"})
    folders = sorted({str(Path(file).parent).replace("/", "\\") for _, file in entries})
    parents = {str(parent).replace("/", "\\") for _, file in entries for parent in Path(file).parents if str(parent) != "."}
    group = ET.SubElement(output, f"{{{NS}}}ItemGroup")
    for folder in sorted(set(folders) | parents):
        entry = ET.SubElement(group, f"{{{NS}}}Filter", {"Include": folder})
        ET.SubElement(entry, f"{{{NS}}}UniqueIdentifier").text = "{" + str(uuid.uuid5(uuid.NAMESPACE_URL, "mousebox-aimbot/" + folder)).upper() + "}"
    group = ET.SubElement(output, f"{{{NS}}}ItemGroup")
    for tag, file in sorted(entries):
        entry = ET.SubElement(group, f"{{{NS}}}{tag}", {"Include": "$(RepoRoot)" + file.replace("/", "\\")})
        ET.SubElement(entry, f"{{{NS}}}Filter").text = str(Path(file).parent).replace("/", "\\")
    ET.indent(output, space="  ")
    ET.ElementTree(output).write(str(project) + ".filters", encoding="utf-8", xml_declaration=True)
    print(f"{project.name}: {len(entries)} files")


if __name__ == "__main__":
    for file in sorted((ROOT / "projects").glob("*.vcxproj")):
        sync(file)

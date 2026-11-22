#!/usr/bin/env python3
"""Archive public PUBG weapon references. Never connects to Reader or the mousebox.

Python standard library only. Fetch is explicit, sequential and paced; --verify is offline.
Published figures are reference data, not verified live-game ballistic configuration.
"""
from __future__ import annotations

import argparse
import datetime as dt
import gzip
import hashlib
import json
import math
import pathlib
import re
import time
import urllib.parse
import urllib.request
from html.parser import HTMLParser

REPO = pathlib.Path(__file__).resolve().parents[1]
UA = "Mousebox-public-reference/1.0 (local research archive; sequential requests)"
GS = "https://pubg.gs"
PLANET = "https://pubgplanet.com"
GITHUB_COMMIT = "32b13b51128b8d8909ae5e77f3b833e01230b24d"


def utc() -> str:
    return dt.datetime.now(dt.timezone.utc).isoformat(timespec="seconds")


def write_json(path: pathlib.Path, value) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2, allow_nan=False) + "\n", encoding="utf-8")


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


class Node:
    def __init__(self, tag="", attrs=()):
        self.tag, self.attrs, self.children = tag, dict(attrs), []

    def walk(self, tag=None):
        if tag is None or self.tag == tag:
            yield self
        for child in self.children:
            if isinstance(child, Node):
                yield from child.walk(tag)

    def text(self):
        if self.tag in ("script", "style"):
            return ""
        return " ".join(" ".join(c.text() if isinstance(c, Node) else c for c in self.children).split())


class Document(HTMLParser):
    VOID = {"area", "base", "br", "col", "embed", "hr", "img", "input", "link", "meta", "param", "source", "track", "wbr"}

    def __init__(self, html):
        super().__init__(convert_charrefs=True)
        self.root = Node("root")
        self.stack = [self.root]
        self.feed(html)

    def handle_starttag(self, tag, attrs):
        n = Node(tag, attrs)
        self.stack[-1].children.append(n)
        if tag not in self.VOID:
            self.stack.append(n)

    def handle_startendtag(self, tag, attrs):
        self.handle_starttag(tag, attrs)
        if tag not in self.VOID:
            self.handle_endtag(tag)

    def handle_endtag(self, tag):
        for i in range(len(self.stack) - 1, 0, -1):
            if self.stack[i].tag == tag:
                self.stack = self.stack[:i]
                break

    def handle_data(self, data):
        self.stack[-1].children.append(data)


def first(node, tag):
    return next(node.walk(tag), None)


def tables(node):
    result = []
    for table in node.walk("table"):
        rows = []
        for tr in table.walk("tr"):
            cells = [c.text() for c in tr.children if isinstance(c, Node) and c.tag in ("th", "td")]
            if cells:
                rows.append(cells)
        result.append(rows)
    return result


def pairs(node):
    result = []
    for div in node.walk("div"):
        spans = [c for c in div.children if isinstance(c, Node) and c.tag == "span"]
        if "justify-between" in div.attrs.get("class", "") and len(spans) == 2:
            result.append({"label": spans[0].text(), "value": spans[1].text()})
    return result


def links(node, prefix):
    return list(dict.fromkeys(a.attrs["href"] for a in node.walk("a") if a.attrs.get("href", "").startswith(prefix)))


NUMBER = r"[-+]?(?:\d[\d,]*)(?:\.\d+)?(?:[eE][-+]?\d+)?"


def number(value):
    if value is None or value in ("", "—", "Not recorded", "N/A"):
        return None
    m = re.fullmatch(rf"\s*({NUMBER})(?:\s*(?:m/s|m|s|RPM|damage))?\s*", str(value))
    if not m:
        return None
    x = float(m[1].replace(",", ""))
    if not math.isfinite(x):
        raise ValueError("Non-finite source number")
    return int(x) if x.is_integer() else x


class Archive:
    def __init__(self, output, fetch, delay):
        self.output, self.fetch, self.delay = output, fetch, max(1.5, delay)
        self.path = output / "responses.json"
        self.records = json.loads(self.path.read_text(encoding="utf-8")) if self.path.exists() else {}
        self.last = 0.0

    def get(self, url):
        if url in self.records:
            r = self.records[url]
            path = (self.output / r["archive"]).resolve()
            if not path.is_relative_to(self.output):
                raise ValueError("Response path leaves archive")
            data = gzip.decompress(path.read_bytes())
            if digest(data) != r["sha256"]:
                raise ValueError("Cached response hash mismatch: " + url)
            return data.decode("utf-8")
        if not self.fetch:
            raise ValueError("Missing offline response: " + url)
        pause = self.delay - (time.monotonic() - self.last)
        if pause > 0:
            time.sleep(pause)
        request = urllib.request.Request(url, headers={"User-Agent": UA, "Accept": "text/html,application/json,text/plain"})
        self.last = time.monotonic()
        # No credential, authentication, challenge bypass or executable remote content.
        with urllib.request.urlopen(request, timeout=25) as response:
            if response.status != 200:
                raise ValueError("Unexpected HTTP status: " + str(response.status))
            data = response.read(8_000_001)
            if len(data) > 8_000_000:
                raise ValueError("Response exceeds archive limit")
            filename = "raw/" + digest(url.encode())[:24] + ".gz"
            path = self.output / filename
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(gzip.compress(data, mtime=0))
            self.records[url] = {"url": url, "final_url": response.url, "fetched_utc": utc(), "status": 200,
                                 "content_type": response.headers.get("Content-Type"), "bytes": len(data),
                                 "sha256": digest(data), "archive": filename}
            write_json(self.path, self.records)
        return data.decode("utf-8")


def footer_build(root):
    foot = first(root, "footer")
    m = re.search(r"build (\d+) .*?updated (\d{4}-\d{2}-\d{2})", foot.text() if foot else "")
    if not m:
        raise ValueError("Missing build/date footer")
    return {"steam_build": int(m[1]), "extracted_date": m[2]}


def gs_index(html):
    doc = Document(html).root
    main = first(doc, "main")
    rows = []
    for tr in main.walk("tr"):
        if "data-row" not in tr.attrs:
            continue
        routes = links(tr, "/weapons/")
        if len(routes) != 1:
            raise ValueError("Expected one weapon detail URL per directory row")
        cells = [c.text() for c in tr.children if isinstance(c, Node) and c.tag == "td"]
        rows.append({"slug": routes[0].strip("/").split("/")[-1], "url": GS + routes[0],
                     "name": tr.attrs.get("data-n"), "category": tr.attrs.get("data-f"),
                     "published_columns": cells, "attributes": tr.attrs,
                     "ammo_urls": [GS + x for x in links(tr, "/ammo/")]})
    declared = re.search(r"(\d+) weapons", main.text())
    if not declared or len(rows) != int(declared[1]) or len({r["slug"] for r in rows}) != len(rows):
        raise ValueError("Weapon directory count/uniqueness mismatch")
    return rows, footer_build(doc)


def gs_weapon(archive, row, build):
    doc = Document(archive.get(row["url"])).root
    if footer_build(doc) != build:
        raise ValueError("Source changed builds during collection: " + row["url"])
    main = first(doc, "main")
    title = first(main, "h1")
    if title is None or title.text() != row["name"]:
        raise ValueError("Detail title differs from directory: " + row["url"])
    sections = []
    for section in main.walk("section"):
        heading = first(section, "h2")
        if heading is None:
            continue
        graphs = []
        for svg in section.walk("svg"):
            label = svg.attrs.get("aria-label")
            if label not in ("velocity by distance", "recoil by shot"):
                continue
            titles = [n.text() for n in svg.walk("title")]
            points = []
            pattern = rf"distance ({NUMBER}) m: velocity ({NUMBER}) m/s" if label == "velocity by distance" else rf"shot ({NUMBER}): recoil ({NUMBER})"
            for text in titles:
                m = re.fullmatch(pattern, text)
                if not m:
                    raise ValueError("Unrecognized chart point: " + text)
                points.append([number(m[1]), number(m[2])])
            declared = re.search(r"(\d+) curve keys", section.text())
            if not declared or len(points) != int(declared[1]):
                raise ValueError("Chart key count mismatch: " + row["url"])
            if any(b[0] <= a[0] for a, b in zip(points, points[1:])):
                raise ValueError("Chart x values are not strictly increasing")
            graphs.append({"published_axis_label": label, "published_key_count": len(points), "points": points,
                           "point_text": titles, "precision": "published tooltip; formatting may round source values",
                           "engine_interpolation_and_tangents": None})
        sections.append({"heading": heading.text(), "fields": pairs(section), "tables": tables(section),
                         "graphs": graphs, "attachment_urls": [GS + x for x in links(section, "/attachments/")]})
    fields = {p["label"]: p["value"] for p in pairs(main)}
    item_id = fields.get("Item id")
    if not item_id:
        raise ValueError("Missing exact item identity: " + row["url"])
    velocity = number(fields.get("Muzzle velocity"))
    indexed_velocity = number(row["attributes"].get("data-velocity"))
    if velocity is not None and indexed_velocity is not None and velocity != indexed_velocity:
        raise ValueError("Index/detail velocity disagreement: " + row["url"])
    curves = [g for s in sections for g in s["graphs"] if g["published_axis_label"] == "velocity by distance"]
    warnings = []
    if curves and any(x < 0 for x, _ in curves[0]["points"]):
        warnings.append("Published velocity curve has negative-distance keys; preserved as published, physical query domain needs review")
    if curves and any(y <= 0 for _, y in curves[0]["points"]):
        warnings.append("Published velocity curve contains non-positive speed; requires review before flight-time integration")
    if curves and velocity is not None and curves[0]["points"][0][0] == 0 and abs(curves[0]["points"][0][1] - velocity) > 1:
        warnings.append("Published curve at 0m differs from InitialSpeed; preserved separately, never rescaled or silently reconciled")
    return {"item_id": item_id, "name": row["name"], "category": row["category"], "blueprint": fields.get("Blueprint"),
            "weapon_tag": fields.get("Weapon tag"), "parent_class": fields.get("Parent class"),
            "source_url": row["url"], "source_response_sha256": archive.records[row["url"]]["sha256"],
            "directory": row, "sections": sections, "warnings": warnings,
            "ballistics_reference": {"muzzle_velocity_mps": velocity, "max_travel_distance_m": number(fields.get("Max travel distance")),
                                     "reference_distance_m": number(fields.get("Reference distance")),
                                     "range_modifier": number(fields.get("Range modifier")),
                                     "projectile_hit_damage": number(fields.get("Projectile hit damage")),
                                     "velocity_curve": curves[0] if curves else None,
                                     "gravity_mps2": None, "drag_law": None, "shooter_velocity_inheritance": None,
                                     "zeroing_and_sight_geometry": None, "runtime_verified": False}}


def related_page(archive, url, build):
    doc = Document(archive.get(url)).root
    if footer_build(doc) != build:
        raise ValueError("Related source build mismatch: " + url)
    main = first(doc, "main")
    return {"url": url, "fields": pairs(main), "tables": tables(main),
            "published_text": main.text(), "source_response_sha256": archive.records[url]["sha256"]}


def collect_gs(archive):
    about = archive.get(GS + "/about/")
    archive.get(GS + "/terms/")
    archive.get(GS + "/robots.txt")
    rows, build = gs_index(archive.get(GS + "/weapons/"))
    if footer_build(Document(about).root) != build:
        raise ValueError("About and directory build mismatch")
    weapons = []
    for i, row in enumerate(rows):
        weapons.append(gs_weapon(archive, row, build))
        if (i + 1) % 10 == 0 or i + 1 == len(rows):
            print(f"pubg.gs: {i + 1}/{len(rows)} weapon pages", flush=True)
    ids = [w["item_id"] for w in weapons]
    if len(ids) != len(set(ids)):
        raise ValueError("Duplicate item identities")
    related = []
    for route in ("/attachments/", "/ammo/", "/damage/", "/rules/"):
        # Only linked public pages; no private endpoints or application scripts.
        if route not in links(Document(about).root, route):
            raise ValueError("Related route was not linked by source: " + route)
        related.append(related_page(archive, GS + route, build))
    return {"schema_version": 1, "source": "pubg.gs", "source_build": build, "archive_created_utc": utc(),
            "source_claim": "Automatically extracted game files; exact live server version and extraction not independently verified",
            "runtime_use": "reference_only", "directory_count": len(weapons), "weapons": weapons, "related_pages": related}


def definitions(node):
    result = {}
    for div in node.walk("div"):
        children = [c for c in div.children if isinstance(c, Node)]
        if len(children) == 2 and children[0].tag == "dt" and children[1].tag == "dd":
            label, value = children[0].text(), children[1].text()
            numeric = number(value)
            result[label] = {"published_value": value, "number": numeric}
    return result


def collect_planet(archive):
    directory_url = PLANET + "/wiki/weapons"
    main = first(Document(archive.get(directory_url)).root, "main")
    routes = [x for x in links(main, "/wiki/weapons/") if re.fullmatch(r"/wiki/weapons/[a-z0-9-]+", x)]
    if not routes:
        raise ValueError("No public comparison weapon directory")
    result = []
    for i, route in enumerate(routes):
        url = PLANET + route
        node = first(Document(archive.get(url)).root, "main")
        title = first(node, "h1")
        records = []
        for li in node.walk("li"):
            # Leaf history rows own their source record and timestamp; do not duplicate ancestor lists.
            if sum(1 for _ in li.walk("li")) != 1:
                continue
            codes = [c.text() for c in li.walk("code")]
            record_ids = [c for c in codes if re.fullmatch(r"pubg-\d+-[a-f0-9]+", c)]
            hashes = [c for c in codes if re.fullmatch(r"[a-f0-9]{64}", c)]
            timestamp = first(li, "time")
            if not record_ids:
                continue
            if len(record_ids) != 1 or len(hashes) != 1 or timestamp is None:
                raise ValueError("Incomplete comparison source record: " + url)
            records.append({"record_id": record_ids[0], "steam_build": int(record_ids[0].split("-")[1]),
                            "recorded_utc": timestamp.attrs.get("datetime"), "source_artifact_sha256": hashes[0],
                            "published_fields": definitions(li)})
        records.sort(key=lambda x: x["recorded_utc"], reverse=True)
        if not records:
            raise ValueError("Missing comparison records: " + url)
        result.append({"name": title.text(), "source_url": url, "source_response_sha256": archive.records[url]["sha256"],
                       "latest_record": records[0], "published_history": records, "published_tables": tables(node)})
        if (i + 1) % 10 == 0 or i + 1 == len(routes):
            print(f"pubgplanet: {i + 1}/{len(routes)} comparison pages", flush=True)
    return {"schema_version": 1, "source": "pubgplanet.com", "runtime_use": "reference_only_separate_builds",
            "directory_count": len(result), "weapons": result}


def collect_names(archive):
    prefix = f"https://raw.githubusercontent.com/pubg/api-assets/{GITHUB_COMMIT}/dictionaries/telemetry/"
    items = json.loads(archive.get(prefix + "item/itemId.json"))
    damage = json.loads(archive.get(prefix + "damageCauserName.json"))
    return {"schema_version": 1, "source": "Official PUBG API assets (identity dictionary only, no ballistics)",
            "repository": "https://github.com/pubg/api-assets", "commit": GITHUB_COMMIT,
            "item_names": {k: v for k, v in items.items() if k.startswith("Item_Weapon_")},
            "damage_causer_names": {k: v for k, v in damage.items() if k.startswith("Weap")}}


def aliases_and_checks(gs, planet, official):
    exact = {}
    for weapon in gs["weapons"]:
        item = weapon["item_id"]
        values = [(item, "item_id"), (weapon["name"], "published_display_name"),
                  (weapon["blueprint"], "published_blueprint"), (weapon["weapon_tag"], "published_weapon_tag")]
        blueprint = weapon["blueprint"]
        if blueprint:
            values.append((blueprint + "_C", "derived_ue_generated_class_name; validate against Reader"))
            official_name = official["damage_causer_names"].get(blueprint + "_C")
            if official_name:
                values.append((official_name, "official_damage_causer_dictionary_exact_class_match"))
        if item in official["item_names"]:
            values.append((official["item_names"][item], "official_item_dictionary_exact_item_match"))
        for value, kind in values:
            if value:
                entry = {"item_id": item, "evidence": kind}
                if entry not in exact.setdefault(value, []):
                    exact[value].append(entry)
    ambiguous = {k: sorted({x["item_id"] for x in v}) for k, v in exact.items() if len({x["item_id"] for x in v}) > 1}
    index = {"schema_version": 1, "matching": "exact; ambiguous aliases must not select a record automatically",
             "runtime_verified_with_reader": False, "aliases": exact, "ambiguous_aliases": ambiguous}
    issues = [{"item_id": w["item_id"], "name": w["name"], "warnings": w["warnings"]} for w in gs["weapons"] if w["warnings"]]
    checks = {"schema_version": 1, "checked_utc": utc(), "primary_build": gs["source_build"],
              "primary_weapon_entries": len(gs["weapons"]), "unique_primary_item_ids": len({w["item_id"] for w in gs["weapons"]}),
              "entries_with_muzzle_velocity": sum(w["ballistics_reference"]["muzzle_velocity_mps"] is not None for w in gs["weapons"]),
              "entries_with_published_velocity_curve": sum(w["ballistics_reference"]["velocity_curve"] is not None for w in gs["weapons"]),
              "comparison_entries": len(planet["weapons"]), "comparison_latest_builds": sorted({w["latest_record"]["steam_build"] for w in planet["weapons"]}),
              "ambiguous_alias_count": len(ambiguous), "source_consistency_issues": issues,
              "numerical_policy": "preserve published figures; no imputation, curve rescaling, cross-build overwrite, integration or assumed gravity",
              "runtime_ballistics_ready": False,
              "missing_runtime_inputs": ["verified engine gravity and drag/integration law", "full curve interpolation/tangents and units",
                                         "actual muzzle/shot transform and sight zero", "shooter velocity inheritance law",
                                         "complete self/target world velocities and timestamps", "attachments/current live-version verification"]}
    return index, checks


def profile_pack(gs):
    """Stable solver-facing envelope; source-specific page structure stays in its adapter."""
    profiles = []
    for i, weapon in enumerate(gs["weapons"]):
        b = weapon["ballistics_reference"]
        curve = b["velocity_curve"]
        profiles.append({"profile_id": weapon["item_id"], "display_name": weapon["name"],
                         "bindings": {"item_ids": [weapon["item_id"]],
                                      "actor_class_names": [weapon["blueprint"] + "_C"] if weapon["blueprint"] else [],
                                      "verified_with_reader": False},
                         "applicability": {"ammo_item_id": None, "attachment_signature": None, "firing_mode": None},
                         "readiness": "reference_only",
                         "model": {"id": "published_reference.v1", "parameters": {
                             "initial_speed_mps": b["muzzle_velocity_mps"], "gravity_vector_mps2": None,
                             "shooter_velocity_inheritance": None, "simulation_step_s": None,
                             "max_travel_distance_m": b["max_travel_distance_m"],
                             "velocity_curve": {"abscissa": "published_distance_m", "ordinate": "published_speed_mps",
                                                "keys": curve["points"], "interpolation": None, "tangents": None,
                                                "precision": curve["precision"]} if curve else None,
                             "drag_law": None, "sight_and_muzzle_geometry": None}},
                         "provenance": {"source_ref": weapon["source_url"], "content_sha256": weapon["source_response_sha256"],
                                        "locator": f"pubg-gs.json#/weapons/{i}"}, "notes": weapon["warnings"]})
    semantic = {"schema_version": 1, "pack_id": "pubg-gs-" + str(gs["source_build"]["steam_build"]),
                "units": {"distance": "m", "time": "s", "speed": "m/s", "angle": "rad"},
                "source": {"kind": "public_reference", "producer": "pubg.gs", "game_build": str(gs["source_build"]["steam_build"]),
                           "recorded_date": gs["source_build"]["extracted_date"], "live_game_verified": False},
                "profiles": profiles}
    fingerprint = digest(json.dumps(semantic, ensure_ascii=False, sort_keys=True, separators=(",", ":"), allow_nan=False).encode())
    return {**semantic, "revision": "sha256:" + fingerprint}


def validate_profile_pack(pack):
    # Critical envelope invariants, independent of optional third-party schema validators.
    def finite_values(value):
        if isinstance(value, float) and not math.isfinite(value):
            raise ValueError("Non-finite profile value")
        if isinstance(value, dict):
            for child in value.values():
                finite_values(child)
        elif isinstance(value, list):
            for child in value:
                finite_values(child)
    finite_values(pack)
    if type(pack["schema_version"]) is not int or pack["schema_version"] != 1 or pack["units"] != {"distance": "m", "time": "s", "speed": "m/s", "angle": "rad"}:
        raise ValueError("Unsupported profile schema or units")
    if not isinstance(pack.get("pack_id"), str) or not pack["pack_id"] or not isinstance(pack.get("revision"), str) or not pack["revision"]:
        raise ValueError("Missing pack identity or producer revision")
    if not pack["profiles"]:
        raise ValueError("Empty profile pack")
    seen = set()
    for profile in pack["profiles"]:
        if not profile["profile_id"] or profile["profile_id"] in seen:
            raise ValueError("Duplicate profile identity")
        seen.add(profile["profile_id"])
        if not re.fullmatch(r"[a-z][a-z0-9_.-]*\.v[1-9][0-9]*", profile["model"]["id"]):
            raise ValueError("Unversioned model identifier")
        value = profile["model"]["parameters"].get("initial_speed_mps")
        if value is not None and (isinstance(value, bool) or not math.isfinite(value) or value <= 0):
            raise ValueError("Invalid initial speed")
        gravity = profile["model"]["parameters"].get("gravity_vector_mps2")
        if gravity is not None and (len(gravity) != 3 or not all(isinstance(x, (int, float)) and not isinstance(x, bool) and math.isfinite(x) for x in gravity)):
            raise ValueError("Invalid gravity vector")
    semantic = {k: v for k, v in pack.items() if k != "revision"}
    expected = "sha256:" + digest(json.dumps(semantic, ensure_ascii=False, sort_keys=True, separators=(",", ":"), allow_nan=False).encode())
    if pack["source"]["kind"] == "public_reference" and pack["source"]["producer"] == "pubg.gs" and pack["revision"] != expected:
        raise ValueError("Profile semantic revision mismatch")


def write_catalog(output, gs):
    rows = ["# 枪械与武器资料索引", "", f"来源：pubg.gs；Steam Build {gs['source_build']['steam_build']}；提取日 {gs['source_build']['extracted_date']}。",
            "", "这里列出来源目录的全部条目；空缺保持“未提供”。曲线为网站公布的采样点，不含引擎插值切线。完整字段及兼容配件见 pubg-gs.json。", "",
            "| 名称 | 类别 | 武器 Actor 类名候选 | 初速 m/s | 弹速曲线点数 |", "|---|---|---|---:|---:|"]
    for w in gs["weapons"]:
        b = w["ballistics_reference"]
        speed = str(b["muzzle_velocity_mps"]) if b["muzzle_velocity_mps"] is not None else "未提供"
        count = str(b["velocity_curve"]["published_key_count"]) if b["velocity_curve"] else "未提供"
        cls = w["blueprint"] + "_C" if w["blueprint"] else "未提供"
        rows.append(f"| [{w['name']}]({w['source_url']}) | {w['category']} | {cls} | {speed} | {count} |")
    (output / "catalog.md").write_text("\n".join(rows) + "\n", encoding="utf-8")


def write_manifest(output, archive):
    files = []
    for path in sorted(output.rglob("*")):
        if path.is_file() and path.name != "manifest.json":
            data = path.read_bytes()
            files.append({"path": path.relative_to(output).as_posix(), "bytes": len(data), "sha256": digest(data)})
    result = {"schema_version": 1, "created_utc": utc(), "purpose": "Public PUBG PC weapon reference; not runtime ballistic calibration",
              "source_response_count": len(archive.records), "files": files}
    write_json(output / "manifest.json", result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=pathlib.Path, default=REPO / "data/games/pubg/weapons/2026-10-15")
    parser.add_argument("--fetch", action="store_true", help="Fetch missing public pages, sequentially; reuse archive")
    parser.add_argument("--delay", type=float, default=1.5, help="Minimum request interval, clamped to >=1.5 seconds")
    parser.add_argument("--verify", action="store_true", help="Offline archive and data integrity verification")
    parser.add_argument("--verify-pack", type=pathlib.Path, help="Check a standalone Reader/calibration/public profile pack without source crawling")
    args = parser.parse_args()
    args.output = args.output.resolve()
    if args.verify_pack:
        pack = json.loads(args.verify_pack.read_text(encoding="utf-8-sig"))
        validate_profile_pack(pack)
        print(f"PASS: standalone pack {pack['pack_id']} / {pack['revision']}, {len(pack['profiles'])} profiles; no network or runtime action")
        return
    if args.verify:
        manifest = json.loads((args.output / "manifest.json").read_text(encoding="utf-8"))
        for row in manifest["files"]:
            p = (args.output / row["path"]).resolve()
            if not p.is_relative_to(args.output) or digest(p.read_bytes()) != row["sha256"]:
                raise ValueError("Manifest mismatch: " + row["path"])
        archive = Archive(args.output, False, args.delay)
        for url in archive.records:
            archive.get(url)
        gs = collect_gs(archive)
        expected = json.loads((args.output / "pubg-gs.json").read_text(encoding="utf-8"))
        if gs["weapons"] != expected["weapons"] or gs["related_pages"] != expected["related_pages"]:
            raise ValueError("Primary normalized data differs from archived pages")
        planet = collect_planet(archive)
        if planet != json.loads((args.output / "pubgplanet.json").read_text(encoding="utf-8")):
            raise ValueError("Comparison normalized data differs from archived pages")
        official = collect_names(archive)
        if official != json.loads((args.output / "official-names.json").read_text(encoding="utf-8")):
            raise ValueError("Official name data differs from pinned originals")
        index, checks = aliases_and_checks(gs, planet, official)
        if index != json.loads((args.output / "aliases.json").read_text(encoding="utf-8")):
            raise ValueError("Alias index differs from sources")
        pack = json.loads((args.output / "profiles.json").read_text(encoding="utf-8"))
        validate_profile_pack(pack)
        if pack != profile_pack(gs):
            raise ValueError("Stable profile pack differs from primary adapter output")
        print(f"PASS: {len(manifest['files'])} files and {len(archive.records)} source responses verified offline; all normalized weapons and aliases re-parsed")
        return
    archive = Archive(args.output, args.fetch, args.delay)
    result = collect_gs(archive)
    write_json(args.output / "pubg-gs.json", result)
    comparison = collect_planet(archive)
    write_json(args.output / "pubgplanet.json", comparison)
    official = collect_names(archive)
    write_json(args.output / "official-names.json", official)
    index, checks = aliases_and_checks(result, comparison, official)
    write_json(args.output / "aliases.json", index)
    write_json(args.output / "checks.json", checks)
    pack = profile_pack(result)
    validate_profile_pack(pack)
    write_json(args.output / "profiles.json", pack)
    write_catalog(args.output, result)
    write_manifest(args.output, archive)
    print(json.dumps(checks, ensure_ascii=False), flush=True)


if __name__ == "__main__":
    main()

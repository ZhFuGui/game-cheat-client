"""Extract reviewed CS2 Hitbox definitions with a pinned Source 2 Viewer CLI.

Only numeric definitions and their provenance enter the repository. Original game
models and verbose CLI dumps stay in the explicitly chosen scratch directory.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import subprocess


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


class Values:
    """The bounded textual KV3 value subset emitted for skeletons/Hitbox sets."""
    token = re.compile(r'\s+|//[^\r\n]*|"(?:[^"\\]|\\.)*"|[-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?|[A-Za-z_][\w]*|[{}\[\]=,:]')

    def __init__(self, text):
        self.text, self.at, self.count = text, 0, 0

    def next(self):
        while True:
            found = self.token.match(self.text, self.at)
            if not found:
                raise ValueError(f'Unsupported/truncated KV3 token at {self.at}')
            self.at = found.end()
            token = found[0]
            if not token.isspace() and not token.startswith('//'):
                self.count += 1
                if self.count > 200000:
                    raise ValueError('KV3 value too large')
                return token

    def value(self, token=None, depth=0):
        if depth > 32:
            raise ValueError('KV3 nesting too deep')
        token = token or self.next()
        if token == '{':
            result = {}
            while (key := self.next()) != '}':
                if key == ',':
                    continue
                key = json.loads(key) if key.startswith('"') else key
                if key in result or self.next() != '=':
                    raise ValueError('Duplicate key or missing KV3 assignment')
                result[key] = self.value(depth=depth + 1)
            return result
        if token == '[':
            result = []
            while (item := self.next()) != ']':
                if item != ',':
                    result.append(self.value(item, depth + 1))
            return result
        if token.startswith('"'):
            return json.loads(token)
        if token == 'resource':
            if self.next() != ':':
                raise ValueError('Invalid typed resource string')
            name = self.next()
            if not name.startswith('"'):
                raise ValueError('Expected a typed resource string')
            return json.loads(name)
        if token in ('true', 'false'):
            return token == 'true'
        if token == 'null':
            return None
        try:
            value = float(token) if any(c in token for c in '.eE') else int(token)
        except ValueError as error:
            raise ValueError('Unsupported KV3 scalar: ' + token) from error
        if isinstance(value, float) and not math.isfinite(value):
            raise ValueError('Nonfinite KV3 number')
        return value


def property_values(text, name):
    return [Values(text[found.end():]).value()
            for found in re.finditer(r'(?m)^\s*' + re.escape(name) + r'\s*=\s*', text)]


def resource_chunks(text):
    starts = list(re.finditer(r'(?m)^\[\d+/\d+\] (agents/models/[^\r\n]+\.vmdl_c)\s*$', text))
    result = {}
    for i, found in enumerate(starts):
        name = found[1][:-2]
        if name in result:
            raise ValueError('Duplicate resource: ' + name)
        result[name] = text[found.end():starts[i + 1].start() if i + 1 < len(starts) else len(text)]
    if not result:
        raise ValueError('No resource records in CLI output')
    return result


def vector(value):
    if not isinstance(value, list) or len(value) != 3 or any(
            not isinstance(v, (int, float)) or isinstance(v, bool) or not math.isfinite(v) for v in value):
        raise ValueError('Expected a finite three-component vector')
    return value


def definitions(skeleton_text, mesh_text):
    skeletons = property_values(skeleton_text, 'm_modelSkeleton')
    if len(skeletons) != 1:
        raise ValueError('Expected one model skeleton')
    skeleton = skeletons[0]
    names, parents = skeleton['m_boneName'], skeleton['m_nParent']
    # VRF 20.1 HitboxSetSceneNode resolves these names case-insensitively;
    # authored Hitbox names use arm_upper_l while skeletons use arm_upper_L.
    if len(names) != len(parents) or len({name.lower() for name in names}) != len(names):
        raise ValueError('Ambiguous model skeleton')
    indices = {name.lower(): i for i, name in enumerate(names)}
    candidates = [entry for block in property_values(mesh_text, 'm_hitboxsets') for entry in block]
    sets = {}
    for entry in candidates:
        source = entry['value']
        name = entry['key']
        if source['m_name'] != name:
            raise ValueError('Hitbox set name mismatch')
        boxes = []
        for raw in source['m_HitBoxes']:
            bone_name, kind = raw['m_sBoneName'], raw['m_nShapeType']
            if bone_name.lower() not in indices or kind not in (0, 1, 2):
                raise ValueError('Unknown bone or Hitbox shape')
            a, b = vector(raw['m_vMinBounds']), vector(raw['m_vMaxBounds'])
            radius, translation = raw['m_flShapeRadius'], raw['m_bTranslationOnly']
            if not isinstance(translation, bool) or not math.isfinite(radius) or radius < 0:
                raise ValueError('Invalid Hitbox radius/translation rule')
            if kind == 0 and any(x > y for x, y in zip(a, b)):
                raise ValueError('Invalid box bounds')
            boxes.append({'id': raw['m_nHitBoxIndex'], 'name': raw['m_name'],
                          'bone': indices[bone_name.lower()], 'boneName': bone_name,
                          'shape': ('box', 'sphere', 'capsule')[kind], 'a': a, 'b': b,
                          'radius': radius, 'group': raw['m_nGroupId'],
                          'translationOnly': translation})
        boxes.sort(key=lambda box: box['id'])
        if not boxes or len({box['id'] for box in boxes}) != len(boxes):
            raise ValueError('Empty/duplicate Hitbox definitions')
        if name in sets and sets[name] != boxes:
            raise ValueError('Conflicting mesh Hitbox sets')
        sets[name] = boxes
    if not sets:
        raise ValueError('No Hitbox definitions')
    return [{'name': name, 'hitboxes': boxes} for name, boxes in sorted(sets.items())]


def mesh_sources(data_text, mesh_text, control_text):
    refs = property_values(data_text, 'm_refMeshes')
    if refs != [[]]:
        raise ValueError('External mesh dependencies require separate explicit extraction')
    controls = property_values(control_text, 'embedded_meshes')
    masks = property_values(data_text, 'm_refMeshGroupMasks')
    lods = property_values(data_text, 'm_refLODGroupMasks')
    groups = property_values(data_text, 'm_meshGroups')
    if any(len(value) != 1 for value in (controls, masks, lods, groups)):
        raise ValueError('Missing mesh/group source metadata')
    meshes = sorted(controls[0], key=lambda item: item['m_nDataBlock'])
    sets = property_values(mesh_text, 'm_hitboxsets')
    if len(meshes) != len(sets) or len(meshes) != len(masks[0]) or len(meshes) != len(lods[0]):
        raise ValueError('MDAT/control/group counts disagree')
    return {'groups': groups[0], 'meshes': [
        {'index': item['m_nMeshIndex'], 'name': item['m_Name'], 'block': item['m_nDataBlock'],
         'groupMask': masks[0][item['m_nMeshIndex']], 'lodMask': lods[0][item['m_nMeshIndex']],
         'sets': [{'name': group['key'], 'hitboxCount': len(group['value']['m_HitBoxes']),
                   'rawDefinitionSha256': hashlib.sha256(json.dumps(group['value']['m_HitBoxes'],
                       sort_keys=True, separators=(',', ':')).encode()).hexdigest()} for group in current]}
        for item, current in zip(meshes, sets)]}


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    data = json.dumps(value, ensure_ascii=False, indent=2, allow_nan=False) + '\n'
    if not path.exists() or path.read_text(encoding='utf-8') != data:
        path.write_text(data, encoding='utf-8')


def generate_header(manifest, output):
    """Compile tiny definitions into the native library, not raw game assets."""
    source = json.loads(manifest.read_text(encoding='utf-8'))
    lines = ['// Generated by tools/assets/cs2_hitboxes.py; do not edit.',
             '// Source manifest SHA256: ' + digest(manifest), '#pragma once',
             '#include "games/cs2/hitboxes/assets.hpp"', '#include <array>',
             'namespace mousebox::games::cs2::hitboxes::generated', '{',
             f'inline constexpr unsigned Build = {source["build"]};',
             'inline constexpr std::string_view Profile = ' + json.dumps(source['profile']) + ';']
    identities = sorted(source['definitions'])
    for i, identity in enumerate(identities):
        sets = source['definitions'][identity]
        if len(sets) != 1:
            raise ValueError('Multiple Hitbox sets require explicit runtime selection support')
        boxes = sets[0]['hitboxes']
        lines.append(f'inline constexpr std::array<Definition, {len(boxes)}> Shapes{i} = {{{{')
        for box in boxes:
            def vec(value):
                return '{' + ', '.join(str(v) for v in value) + '}'
            lines.append('    {' + f'{box["id"]}, {box["bone"]}, {box["group"]}, Shape::{box["shape"].title()}, '
                         + vec(box['a']) + ', ' + vec(box['b']) + ', ' + str(box['radius'])
                         + ', ' + str(box['translationOnly']).lower() + '},')
        lines.append('}};')
    lines.append(f'inline constexpr std::array<ModelAsset, {len(source["models"])}> Models = {{{{')
    for model in source['models']:
        i = identities.index(model['definition'])
        set_name = source['definitions'][model['definition']][0]['name']
        lines.append('    {' + ', '.join(json.dumps(s) for s in (model['name'], model['resourceSha256'], set_name))
                     + f', Shapes{i}' + '},')
    lines.extend(['}};', '} // namespace mousebox::games::cs2::hitboxes::generated', ''])
    output.parent.mkdir(parents=True, exist_ok=True)
    content = '\n'.join(lines)
    if not output.exists() or output.read_text(encoding='utf-8') != content:
        output.write_text(content, encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tool', type=Path, required=True)
    parser.add_argument('--tool-sha256', required=True)
    parser.add_argument('--vpk', type=Path, required=True)
    parser.add_argument('--models', type=Path, required=True, help='Reviewed build/profile/models JSON')
    parser.add_argument('--scratch', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if digest(args.tool) != args.tool_sha256.lower():
        raise ValueError('Resource CLI differs from the pinned executable')
    reviewed = json.loads(args.models.read_text(encoding='utf-8-sig'))
    models = reviewed['models']
    if not models or len(set(models)) != len(models) or any(
            not re.fullmatch(r'agents/models/[a-z0-9_/]+\.vmdl', name) for name in models):
        raise ValueError('Explicit reviewed model list is required')
    args.scratch.mkdir(parents=True, exist_ok=True)
    vpk_hash = digest(args.vpk)
    base = [str(args.tool), '-i', str(args.vpk), '--vpk_filepath', 'agents/models/', '--vpk_extensions', 'vmdl_c']
    for block in ('DATA', 'MDAT', 'CTRL'):
        with (args.scratch / (block + '.txt')).open('w', encoding='utf-8') as output:
            subprocess.run(base + ['--block', block], stdout=output, stderr=subprocess.STDOUT,
                           check=True, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
    raw_assets = args.scratch / 'assets'
    with (args.scratch / 'extract.log').open('w', encoding='utf-8') as output:
        subprocess.run(base + ['-o', str(raw_assets)], stdout=output, stderr=subprocess.STDOUT,
                       check=True, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
    data = resource_chunks((args.scratch / 'DATA.txt').read_text(encoding='utf-8-sig'))
    mesh = resource_chunks((args.scratch / 'MDAT.txt').read_text(encoding='utf-8-sig'))
    controls = resource_chunks((args.scratch / 'CTRL.txt').read_text(encoding='utf-8-sig'))
    profiles, entries = {}, []
    for name in sorted(models):
        sets = definitions(data[name], mesh[name])
        identity = hashlib.sha256(json.dumps(sets, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
        profiles[identity] = sets
        entries.append({'name': name, 'resourceSha256': digest(raw_assets / (name + '_c')),
                        'definition': identity, 'meshSources': mesh_sources(data[name], mesh[name], controls[name])})
    if digest(args.vpk) != vpk_hash:
        raise ValueError('VPK directory changed during extraction')
    result = {'schema': 1, 'game': 'cs2', 'build': reviewed['build'], 'profile': reviewed['profile'],
              'provenance': {'tool': 'Source 2 Viewer CLI 20.1', 'toolSha256': args.tool_sha256.lower(),
                             'vpkDirectorySha256': vpk_hash, 'reviewedModelsSha256': digest(args.models)},
              'models': entries, 'definitions': profiles}
    write_json(args.output, result)
    required = {box['bone'] for sets in profiles.values() for item in sets for box in item['hitboxes']}
    print(json.dumps({'models': len(entries), 'uniqueDefinitions': len(profiles),
                      'requiredBones': sorted(required), 'bytes': args.output.stat().st_size}))


if __name__ == '__main__':
    import sys
    if len(sys.argv) == 4 and sys.argv[1] == '--generate-header':
        generate_header(Path(sys.argv[2]), Path(sys.argv[3]))
    else:
        main()

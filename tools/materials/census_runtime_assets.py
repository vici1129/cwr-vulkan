"""Read-only reinspection of a placed-model inventory through gameplay's ODOL adapter.

The old inventory is immutable. An explicit enhanced-identity interval can replay
its older source policy for comparison; every other prerequisite must still agree.
Original files stay in the inventory cache, and raw output stays in --cache.
"""
import argparse
import collections
import concurrent.futures
import json
import subprocess
from pathlib import Path

from inventory_original_assets import REASONS, ROOT, parse_model, save, sha


def section_signature(section, replay=()):
    s = dict(section)
    if s['source'] in replay:
        if s['declared']:
            raise ValueError('Cannot replay a source-declared material as an enhanced identity')
        s['unknownSource'] += s['eligible']
        s['eligible'], s['source'] = 0, 0
    return (s['texture'], s['flags'], s['vertices'], s['invalidNormals'],
            tuple(sorted(s['materialIndices'].items())), s['source'], s['opaque'],
            s['declared'], *(s[r] for r in REASONS))


def compare_models(before, after, replay=()):
    differences = []
    old = {lod['index']: lod for lod in before['visualLODs']}
    new = {lod['index']: lod for lod in after['visualLODs']}
    if old.keys() != new.keys():
        differences.append({'kind': 'visual LOD indices'})
    for index in sorted(old.keys() & new.keys()):
        a, b = old[index], new[index]
        for key in ('resolution', 'vertices', 'normals', 'invalidNormals',
                    'authoredVertices', 'landHintVertices', 'lightModes'):
            if a[key] != b[key]:
                differences.append({'lod': index, 'kind': key, 'old': a[key], 'runtime': b[key]})
        # Allocation/sort paths can reorder sections. Ordinals are not asset IDs.
        if collections.Counter(section_signature(s) for s in a['sections']) != collections.Counter(
                section_signature(s, replay) for s in b['sections']):
            differences.append({'lod': index, 'kind': 'complete section prerequisite multiset'})
    return differences


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inventory', type=Path, required=True)
    parser.add_argument('--library', type=Path, required=True)
    parser.add_argument('--config', type=Path, required=True)
    parser.add_argument('--tool', type=Path, required=True)
    parser.add_argument('--cache', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--workers', type=int, default=4)
    parser.add_argument('--replay-enhanced-from', type=int)
    parser.add_argument('--replay-enhanced-through', type=int)
    args = parser.parse_args()
    for key in ('inventory', 'library', 'config', 'tool', 'cache', 'output'):
        setattr(args, key, getattr(args, key).resolve())
    for dest in (args.cache, args.output):
        if dest.is_relative_to(args.library) or dest.is_relative_to(ROOT/'game-local') or dest.is_relative_to(ROOT/'private'):
            parser.error('cache/output must not be inside original inputs or private repositories')
    if not 1 <= args.workers <= 8:
        parser.error('workers must be between one and eight')
    first, last = args.replay_enhanced_from, args.replay_enhanced_through
    if (first is None) != (last is None) or (first is not None and not 128 <= first <= last < 256):
        parser.error('supply both replay interval endpoints, inside the enhanced identity range 128..255')
    replay = range(first, last+1) if first is not None else ()
    inventory = json.loads(args.inventory.read_text())
    placed = [m for m in inventory['models'] if 'inspection' in m]
    raw = args.cache/'models'
    raw.mkdir(parents=True, exist_ok=True)

    def one(model):
        path = model['path']
        source = args.library/path
        if sha(source) not in {s['sha256'] for s in model['sources']}:
            raise ValueError('Extracted model differs from original inventory: '+path)
        result = subprocess.run([str(args.tool), 'model', 'inspect', str(source),
                                 '--runtime-loader', '--textures', '--lighting', '--sections', '--qualify',
                                 '--texroot', str(args.library), '--material-config', str(args.config)],
                                cwd=ROOT, capture_output=True, text=True, encoding='utf-8',
                                errors='replace', timeout=120)
        if result.returncode:
            raise RuntimeError(path+': '+result.stderr[-1200:])
        file = raw/(path.replace('/', '_')+'.txt')
        file.write_text(result.stdout, encoding='utf-8')
        parsed = parse_model(result.stdout)
        if parsed['format'] != 'ODOL' or 'loader=runtime-adapter' not in result.stdout:
            raise ValueError('Runtime ODOL adapter was not used: '+path)
        return path, parsed

    models = {}
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as pool:
        for i, (path, model) in enumerate(pool.map(one, placed), 1):
            models[path] = model
            if i % 100 == 0:
                print('runtime model inspections', i, flush=True)
    differences = []
    for model in placed:
        for difference in compare_models(model['inspection'], models[model['path']], replay):
            differences.append({'model': model['path'], **difference})
    summary = {'models': len(models), 'visualLODs': sum(len(m['visualLODs']) for m in models.values()),
               'highestDetailTriangleBuckets': dict(sum((collections.Counter(min(m['visualLODs'],
                   key=lambda lod: (lod['resolution'], lod['index']))['triangles'])
                   for m in models.values()), collections.Counter()))}
    save(args.output, {'schema': 'cwrr-runtime-source-census/2', 'toolSHA256': sha(args.tool),
        'inventorySHA256': sha(args.inventory), 'configSHA256': sha(args.config),
        'loader': 'ModelCache / ODOL ShapeAdapter, as ordinary ShapeBank gameplay. Immutable model prerequisites, not live instance or receiver coverage.',
        'comparison': 'All visual LOD indices, resolutions, normals, vertex hints, complete section multisets and rejection buckets. Section ordinals are deliberately not stable IDs. Original inventory predates UV-bound diagnostics; UV equality is separately checked in batch audits.',
        'replayedEnhancedIdentityInterval': [first, last] if first is not None else [],
        'summary': summary, 'legacyInventoryDifferences': differences, 'models': models})
    print(summary)
    print('legacy/runtime prerequisite differences after explicit source-policy replay:', len(differences))
    if differences:
        raise SystemExit(1)


if __name__ == '__main__':
    main()

"""Read-only reinspection of a placed-model inventory through gameplay's ODOL adapter.

The old inventory is immutable. An explicit enhanced-identity interval can replay
its older source policy for comparison; every other prerequisite must still agree.
Original files stay in the inventory cache, and raw output stays in --cache.
"""
import argparse
import collections
import concurrent.futures
import json
import hashlib
import re
import subprocess
from pathlib import Path

from inventory_original_assets import REASONS, ROOT, parse_model, save, sha


def historical_interpretation(raw):
    """Repair an exactly verified legacy record, not current native output.

    The old optimized loader also printed corrupt resolution floats for one
    model. Keep its recorded declared indices/prerequisites for comparison,
    explicitly report those floats, and never accept them from a current tool.
    A missing qualification may only be removed when native output proves that
    a nonvisual level occupied that old declared visual slot.
    """
    parsed = parse_model(raw, legacy_indexed=True)
    qualified = {int(i):float(r) for i,r in re.findall(r'^Qualification LOD (\d+) res=(\S+)',raw,re.M)}
    native = {int(i):float(r) for i,r in re.findall(r'^  LOD (\d+) res=(\S+) vertices=',raw,re.M)}
    retained, excluded, diagnostics = [], [], []
    for lod in parsed['visualLODs']:
        index=lod['index']
        if index not in qualified:
            if native.get(index,0)<10000 or lod['sections']:
                raise ValueError('Unexplained missing historical visual qualification')
            excluded.append({k:lod[k] for k in ('index','resolution','textures','pointsCount','facesCount')})
        else:
            if native.get(index)!=qualified[index]:
                raise ValueError('Historical native and qualification resolutions disagree')
            retained.append(lod)
            if qualified[index]!=lod['resolution']:
                diagnostics.append({'index':index,'declaredResolution':lod['resolution'],
                    'historicalNativeResolution':qualified[index],
                    'reason':'Malformed legacy optimized-loader resolution print; declared identity retained, complete prerequisites require fresh native comparison.'})
    return dict(parsed,visualLODs=retained,declaredVisualLODCount=len(parsed['visualLODs']),
                excludedVisualLODs=excluded,historicalResolutionDiagnostics=diagnostics)


def historical_baseline(model, reports):
    """Verify the recorded legacy interpretation, then repair only its parsing."""
    path = reports/(hashlib.sha256(model['path'].encode()).hexdigest()[:20]+'.txt')
    raw = path.read_text(encoding='utf-8')
    if parse_model(raw, legacy_indexed=True) != model['inspection']:
        raise ValueError('Historical report does not reproduce immutable inventory: '+model['path'])
    return historical_interpretation(raw), sha(path)


def baseline_comparison_view(before, after, all_source_lods):
    if not all_source_lods:return after, []
    retained = {l['index'] for l in before['visualLODs']}
    excluded = {l['index']:l for l in before.get('excludedVisualLODs',[])}
    expected = retained | excluded.keys()
    if {l['index'] for l in after['visualLODs']} != expected:
        raise ValueError('Source-LOD inspection does not cover exactly declared original visual levels')
    extra = []
    for lod in after['visualLODs']:
        if lod['index'] in excluded:
            original = excluded[lod['index']]
            if lod['resolution'] != original['resolution']:
                raise ValueError('Excluded source LOD changed resolution')
            extra.append({'index':lod['index'],'resolution':lod['resolution'],
                          'reason':'Not retained by original native loader; source-only prerequisites inspected separately'})
    return dict(after,visualLODs=[l for l in after['visualLODs'] if l['index'] in retained]), extra


def section_signature(section, replay=(), regional_replay=()):
    s = dict(section)
    regions = dict(s.get('regionalSources', {}))
    for identity in list(regions):
        if int(identity) in regional_replay:
            if s['source'] or s['declared'] or not 0 < regions[identity] <= s['eligible']:
                raise ValueError('Regional replay needs a bounded additional identity, not whole-source proof')
            s['eligible'] -= regions[identity]
            s['unknownSource'] += regions.pop(identity)
    if s['source'] in replay:
        if s['declared']:
            raise ValueError('Cannot replay a source-declared material as an enhanced identity')
        s['unknownSource'] += s['eligible']
        s['eligible'], s['source'] = 0, 0
    return (s['texture'], s['flags'], s['vertices'], s['invalidNormals'],
            tuple(sorted(s['materialIndices'].items())), s['source'], s['opaque'],
            s['declared'], *(s[r] for r in REASONS), tuple(sorted(regions.items())))


def compare_models(before, after, replay=(), regional_replay=()):
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
                section_signature(s, replay, regional_replay) for s in b['sections']):
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
    parser.add_argument('--replay-regional-identities', nargs='+', type=int, default=[])
    parser.add_argument('--legacy-report-cache',type=Path,
        help='Verify/reparse original historical reports; required to correct removed-LOD index artifacts')
    parser.add_argument('--all-source-lods',action='store_true',
        help='Diagnostic unoptimized source census, not actual gameplay-retained LOD coverage')
    args = parser.parse_args()
    for key in ('inventory', 'library', 'config', 'tool', 'cache', 'output'):
        setattr(args, key, getattr(args, key).resolve())
    if args.legacy_report_cache:args.legacy_report_cache=args.legacy_report_cache.resolve()
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
        options=['--all-source-lods'] if args.all_source_lods else []
        result = subprocess.run([str(args.tool), 'model', 'inspect', str(source),
                                 '--runtime-loader', '--textures', '--lighting', '--sections', '--qualify',
                                 '--texroot', str(args.library), '--material-config', str(args.config),*options],
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
    differences, corrections, uncompared, historical_resolutions = [], {}, [], {}
    for model in placed:
        before = model['inspection']
        if args.legacy_report_cache:
            before,digest = historical_baseline(model,args.legacy_report_cache)
            if before['historicalResolutionDiagnostics']:
                historical_resolutions[model['path']]={'diagnostics':before['historicalResolutionDiagnostics'],
                    'verifiedHistoricalReportSHA256':digest}
            if before.get('excludedVisualLODs'):
                corrections[model['path']]={'excludedSourceLODs':before['excludedVisualLODs'],'verifiedHistoricalReportSHA256':digest,
                    'reason':'Legacy parser copied later nonvisual native metadata into removed declared visual slots. Original inventory remains unchanged.'}
        after,extra=baseline_comparison_view(before,models[model['path']],args.all_source_lods)
        uncompared.extend({'model':model['path'],**r} for r in extra)
        for difference in compare_models(before, after, replay, args.replay_regional_identities):
            differences.append({'model': model['path'], **difference})
    summary = {'models': len(models), 'visualLODs': sum(len(m['visualLODs']) for m in models.values()),
               'declaredVisualLODs':sum(m.get('declaredVisualLODCount',len(m['visualLODs'])) for m in models.values()),
               'excludedFromInspection':sum(len(m.get('excludedVisualLODs',[])) for m in models.values()),
               'highestDetailTriangleBuckets': dict(sum((collections.Counter(min(m['visualLODs'],
                   key=lambda lod: (lod['resolution'], lod['index']))['triangles'])
                   for m in models.values()), collections.Counter()))}
    save(args.output, {'schema': 'cwrr-runtime-source-census/3', 'toolSHA256': sha(args.tool),
        'inventorySHA256': sha(args.inventory), 'configSHA256': sha(args.config),
        'loader': 'ModelCache / ODOL ShapeAdapter; '+('DIAGNOSTIC all source LODs before optimization, NOT gameplay-retained coverage.' if args.all_source_lods else 'ordinary ShapeBank optimization under inspection defaults; live game settings may retain/select different levels.')+' Immutable prerequisites, not live reception.',
        'comparison': 'All visual LOD indices, resolutions, normals, vertex hints, complete section multisets and rejection buckets. Section ordinals are deliberately not stable IDs. Original inventory predates UV-bound diagnostics; UV equality is separately checked in batch audits.',
        'replayedEnhancedIdentityInterval': [first, last] if first is not None else [],
        'replayedRegionalIdentities': args.replay_regional_identities,
        'historicalReportsVerified':len(placed) if args.legacy_report_cache else 0,
        'legacyLODIndexCorrections':corrections,'sourceOnlyWithoutRetainedBaseline':uncompared,
        'historicalResolutionDiagnostics':historical_resolutions,
        'summary': summary, 'legacyInventoryDifferences': differences, 'models': models})
    print(summary)
    print('legacy/runtime prerequisite differences after explicit source-policy replay:', len(differences))
    if differences:
        raise SystemExit(1)


if __name__ == '__main__':
    main()

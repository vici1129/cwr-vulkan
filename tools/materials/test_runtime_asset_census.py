"""Regression checks for order-independent, source-policy-only census replay."""
import copy
import unittest
from census_runtime_assets import compare_models, section_signature, baseline_comparison_view, historical_interpretation


class CensusComparisonTests(unittest.TestCase):
    def test_historical_resolution_corruption_is_reported_not_accepted_as_current(self):
        text='''Format: ODOL
LOD 0 (Resolution: 2)
  Points: 3
  Faces: 1
LOD 1 (Resolution: 40)
  Points: 16
  Faces: 6
Native lighting (source hints)
Qualification LOD 0 res=2e-41
  LOD 0 res=2e-41 vertices=3 normals=3 invalidNormals=0 userMaterialVertices=0 landHintVertices=0 lightModes=0:3
  LOD 1 res=1e+13 vertices=328 normals=328 invalidNormals=0 userMaterialVertices=0 landHintVertices=0 lightModes=0:328
'''
        result=historical_interpretation(text)
        self.assertEqual(len(result['visualLODs']),1)
        self.assertEqual(result['visualLODs'][0]['resolution'],2)
        self.assertEqual(result['excludedVisualLODs'][0]['resolution'],40)
        self.assertEqual(result['historicalResolutionDiagnostics'][0]['historicalNativeResolution'],2e-41)
        with self.assertRaises(ValueError):historical_interpretation(text.replace('LOD 1 res=1e+13','LOD 1 res=40'))
        with self.assertRaises(ValueError):historical_interpretation(text.replace('LOD 0 res=2e-41 vertices','LOD 0 res=2 vertices'))

    def test_source_only_extra_levels_are_explicit_not_runtime_coverage(self):
        before={'visualLODs':[{'index':0,'resolution':1}],
                'excludedVisualLODs':[{'index':1,'resolution':40}]}
        after={'visualLODs':[{'index':0,'resolution':1},{'index':1,'resolution':40}]}
        view,extra=baseline_comparison_view(before,after,True)
        self.assertEqual([l['index'] for l in view['visualLODs']],[0])
        self.assertEqual(extra[0]['index'],1)
        self.assertEqual(extra[0]['resolution'],40)
        self.assertEqual(baseline_comparison_view(before,after,False),(after,[]))
        with self.assertRaises(ValueError):baseline_comparison_view(before,{'visualLODs':[{'index':0,'resolution':1}]},True)
        with self.assertRaises(ValueError):baseline_comparison_view(before,{'visualLODs':[{'index':0,'resolution':1},{'index':1,'resolution':41}]},True)

    def setUp(self):
        section = dict(index=0, texture='data/wall.pac', flags=16384, vertices=3,
                       invalidNormals=0, materialIndices={'0': 3}, source=0, opaque=1,
                       declared=0, eligible=0, unknownSource=1, renderBehavior=0,
                       authoredLighting=0, geometry=0)
        self.before = {'visualLODs': [dict(index=0, resolution=.5, vertices=6, normals=6,
            invalidNormals=0, authoredVertices=0, landHintVertices=0, lightModes={'0': 6},
            sections=[section, dict(section, index=1, texture='data/other.pac')])]}

    def test_section_order_and_ordinal_do_not_authorize_or_reject(self):
        after = copy.deepcopy(self.before)
        after['visualLODs'][0]['sections'].reverse()
        for i, s in enumerate(after['visualLODs'][0]['sections']):
            s['index'] = i
        self.assertEqual(compare_models(self.before, after), [])

    def test_only_explicit_source_interval_can_replay(self):
        after = copy.deepcopy(self.before)
        s = after['visualLODs'][0]['sections'][0]
        s.update(source=141, eligible=1, unknownSource=0)
        self.assertTrue(compare_models(self.before, after))
        self.assertEqual(compare_models(self.before, after, range(130, 152)), [])
        self.assertTrue(compare_models(self.before, after, range(130, 141)))

    def test_authored_render_and_geometry_rejections_cannot_be_erased(self):
        for key in ('authoredLighting', 'renderBehavior', 'geometry'):
            after = copy.deepcopy(self.before)
            after['visualLODs'][0]['sections'][0].update(source=141, unknownSource=0, **{key: 1})
            self.assertTrue(compare_models(self.before, after, range(130, 152)))

    def test_texture_replay_preserves_canonical_users_of_same_identity(self):
        before=copy.deepcopy(self.before)
        canonical=before['visualLODs'][0]['sections'][1]
        canonical.update(source=86,eligible=1,unknownSource=0)
        after=copy.deepcopy(before)
        after['visualLODs'][0]['sections'][0].update(source=86,eligible=1,unknownSource=0)
        pair=[('data/wall.pac',86)]
        self.assertTrue(compare_models(before,after))
        self.assertEqual(compare_models(before,after,texture_replay=pair),[])
        self.assertTrue(compare_models(before,after,replay=[86]))
        self.assertTrue(compare_models(before,after,texture_replay=[('data/other.pac',86)]))
        self.assertTrue(compare_models(before,after,texture_replay=[('data/wall.pac',85)]))
        for key in ('authoredLighting','renderBehavior','geometry'):
            broken=copy.deepcopy(after)
            broken['visualLODs'][0]['sections'][0].update(eligible=0,**{key:1})
            self.assertTrue(compare_models(before,broken,texture_replay=pair))
        declared=dict(after['visualLODs'][0]['sections'][0],declared=1)
        with self.assertRaises(ValueError):section_signature(declared,texture_replay=pair)

    def test_declared_source_cannot_be_replayed(self):
        s = dict(self.before['visualLODs'][0]['sections'][0], source=141, declared=1)
        with self.assertRaises(ValueError):
            section_signature(s, range(130, 152))

    def test_regional_proof_is_not_a_whole_image_identity(self):
        after = copy.deepcopy(self.before)
        s = after['visualLODs'][0]['sections'][0]
        s.update(regionalSources={'512': 1}, eligible=1, unknownSource=0)
        self.assertTrue(compare_models(self.before, after))
        self.assertEqual(compare_models(self.before, after, regional_replay=[512]), [])
        s['source'] = 128
        with self.assertRaises(ValueError):
            section_signature(s, regional_replay=[512])

    def test_regional_replay_cannot_erase_other_failures(self):
        after = copy.deepcopy(self.before)
        s = after['visualLODs'][0]['sections'][0]
        s.update(regionalSources={'512': 1}, eligible=0, unknownSource=0, authoredLighting=1)
        with self.assertRaises(ValueError):
            section_signature(s, regional_replay=[512])

    def test_vertex_hints_and_section_duplicates_are_counted(self):
        after = copy.deepcopy(self.before)
        after['visualLODs'][0]['landHintVertices'] = 1
        self.assertTrue(compare_models(self.before, after))
        after = copy.deepcopy(self.before)
        after['visualLODs'][0]['sections'].append(copy.deepcopy(after['visualLODs'][0]['sections'][0]))
        self.assertTrue(compare_models(self.before, after))


if __name__ == '__main__':
    unittest.main()

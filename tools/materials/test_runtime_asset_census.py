"""Regression checks for order-independent, source-policy-only census replay."""
import copy
import unittest
from census_runtime_assets import compare_models, section_signature


class CensusComparisonTests(unittest.TestCase):
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

    def test_declared_source_cannot_be_replayed(self):
        s = dict(self.before['visualLODs'][0]['sections'][0], source=141, declared=1)
        with self.assertRaises(ValueError):
            section_signature(s, range(130, 152))

    def test_vertex_hints_and_section_duplicates_are_counted(self):
        after = copy.deepcopy(self.before)
        after['visualLODs'][0]['landHintVertices'] = 1
        self.assertTrue(compare_models(self.before, after))
        after = copy.deepcopy(self.before)
        after['visualLODs'][0]['sections'].append(copy.deepcopy(after['visualLODs'][0]['sections'][0]))
        self.assertTrue(compare_models(self.before, after))


if __name__ == '__main__':
    unittest.main()

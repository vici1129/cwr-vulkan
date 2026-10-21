"""Small parser/namespace regression tests; no game assets or renderer needed."""
import unittest
import collections
import json
from inventory_original_assets import ROOT, REASONS, norm, parse_model


class InventoryTests(unittest.TestCase):
    def test_full_namespaces_are_not_basename_deduplicated(self):
        self.assertEqual(norm(r"\DATA3D\House.P3D"), "data3d/house.p3d")
        self.assertNotEqual(norm("data/wall.pac"), norm("o/wall.pac"))
        self.assertNotEqual(norm("data/wall.pac"), norm("data/wall.paa"))
        self.assertNotEqual(norm("@Templates/a.pbo/wall.pac"), norm("data/wall.pac"))
        with self.assertRaises(ValueError):
            norm("data/../other/wall.pac")

    def test_visual_lods_and_native_section_evidence(self):
        text = """File: fixture.p3d
Format: ODOL
Version: 7
LOD 0 (Resolution: 1)
  Points: 3
  Faces: 1
  Textures: 1
  Selections: 0
  Texture List:
    [0] DATA\\Wall.pac
LOD 1 (Resolution: 1e+13)
  Points: 3
  Faces: 1
  Textures: 0
  Selections: 0
Native lighting (source hints; not PBR/GI eligibility)
Qualification LOD 0 res=1
  section=0 texture=data\\wall.pac source=0 opaque=1 declared=0 eligible=0 unknownSource=1 renderBehavior=0 authoredLighting=0 geometry=0 reception=unverified(no ambient capture)
  LOD 0 res=1 vertices=3 normals=3 invalidNormals=0 userMaterialVertices=0 landHintVertices=0 lightModes=0:3
    native section 0 tex=data\\wall.pac flags=0x8000 vertices=3 invalidVertexNormals=0 materialIndices=0:2 201:1
  LOD 1 res=1e+13 vertices=3 normals=3 invalidNormals=0 userMaterialVertices=0 landHintVertices=0 lightModes=0:3
"""
        model = parse_model(text)
        self.assertEqual(model["nonvisualLODCount"], 1)
        self.assertEqual(len(model["visualLODs"]), 1)
        lod = model["visualLODs"][0]
        self.assertEqual(lod["textures"], ["data/wall.pac"])
        self.assertEqual(lod["triangles"]["unknownSource"], 1)
        self.assertEqual(lod["lightModes"], {"0": 3})
        self.assertEqual(lod["sections"][0]["materialIndices"], {"0": 2, "201": 1})
        self.assertEqual(lod["sections"][0]["flags"], 0x8000)


class SnapshotIntegrityTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        folder = ROOT / "docs/evidence/original-asset-inventory"
        cls.data = json.loads((folder / "inventory.json").read_text())
        cls.backlog = json.loads((folder / "backlog.json").read_text())

    def test_library_provenance_and_namespaces(self):
        for kind in ("models", "textures"):
            paths = [row["path"] for row in self.data[kind]]
            self.assertEqual(len(paths), len(set(paths)))
            for row in self.data[kind]:
                self.assertEqual(norm(row["path"]), row["path"])
                self.assertTrue(row["sources"])
                for source in row["sources"]:
                    self.assertEqual(len(source["sha256"]), 64)
                    if "archive" in source:
                        archive = self.data["archives"][source["archive"]]
                        self.assertEqual(norm(archive["namespace"] + "/" + source["entry"]), row["path"])
        self.assertEqual(len(self.data["models"]), 1400)
        self.assertEqual(len(self.data["textures"]), 7266)

    def test_world_counts_and_complete_model_inspections(self):
        referenced = set()
        models = {m["path"]: m for m in self.data["models"]}
        for name, world in self.data["worlds"].items():
            self.assertEqual(sum(world["models"].values()), world["instances"])
            for path, count in world["models"].items():
                referenced.add(path)
                self.assertEqual(models[path]["placements"][name], count)
        inspected = {m["path"] for m in models.values() if "inspection" in m}
        self.assertEqual(referenced, inspected)
        self.assertEqual(len(inspected), 726)
        self.assertFalse(self.data["missingWorldModels"])
        self.assertEqual(sum(w["instances"] for w in self.data["worlds"].values()), 407848)

    def test_texture_reverse_index_is_actual_nonempty_sections(self):
        expected = collections.defaultdict(set)
        for model in self.data["models"]:
            for lod in model.get("inspection", {}).get("visualLODs", []):
                for section in lod["sections"]:
                    key = section["texture"]
                    if key.endswith((".paa", ".pac")) and sum(section[r] for r in REASONS):
                        expected[key].add(model["path"])
        actual = {t["path"]: set(t["worldModels"]) for t in self.data["textures"] if t.get("worldModels")}
        self.assertEqual(dict(expected), actual)
        terrain = {t for w in self.data["worlds"].values() for t in w["terrainTextureDeclarations"]}
        self.assertEqual(terrain, {t["path"] for t in self.data["textures"] if t.get("terrainWorlds")})
        self.assertEqual(len(set(expected) | terrain), 2553)
        self.assertFalse(self.data["missingWorldTextures"])
        self.assertFalse(any("decodeError" in t for t in self.data["textures"]))

    def test_geometry_partitions_and_lod_units(self):
        total = collections.Counter()
        lod_count = 0
        for model in self.data["models"]:
            lods = model.get("inspection", {}).get("visualLODs", [])
            lod_count += len(lods)
            for lod in lods:
                self.assertLess(lod["resolution"], 10000)
                for reason in REASONS:
                    self.assertEqual(sum(s[reason] for s in lod["sections"]), lod["triangles"][reason])
                for section in lod["sections"]:
                    self.assertIn("flags", section)
                    self.assertIn("materialIndices", section)
            if lods:
                total.update(min(lods, key=lambda l: (l["resolution"], l["index"]))["triangles"])
        self.assertEqual(lod_count, 1711)
        self.assertEqual(dict(total), self.data["summary"]["highestDetailTriangles"])
        self.assertEqual(sum(total.values()), 215109)
        self.assertEqual(len(self.data["summary"]["nativePreviewFaceDiscrepancies"]), 3)

    def test_backlog_usage_is_derived_not_estimated(self):
        models = {m["path"]: m for m in self.data["models"]}
        rows = self.backlog["candidates"]
        self.assertEqual([r["rank"] for r in rows], list(range(1, 31)))
        for row in rows:
            usage = collections.Counter()
            for path in row["models"]:
                usage.update(models[path]["placements"])
            self.assertEqual(dict(usage), row["placements"])
            self.assertEqual(sum(usage.values()), row["totalInstances"])
        self.assertEqual(len({p for r in rows for p in r["models"]}), 78)

    def test_inputs_and_config_audit_are_explicit(self):
        self.assertEqual(self.data["preservation"]["inputsChecked"], 270)
        self.assertEqual(self.data["preservation"]["changed"], 0)
        self.assertEqual(len(self.data["addonMaterialAudit"]), 32)
        self.assertFalse(any(a["worldSectionTextureOverlap"] or a["materialClasses"]
                             for a in self.data["addonMaterialAudit"]))
        self.assertEqual(self.data["coverageContract"]["reception"].split(":")[0], "Unverified")
        self.assertEqual(len(self.data["existingCatalogDeclarations"]), 4)


if __name__ == "__main__":
    unittest.main()

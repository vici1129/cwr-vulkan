"""Read-only stock inventory; orchestrates existing PoseidonTools, never the renderer.

Run from the repository root. Extracted original copies and raw command output
stay in --cache; only metadata is written to --output. No basename resolution.
"""
import argparse
import collections
import concurrent.futures
import hashlib
import json
import re
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ISLANDS = {"malden": "abel", "everon": "eden", "kolgujev": "cain", "nogova": "noe"}
KINDS = {".p3d": "models", ".pac": "textures", ".paa": "textures"}
REASONS = ["eligible", "unknownSource", "renderBehavior", "authoredLighting", "geometry"]


def norm(value):
    parts = value.replace("\\", "/").lower().strip("/").split("/")
    if ".." in parts:
        raise ValueError("unsafe asset path: " + value)
    return "/".join(p for p in parts if p and p != ".")


def sha(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def save(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, separators=(",", ":"), ensure_ascii=True) + "\n", encoding="utf-8")


def command(args, *words, timeout=120):
    result = subprocess.run([str(args.tool), *map(str, words)], cwd=ROOT,
                            capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=timeout)
    if result.returncode:
        raise RuntimeError(f"{words}: exit {result.returncode}: {result.stderr[-1200:]}")
    return result.stdout


def scan(args):
    assets = {kind: {} for kind in ("models", "textures")}
    archives, preserved = [], {}
    for pbo in sorted(args.game.rglob("*")):
        if pbo.suffix.lower() != ".pbo" or not pbo.is_file():
            continue
        relative = pbo.relative_to(args.game).as_posix()
        # BankList::Load mounts DTA and AddOns by bank name. Mission containers
        # are not global resource namespaces and retain their container scope.
        namespace = norm(pbo.stem) if pbo.parent.name.lower() in ("dta", "addons") else "@" + norm(relative)
        listing = command(args, "pbo", "list", pbo)
        entries = []
        for line in listing.splitlines():
            match = re.match(r"^(.*?)\s{2,}[\d.]+ (?:B|KB|MB|GB)\s", line)
            if match:
                entries.append(match[1])
        total = re.search(r"^(\d+) files,", listing, re.M)
        assert total and len(entries) == int(total[1]), relative
        index = len(archives)
        archive = {"path": relative, "namespace": namespace, "sha256": sha(pbo), "entries": len(entries)}
        preserved[relative] = archive["sha256"]
        raw = args.cache / "archives" / str(index)
        for extension in KINDS:
            selected = [entry for entry in entries if Path(entry).suffix.lower() == extension]
            if selected:
                command(args, "pbo", "extract", pbo, raw, "--filter", extension)
            for entry in selected:
                key = norm(namespace + "/" + entry)
                source = raw / entry.replace("\\", "/")
                assert source.is_file(), source
                record = assets[KINDS[extension]].setdefault(key, {"path": key, "sources": []})
                record["sources"].append({"archive": index, "entry": entry, "sha256": sha(source)})
                dest = args.cache / "library" / key
                if not dest.is_file():
                    dest.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copyfile(source, dest)
        archive.update({kind: sum(Path(e).suffix.lower() in ([".p3d"] if kind == "models" else [".pac", ".paa"]) for e in entries)
                        for kind in assets})
        archives.append(archive)
        print(f"archive {index + 1}: {relative}: {archive['models']} models, {archive['textures']} textures", flush=True)
    for source in sorted(args.game.rglob("*")):
        if not source.is_file() or source.suffix.lower() not in KINDS:
            continue
        key = norm(source.relative_to(args.game).as_posix())
        digest = sha(source)
        preserved[source.relative_to(args.game).as_posix()] = digest
        record = assets[KINDS[source.suffix.lower()]].setdefault(key, {"path": key, "sources": []})
        record["sources"].append({"loose": source.relative_to(args.game).as_posix(), "sha256": digest})
        dest = args.cache / "library" / key
        if not dest.is_file():
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, dest)
    worlds = {}
    for name, stem in ISLANDS.items():
        wrp = args.game / "Worlds" / (stem + ".wrp") if stem != "noe" else args.cache / "noe" / "noe.wrp"
        if stem == "noe":
            command(args, "pbo", "extract", args.game / "AddOns" / "Noe.pbo", wrp.parent, "--filter", ".wrp")
        else:
            preserved[wrp.relative_to(args.game).as_posix()] = sha(wrp)
        objects = command(args, "terrain", "objects", wrp)
        info = command(args, "terrain", "inspect", wrp)
        textures = command(args, "terrain", "textures", wrp)
        (args.cache / (stem + "-objects.txt")).write_text(objects, encoding="utf-8")
        counts = collections.Counter()
        for line in objects.splitlines():
            match = re.match(r"^(\d+)\s+(.+?\.p3d)\s*([-\d.]+)\s+([-\d.]+)\s+([-\d.]+)\s*$", line, re.I)
            if match:
                counts[norm(match[2])] += 1
        reported = int(re.search(r"Objects:\s+(\d+)", info)[1])
        assert sum(counts.values()) == reported, (name, sum(counts.values()), reported)
        terrain = [norm(m[1]) for line in textures.splitlines()
                   if (m := re.match(r"^\d+\s+(.+\.(?:pac|paa))\s*$", line, re.I))]
        worlds[name] = {"asset": ("noe/noe.wrp" if stem == "noe" else "worlds/" + stem + ".wrp"),
                        "sha256": sha(wrp), "instances": reported, "models": dict(sorted(counts.items())),
                        "terrainTextureDeclarations": sorted(set(terrain))}
        print(f"world {name}: {reported} placements / {len(counts)} models", flush=True)
    config = args.game / "bin" / "CONFIG.BIN"
    preserved[config.relative_to(args.game).as_posix()] = sha(config)
    data = {"schema": "cwrr-original-asset-inventory/1", "sourceCommit": subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(), "toolSHA256": sha(args.tool),
        "materialConfigSHA256": sha(config), "archives": archives, "worlds": worlds,
        "models": list(assets["models"].values()), "textures": list(assets["textures"].values()),
        "preservedInputs": preserved}
    save(args.cache / "scan.json", data)
    return data


def parse_model(output):
    lods, current, native, qualify = {}, None, False, None
    for line in output.splitlines():
        if m := re.match(r"LOD (\d+) \(Resolution: ([^)]+)\)", line):
            current = int(m[1]); lods[current] = {"index": current, "resolution": float(m[2]), "textures": [], "sections": []}
        elif m := re.match(r"  (Points|Faces|Textures|Selections):\s+(\d+)", line):
            lods[current][m[1].lower() + "Count"] = int(m[2])
        elif m := re.match(r"    \[\d+\] (.*\.(?:pac|paa))$", line, re.I):
            lods[current]["textures"].append(norm(m[1]))
        elif line.startswith("Native lighting ("):
            native = True
        elif m := re.match(r"Qualification LOD (\d+) res=(.*)", line):
            qualify = int(m[1])
        elif m := re.match(r"  section=(\d+) texture=(.*?) source=(\d+) opaque=(\d+) declared=(\d+) "
                          r"eligible=(\d+) unknownSource=(\d+) renderBehavior=(\d+) authoredLighting=(\d+) geometry=(\d+)", line):
            values = m.groups()
            lods[qualify]["sections"].append(dict(zip(
                ["index", "texture", "source", "opaque", "declared", *REASONS],
                [int(v) if i != 1 else norm(v) for i, v in enumerate(values)])))
        elif native and (m := re.match(r"  LOD (\d+) res=(\S+) vertices=(\d+) normals=(\d+) invalidNormals=(\d+) "
                                      r"userMaterialVertices=(\d+) landHintVertices=(\d+) lightModes=(.*)", line)):
            current = int(m[1])
            lods[current].update(dict(zip(["vertices", "normals", "invalidNormals", "authoredVertices", "landHintVertices"],
                                         map(int, m.groups()[2:7]))))
            lods[current]["lightModes"] = {k: int(v) for k, v in re.findall(r"(\d+):(\d+)", m[8])}
        elif native and (m := re.match(r"    native section (\d+) tex=(.*?) flags=0x([0-9a-f]+) vertices=(\d+) "
                                      r"invalidVertexNormals=(\d+) materialIndices=(.*)", line)):
            section = next(s for s in lods[current]["sections"] if s["index"] == int(m[1]))
            assert section["texture"] == ("" if m[2] == "<none>" else norm(m[2]))
            section.update(flags=int(m[3], 16), vertices=int(m[4]), invalidNormals=int(m[5]),
                           materialIndices={k: int(v) for k, v in re.findall(r"(\d+):(\d+)", m[6])})
            if uv := re.search(r" uvMin=(\S+),(\S+) uvMax=(\S+),(\S+) invalidUV=(\d+)", line):
                section.update(uvMin=list(map(float, uv.groups()[:2])),
                               uvMax=list(map(float, uv.groups()[2:4])), invalidUV=int(uv[5]))
    visual = [lod for lod in lods.values() if lod["resolution"] < 10000]
    for lod in visual:
        assert "normals" in lod, lod
        for section in lod["sections"]:
            assert "flags" in section, section
        lod["triangles"] = {reason: sum(s[reason] for s in lod["sections"]) for reason in REASONS}
    return {"format": re.search(r"^Format: (.+)$", output, re.M)[1], "visualLODs": visual,
            "nonvisualLODCount": len(lods) - len(visual)}


def inspect(args, data):
    assert sha(args.tool) == data["toolSHA256"], "tool changed; use a fresh cache"
    uses = collections.defaultdict(dict)
    for name, world in data["worlds"].items():
        for model, count in world["models"].items():
            uses[model][name] = count
    known = {m["path"] for m in data["models"]}
    data["missingWorldModels"] = sorted(set(uses) - known)
    jobs = [m for m in data["models"] if m["path"] in uses]

    def one(model):
        key = model["path"]
        model["placements"] = uses[key]
        raw = args.cache / "model-reports" / (hashlib.sha256(key.encode()).hexdigest()[:20] + ".txt")
        raw.parent.mkdir(exist_ok=True)
        try:
            if not raw.is_file():
                output = command(args, "model", "inspect", args.cache / "library" / key, "--textures", "--sections",
                                 "--qualify", "--texroot", args.cache / "library", "--material-config",
                                 args.game / "bin" / "CONFIG.BIN", timeout=180)
                raw.write_text(output, encoding="utf-8")
            model["inspection"] = parse_model(raw.read_text(encoding="utf-8"))
        except Exception as error:
            model["inspectionError"] = str(error)
        return model

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as pool:
        for n, model in enumerate(pool.map(one, jobs), 1):
            if n % 25 == 0 or "inspectionError" in model:
                print(f"inspected {n}/{len(jobs)} {model['path']} {'ERROR' if 'inspectionError' in model else ''}", flush=True)
    referenced = collections.defaultdict(set)
    for model in jobs:
        for lod in model.get("inspection", {}).get("visualLODs", []):
            for section in lod["sections"]:
                texture = section["texture"]
                if Path(texture).suffix in (".paa", ".pac") and sum(section[r] for r in REASONS):
                    referenced[texture].add(model["path"])
    terrain = collections.defaultdict(list)
    for name, world in data["worlds"].items():
        for texture in world["terrainTextureDeclarations"]:
            terrain[texture].append(name)
    data["missingWorldTextures"] = sorted((set(referenced) | set(terrain)) - {t["path"] for t in data["textures"]})
    texture_jobs = [t for t in data["textures"] if t["path"] in referenced or t["path"] in terrain]

    def texture_one(texture):
        key = texture["path"]
        texture["worldModels"] = sorted(referenced[key])
        texture["terrainWorlds"] = terrain[key]
        raw = args.cache / "texture-reports" / (hashlib.sha256(key.encode()).hexdigest()[:20] + ".txt")
        raw.parent.mkdir(exist_ok=True)
        try:
            if not raw.is_file():
                raw.write_text(command(args, "image", "inspect", args.cache / "library" / key), encoding="utf-8")
            text = raw.read_text(encoding="utf-8")
            size = re.search(r"Dimensions: (\d+)x(\d+)", text)
            alpha = re.search(r"Classification: (\w+)", text)
            texture["decoded"] = {"width": int(size[1]), "height": int(size[2]), "alpha": alpha[1].lower()}
        except Exception as error:
            texture["decodeError"] = str(error)
        return texture

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as pool:
        for n, texture in enumerate(pool.map(texture_one, texture_jobs), 1):
            if n % 100 == 0:
                print(f"decoded {n}/{len(texture_jobs)} referenced textures", flush=True)
    save(args.cache / "inspected.json", data)
    return data


def summarize(data):
    models = [m for m in data["models"] if "placements" in m]
    totals, weighted, support, frequency = (collections.Counter() for _ in range(4))
    base = {}
    notes = []
    for model in models:
        lods = model.get("inspection", {}).get("visualLODs", [])
        if not lods:
            continue
        lod = min(lods, key=lambda lod: (lod["resolution"], lod["index"]))
        base[model["path"]] = lod
        count = sum(model["placements"].values())
        totals.update(lod["triangles"])
        weighted.update({k: v * count for k, v in lod["triangles"].items()})
        triangles, eligible = sum(lod["triangles"].values()), lod["triangles"]["eligible"]
        support["fullyQualified" if eligible == triangles and triangles else "partlyQualified" if eligible else "noneQualified"] += 1
        bucket = "1" if count == 1 else "2-9" if count < 10 else "10-99" if count < 100 else "100-999" if count < 1000 else "1000+"
        frequency[bucket] += 1
        for visual in lods:
            count_tri = sum(visual["triangles"].values())
            if not visual["facesCount"] <= count_tri <= 2 * visual["facesCount"]:
                notes.append({"model": model["path"], "lod": visual["index"],
                              "previewFaces": visual["facesCount"], "nativeSectionTriangles": count_tri})
    texture_union = [t for t in data["textures"] if t.get("worldModels") or t.get("terrainWorlds")]
    data["summary"] = {
        "libraryModels": len(data["models"]), "libraryTextures": len(data["textures"]),
        "archives": len(data["archives"]), "worldModels": len(models),
        "worldInstances": sum(w["instances"] for w in data["worlds"].values()),
        "worldVisualSectionTextures": sum(bool(t.get("worldModels")) for t in texture_union),
        "worldTerrainTableTextures": sum(bool(t.get("terrainWorlds")) for t in texture_union),
        "worldTextureUnion": len(texture_union),
        "visualLODsInspected": sum(len(m.get("inspection", {}).get("visualLODs", [])) for m in models),
        "modelsWithInvalidVisualNormals": sum(any(l["invalidNormals"] for l in m.get("inspection", {}).get("visualLODs", [])) for m in models),
        "highestDetailTriangles": dict(totals), "placementWeightedHighestDetailTriangles": dict(weighted),
        "highestDetailModelQualification": dict(support), "placementFrequencyModels": dict(frequency),
        "referencedTextureAlpha": dict(collections.Counter(t["decoded"]["alpha"] for t in texture_union if "decoded" in t)),
        "inspectionErrors": sum("inspectionError" in m for m in models),
        "decodeErrors": sum("decodeError" in t for t in texture_union),
        "nativePreviewFaceDiscrepancies": notes,
    }
    return base


def backlog(args, data, base):
    # Curated work ordering, not runtime identity inference or an allowlist.
    # Bundle close variants so frequency does not fill the list with rock clones.
    groups = [
        ("A", "Nogova rock variants", [m["path"] for m in data["models"] if m["path"].startswith("o/skal/") and "placements" in m], "Verify original surface identity across variants; retain namespace-specific source proof."),
        ("A", "Nogova low boundary walls", [f"o/hous/zidka0{i}.p3d" for i in (1,2,3,4)], "Verify narrow opaque sections and source identity; do not inherit classic-wall texture identities."),
        ("A", "Classic fence brick pillars", [f"data3d/plot_{c}_cihlsloup.p3d" for c in ("green","rust")], "Inspect isolated pillar geometry before any optical classification; do not adopt wire or gates."),
        ("A", "Nogova wood pile", ["o/hous/drevo_hromada.p3d"], "Audit all original sections for separable diffuse response; name and opacity alone are not proof."),
        ("A", "Classic farm shed", ["data3d/statek_kulna.p3d"], "Preserve existing qualified wood; investigate only the remaining source-unknown sections."),
        ("B", "Green wire fence", ["data3d/plot_green_draty.p3d"], "Separate painted/metal hardware from diffuse regions; preserve cutouts and original wire behavior."),
        ("B", "Rust wire fence", ["data3d/plot_rust_draty.p3d"], "Audit wire/post response without treating opaque metal as rough dielectric."),
        ("B", "Metal anti-tank obstacle", ["data3d/jezek kov.p3d"], "Establish metal/paint response from trustworthy source evidence; never infer dielectric from opacity."),
        ("B", "Classic farm main building", ["data3d/statek_hl_bud.p3d"], "Keep supported roof; separate wall from windows, doors and hardware using original source evidence."),
        ("B", "Barracks pilot family", ["data3d/budova1.p3d","data3d/budova2.p3d"], "Resolve mixed opaque atlas identity; preserve windows/doors and do not globally promote the texture."),
        ("B", "Classic city-house pilot", ["data3d/dum_mesto3.p3d"], "Keep four supported roof triangles; facade/window identity remains unresolved."),
        ("B", "Classic rasovna houses", ["data3d/dum_rasovna.p3d","data3d/dum_rasovna_in.p3d"], "Extend only reliably separable exterior regions; retain interiors and authored shadow lighting."),
        ("B", "Classic brick-house variants", ["data3d/cihlovej_dum.p3d","data3d/cihlovej_dum_mini.p3d","data3d/cihlovej_dum_in.p3d"], "Preserve roof coverage, authored shadow indices and invalid geometry fallback across variants."),
        ("B", "Nogova barn", ["o/hous/stodola.p3d"], "Separate source-unknown opaque areas from cutout and authored-lighting ranges."),
        ("B", "Nogova barn variant 3", ["o/hous/stodola3.p3d"], "Verify opaque atlas identity; compare exterior and openings without guessed material labels."),
        ("B", "Nogova grey house", ["o/hous/domek_sedy.p3d"], "Preserve cutouts/interiors/authored modes; prove any facade subsection independently."),
        ("B", "Nogova ruined house", ["o/hous/domek_ruina.p3d"], "Preserve ruin openings and authored lighting; classify only defensible opaque subsections."),
        ("B", "Nogova Podhradi house", ["o/hous/domek_podhradi_1.p3d"], "Resolve mixed opaque source identity while keeping authored lighting legacy."),
        ("B", "Nogova yellow farm", ["o/hous/zluty_statek.p3d"], "Audit mixed facade atlas and cutout ranges independently."),
        ("B", "Nogova yellow house", ["o/hous/domek_zluty.p3d"], "Separate windows/interiors/cutouts and retain authored material indices."),
        ("B", "Nogova smithy house", ["o/hous/domek_kovarna.p3d"], "Inspect source separability before expanding exterior coverage; keep authored modes."),
        ("B", "Nogova multi-storey house 02", ["o/hous/dum_patrovy02.p3d"], "Resolve opaque mixed sections independently of cutouts and authored shading."),
        ("B", "Nogova apartment pilot family", ["o/hous/panelak.p3d","o/hous/panelak2.p3d"], "Keep shadow-only 201/205/207, cutouts and interior shading; also audit invalid geometry before adoption."),
        ("B", "Classic chapel", ["data3d/kaple.p3d"], "Review opaque mixed-source sections; geometry and opacity are prerequisites, not material identity."),
        ("B", "Nogova rubble wall", ["o/hous/bordel_zidka.p3d"], "Verify source identity and geometry consistency before any diffuse adoption."),
        ("B", "Classic pine trunks", ["data3d/str borovice.p3d","data3d/str borovice horska.p3d"], "Investigate separately sectioned opaque trunk regions; keep all foliage/cutout shading legacy."),
        ("B", "Nogova maple trunks", ["o/tree/javor01.p3d","o/tree/javor02.p3d"], "Verify trunk-only source separation and normals across LODs; preserve canopy behavior."),
        ("C", "Nogova small spruce canopy", ["o/tree/smrk_maly.p3d"], "Defer until a thin/cutout vegetation contract exists; do not relax opaque diffuse qualification."),
        ("C", "Nogova common bush canopy", ["o/tree/dd_bush02d.p3d"], "High frequency but no qualifying diffuse region; preserve alpha and authored vegetation response."),
        ("C", "Classic fitted roads", [m["path"] for m in data["models"] if "placements" in m and m["path"].startswith("data3d/") and Path(m["path"]).stem.startswith(("asfalt","asfatlka","silnice","cesta"))], "Defer to a fitted-ground lighting contract; original land hints/depth behavior must not be bypassed."),
    ]
    models = {m["path"]: m for m in data["models"]}
    rows = []
    for rank, (tier, title, paths, action) in enumerate(groups, 1):
        assert paths and all(path in base for path in paths), title
        placements, geometry = collections.Counter(), collections.Counter()
        for path in paths:
            placements.update(models[path]["placements"])
            geometry.update(base[path]["triangles"])
        rows.append({"rank": rank, "tier": tier, "candidate": title, "models": sorted(paths),
                     "placements": dict(placements), "totalInstances": sum(placements.values()),
                     "highestDetailTriangles": dict(geometry), "nextAction": action})
    save(args.output.with_name("backlog.json"), {"schema": "cwrr-modernization-backlog/1", "sourceCommit": data["sourceCommit"],
        "ordering": "Curated: narrower opaque-source audits first, mixed sources next, unsupported transport contracts deferred. Frequency and related-model reuse inform order; not predicted screen coverage or automatic material identity.", "candidates": rows})


def finish(args, data):
    referenced = {t["path"] for t in data["textures"] if t.get("worldModels")}
    audit = []
    for index, archive in enumerate(data["archives"]):
        if not archive["path"].lower().startswith("addons/"):
            continue
        pbo = args.game / archive["path"]
        listing = command(args, "pbo", "list", pbo)
        entries = [m[1] for line in listing.splitlines()
                   if (m := re.match(r"^(config\.(?:bin|cpp))\s{2,}", line, re.I))]
        for entry in entries:
            folder = args.cache / "configs" / str(index)
            command(args, "pbo", "extract", pbo, folder, "--filter", entry)
            config = json.loads(command(args, "config", "tojson", folder / entry))
            materials = config.get("CfgMaterials", {})
            mappings = config.get("CfgTextureToMaterial", {})
            textures = {norm(t) for mapping in mappings.values() if isinstance(mapping, dict)
                        for t in mapping.get("textures", [])}
            audit.append({"archive": index, "entry": entry, "materialClasses": sorted(materials),
                          "mappedTextures": sorted(textures), "worldSectionTextureOverlap": sorted(textures & referenced)})
    data["addonMaterialAudit"] = audit
    data["coverageContract"] = {
        "mode": "Shared automatic material qualifier, static original source prerequisites only",
        "visualLODDefinition": "Source resolution < 10000; highest detail is minimum resolution then index",
        "reception": "Unverified: independently captured ambient/direct and live object/render gates are required",
        "textures": "Nonempty visual sections with triangles plus WRP terrain texture-table declarations; no proxy or mission traversal",
        "rejectionBuckets": "Mutually exclusive first-failure results, not independent lists of every blocker",
        "previewTriangleCounts": "Not used; native OriginalMaterialRanges fan counts are authoritative for this report",
        "otherRendering": "A strict qualifier rejection is not absence of legacy lighting, RT shadows, alpha traversal, or hand-verified catalog support",
    }
    # Existing hand-verified catalog endpoints are separate from the generic
    # qualifier. The inspection CLI does not expose their geometry/pixel hashes.
    declarations = [(1, "data3d/zed_kamenna.p3d", "data/kamennazed.pac", [.5, 1]),
                    (2, "data3d/zed2.p3d", "data/plot_beton_03_top.pac", [.5, 1]),
                    (3, "data3d/zed2.p3d", "data/plot_beton_0302_side.pac", [.5, 1, 4]),
                    (4, "data3d/zed2.p3d", "data/plot_beton_02_side.pac", [.5, 1, 4])]
    by_path = {model["path"]: model for model in data["models"]}
    data["existingCatalogDeclarations"] = [
        {"id": identity, "model": model, "texture": texture, "sourceResolutions": resolutions,
         "placements": by_path[model].get("placements", {}), "runtimeFingerprintGate": "not evaluated by this CLI"}
        for identity, model, texture, resolutions in declarations]
    base = summarize(data)
    backlog(args, data, base)
    for relative, digest in data["preservedInputs"].items():
        assert sha(args.game / relative) == digest, relative
    private = ROOT / "private" / "dlss"
    private_exists = (private / ".git").exists()
    data["preservation"] = {"inputsChecked": len(data["preservedInputs"]), "changed": 0,
        "privateDLSSClean": not subprocess.check_output(["git", "-C", str(private), "status", "--porcelain"], text=True).strip() if private_exists else None,
        "privateDLSSRemoteFree": not subprocess.check_output(["git", "-C", str(private), "remote"], text=True).strip() if private_exists else None,
        "privateSDKsPresent": all((ROOT / "build" / folder).is_dir() for folder in ("private-dlss-sdk", "private-nvapi"))}
    data["policySHA256"] = {name: sha(ROOT / "engine" / "PoseidonVK" / name) for name in (
        "MaterialQualificationVK.hpp", "PBRMaterialVK.hpp", "EnhancedMaterialVK.hpp")}
    for kind in ("models", "textures"):
        data[kind].sort(key=lambda row: row["path"])
    save(args.output, data)
    print(f"Saved {args.output}: {len(data['models'])} models / {len(data['textures'])} textures", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game", type=Path, required=True)
    parser.add_argument("--tool", type=Path, required=True)
    parser.add_argument("--cache", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--workers", type=int, default=4)
    parser.add_argument("--phase", choices=["scan", "inspect", "all"], default="all")
    args = parser.parse_args()
    for key in ("game", "tool", "cache", "output"):
        setattr(args, key, getattr(args, key).resolve())
    for destination in (args.cache, args.output):
        if destination.is_relative_to(args.game) or destination.is_relative_to(ROOT / "private"):
            parser.error("cache/output must not be inside original game data or private repositories")
    args.cache.mkdir(parents=True, exist_ok=True)
    data = scan(args) if args.phase in ("scan", "all") else json.loads((args.cache / "scan.json").read_text())
    if args.phase != "scan":
        data = inspect(args, data)
        finish(args, data)


if __name__ == "__main__":
    main()

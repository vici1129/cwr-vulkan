# Stone wall v1

Original supplemental artwork for `data3d/zed_kamenna.p3d`, visual LODs 0.5
(48 vertices) and 1 (24 vertices), section `data/kamennazed.pac`. The catalog
and source fingerprints are in `engine/PoseidonVK/EnhancedMaterialVK.hpp` and
`EngineVK_Materials.cpp`. Changed source geometry, vertex modes, original color
pixels, other models, other LODs and other sections retain their existing paths.

Enable with `triPBR 'enhanced'`; `triPBR 'on'` restores stock PBR v3 and
`triPBR 'off'` restores legacy materials. `triRTGI 'on'` separately enables
indirect replacement. Neither enhancement nor GI is a player preset/default.
The build copies this directory alongside the executable. Missing maps fall
back to stock PBR; original game archives are never changed.

* `albedo.paa`: opaque, unlit sRGB reflectance. Newly authored gray rubble,
  mortar and a fired-clay cap; no original game pixels, shadows or AO.
* `normal-roughness.paa`: **linear data**, RGB tangent normal with +X/+Y toward
  increasing U/V and +Z outward; alpha times catalog roughness (.9) gives
  perceptual roughness. Normal lengths are renormalized after sampling.
* Metallic is zero and dielectric F0 is .04. Roughness is .82-.90, not a value
  extracted from image brightness. Colors/optical properties are authored,
  unmeasured assumptions, not measured Malden masonry.

Both maps are lossless ARGB8888 PAA with mip chains, decoded/uploaded through
the existing texture system. The data map's partial alpha is roughness, not
transparency; only the original section controls coverage. The top 1/16 of the
atlas is the cap; this layout follows section UV usage, not inferred lighting.

Reproduce with Python/Pillow:

```
python tools/materials/author_stone_wall.py build/stone-wall-art
```

Copy the two generated PAA files here and rebuild. The generator reads no game
assets. It defines stone outlines, shallow 7 mm bevels and small surface relief,
then differentiates that explicit height field to create normals. It assumes a
4 m by 1.28 m atlas metric; it is an authored replacement, not a reconstruction
of the stock texture or its real masonry. Further artistic refinement of stone
shape, grain and cap layout is expected. Artwork and generator are supplied
under the repository license.

#pragma once

// GraphicsConfig → live engine bridge.
//
// The Settings/ layer is allowed to know about engine globals
// (GScene + GEngine) here because this module is the boundary
// between "config value" and "running renderer".  Two callers:
//
//   - GameApplication::LoadAndApplyGraphicsConfig at boot.
//   - VideoPage's explicit Apply/OK transaction.
//
// Tier→engine-value mappings live in this TU so both callers see the
// same translation table; changing a mapping touches one place.

#include <Poseidon/UI/Settings/GraphicsConfig.hpp>

// Push every cfg field into the live engine state.  Idempotent — safe
// to call repeatedly, e.g. after every row change.  Reads GScene +
// GEngine + writes to gUserFpsCap (the FPS-cap global defined in
// engine/Poseidon/Core/Game/GameLoop.cpp).

namespace Poseidon
{
float TerrainGridForTier(GraphicsConfig::Tier tier);
float ObjectLodBiasForTier(GraphicsConfig::Tier tier);
// Optional baseline limits a staged UI commit to fields that actually changed;
// boot still applies the complete file. Unrelated developer/mod overrides stay live.
void ApplyGraphicsConfigToEngine(const GraphicsConfig& cfg, const GraphicsConfig* baseline = nullptr);

} // namespace Poseidon

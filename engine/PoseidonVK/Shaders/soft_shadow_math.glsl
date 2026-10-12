// Orthographic directional-light penumbra; also compiled by CPU policy tests.
float softShadowRadius(float separation, float sunRadius, float uvPerMetre, float texel) {
    return min(max(separation*sunRadius*uvPerMetre,texel),8.0f*texel);
}

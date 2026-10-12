// Equal-area inner/outer annuli, with an opposite pair in each. Averaged over
// spatial rotations this covers a uniform disc, with zero sample centroid at
// EVERY receiver (unlike four independent random directions). No frame seed.
// Scalar functions are also compiled by the existing CPU shader-math tests.
float rtSunDiskRadius(int sampleIndex, float radial) {
    return sqrt(sampleIndex<2 ? radial*.5f : 1.0f-radial*.5f);
}
float rtSunDiskAngle(int sampleIndex, float rotation) {
    return rotation+float(sampleIndex%2)*3.14159265359f+
        (sampleIndex<2 ? 0.0f : 1.57079632679f);
}
float rtSunDiscSlope() { return .00698143043f; } // tan(0.4 degrees), same sun as PCSS

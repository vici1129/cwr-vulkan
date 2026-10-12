// Parameters extend the existing AA push constants; no new pass or descriptors.
// The caller supplies the final world depth, before cockpit and UI composition.
vec3 underwaterColor(vec3 color, float depth, vec2 uv) {
    if (aa.water.x <= 0.0 || aa.water.w < .5) return color;
    vec3 viewRay = vec3((uv.x*2-1)*aa.waterView.x, (1-uv.y*2)*aa.waterView.y, 1);
    float rayLength = length(viewRay);
    float rayY = dot(viewRay, vec3(aa.waterView.zw, aa.waterLight.x)) / rayLength;
    float distance = depth < 1.0 ? abs(aa.water.z / min(depth-aa.water.y,-1e-7))*rayLength : 1000.0;
    // An upward ray travels through water only as far as the actual sea plane.
    if (rayY > 0.00001) distance = min(distance, aa.water.x/rayY);
    // Fade entirely below the plane, so an above-water view is an exact no-op.
    float crossing = smoothstep(0.0, .25, aa.water.x);
    distance = min(distance, 300.0)*crossing;
    vec3 transmission = exp(-vec3(.14,.065,.045)*distance);
    // Stock fog is already weather/time/eye adapted. Never invent a light floor.
    vec3 scatter = toLinear(max(aa.waterLight.yzw,vec3(0))) * vec3(.22,.40,.46);
    scatter *= exp(-.045*aa.water.x);
    return color*transmission + scatter*(1-transmission);
}

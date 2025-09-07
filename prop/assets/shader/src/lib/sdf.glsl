float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

float median(vec3 v) {
    return median(v.r, v.g, v.b);
}

float msdf(sampler2D samp, vec2 uv, float scale) {
    vec2 msdfUnit = 1. / vec2(textureSize(samp, 0)) * scale;
    vec3 col = texture(samp, uv).rgb;
    float sigDist = median(col) - 0.5;

    /**if(uniformed)*/sigDist *= dot(msdfUnit, 0.7 / fwidth(uv));

    return sigDist;
}

float msdf_no_lod(sampler2D samp, vec2 uv, float scale) {
    vec2 msdfUnit = 1. / vec2(textureSize(samp, 0)) * scale;
    vec3 col = texture(samp, uv, 0).rgb;
    float sigDist = median(col) - 0.5;

    /**if(uniformed)*/sigDist *= dot(msdfUnit, 0.7 / fwidth(uv));

    return sigDist;
}

//float msdf(sampler2DArray samp, vec3 uv, float scale, bool uniformed) {
//    vec2 msdfUnit = 1. / vec2(textureSize(samp, 0)) * scale;
//    vec3 col = texture(samp, uv).rgb;
//    float sigDist = median(col) - 0.5;
//
//    if(uniformed)sigDist *= dot(msdfUnit, 0.5 / fwidth(uv.xy));
//
//    return sigDist;
//}
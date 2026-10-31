#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 uResolution;
uniform float uBloomStrength;
uniform float uStormFlash;

out vec4 finalColor;

vec3 acesToneMap(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec2 uv = fragTexCoord;
    vec2 texel = 1.0 / max(uResolution, vec2(1.0));

    vec4 base = texture(texture0, uv);

    // 12-tap dual-ring bloom extraction for soft atmospheric halation
    vec3 bloomSum = vec3(0.0);
    const vec2 offsets[12] = vec2[](
        vec2(-2.0, -2.0), vec2(2.0, -2.0),
        vec2(-2.0,  2.0), vec2(2.0,  2.0),
        vec2(-4.0,  0.0), vec2(4.0,  0.0),
        vec2( 0.0, -4.0), vec2(0.0,  4.0),
        vec2(-6.5, -3.5), vec2(6.5, -3.5),
        vec2(-3.5,  6.5), vec2(3.5,  6.5)
    );

    for (int i = 0; i < 12; ++i) {
        vec3 s = texture(texture0, uv + offsets[i] * texel).rgb;
        float lum = dot(s, vec3(0.2126, 0.7152, 0.0722));
        float softKnee = smoothstep(0.62, 1.15, lum);
        bloomSum += max(vec3(0.0), s - vec3(0.55)) * (softKnee * 1.28);
    }
    bloomSum *= (1.0 / 12.0) * uBloomStrength;

    vec3 color = base.rgb + bloomSum;

    // Storm lightning flash boost (clamped by accessibility setting in C++)
    color += vec3(0.55, 0.78, 1.0) * (uStormFlash * 0.24);

    // Subtle cinematic brass-spyglass vignette
    vec2 centered = uv * 2.0 - 1.0;
    float vig = 1.0 - dot(centered, centered) * 0.11;
    color *= clamp(vig, 0.78, 1.0);

    // ACES filmic tone curve + crisp contrast polish
    color = acesToneMap(color * 1.10);

    finalColor = vec4(color, 1.0);
}

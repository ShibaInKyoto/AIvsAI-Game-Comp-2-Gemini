#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec3 uViewPos;
uniform vec3 uSunDir;
uniform vec3 uSunColor;
uniform vec3 uSkyAmbient;
uniform vec3 uGroundAmbient;
uniform vec3 uFogColor;
uniform float uFogDensity;
uniform float uEmissive;

out vec4 finalColor;

void main() {
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec3 baseAlbedo = texelColor.rgb * colDiffuse.rgb * fragColor.rgb;
    float alpha = texelColor.a * colDiffuse.a * fragColor.a;

    vec3 N = normalize(fragNormal);
    vec3 V = normalize(uViewPos - fragPosition);
    vec3 L = normalize(-uSunDir);
    vec3 H = normalize(L + V);

    // Hemisphere sky-to-cloud ambient lighting
    float hemiMix = N.y * 0.5 + 0.5;
    vec3 ambient = mix(uGroundAmbient, uSkyAmbient, hemiMix);

    // Wrapped diffuse for soft stylized 3D form readability
    float NdotL = max(0.0, (dot(N, L) + 0.18) / 1.18);
    vec3 diffuse = uSunColor * NdotL;

    // Blinn-Phong specular highlight + Fresnel rim light
    float spec = pow(max(0.0, dot(N, H)), 28.0) * 0.35;
    float fresnel = pow(1.0 - max(0.0, dot(N, V)), 3.0) * 0.25;
    vec3 rimLight = uSkyAmbient * fresnel;

    vec3 litColor = baseAlbedo * (ambient + diffuse + baseAlbedo * uEmissive * 1.8) +
                    uSunColor * spec + rimLight;

    // Distance + low-altitude cloud sea atmospheric fog
    float dist = length(uViewPos - fragPosition);
    float distFog = 1.0 - exp(-dist * uFogDensity);
    float heightFog = clamp((72.0 - fragPosition.y) / 65.0, 0.0, 0.85);
    float totalFog = clamp(max(distFog, heightFog * 0.65), 0.0, 0.92);

    vec3 fogged = mix(litColor, uFogColor, totalFog);
    finalColor = vec4(fogged, alpha);
}

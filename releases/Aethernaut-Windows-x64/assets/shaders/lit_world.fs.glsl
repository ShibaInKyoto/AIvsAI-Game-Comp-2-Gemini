#version 330

in vec3 fragPosition;
in vec3 fragLocalPos;
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
uniform int uMaterialType;
uniform float uTime;

// Dynamic Bow Searchlight cone
uniform vec3 uSearchlightPos;
uniform vec3 uSearchlightDir;
uniform float uSearchlightActive;

// Dynamic Boss / Anomaly Point Light
uniform vec3 uBossLightPos;
uniform vec3 uBossLightColor;
uniform float uBossLightIntensity;

// 3D Acoustic Sonar Wavefront Ring
uniform vec3 uSonarOrigin;
uniform float uSonarRadius;
uniform float uSonarStrength;

out vec4 finalColor;

// Fast 3D hash & value noise for procedural material synthesis
float hash31(vec3 p) {
    p = fract(p * vec3(443.897, 441.423, 437.195));
    p += dot(p, p.yzx + 19.19);
    return fract((p.x + p.y) * p.z);
}

float noise3D(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float n000 = hash31(i + vec3(0.0, 0.0, 0.0));
    float n100 = hash31(i + vec3(1.0, 0.0, 0.0));
    float n010 = hash31(i + vec3(0.0, 1.0, 0.0));
    float n110 = hash31(i + vec3(1.0, 1.0, 0.0));
    float n001 = hash31(i + vec3(0.0, 0.0, 1.0));
    float n101 = hash31(i + vec3(1.0, 0.0, 1.0));
    float n011 = hash31(i + vec3(0.0, 1.0, 1.0));
    float n111 = hash31(i + vec3(1.0, 1.0, 1.0));

    float nx00 = mix(n000, n100, f.x);
    float nx10 = mix(n010, n110, f.x);
    float nx01 = mix(n001, n101, f.x);
    float nx11 = mix(n011, n111, f.x);

    float nxy0 = mix(nx00, nx10, f.y);
    float nxy1 = mix(nx01, nx11, f.y);

    return mix(nxy0, nxy1, f.z);
}

float fbm3(vec3 p) {
    float v = 0.0;
    float a = 0.5;
    vec3 shift = vec3(37.2, 19.8, 53.4);
    for (int i = 0; i < 3; ++i) {
        v += a * noise3D(p);
        p = p * 2.04 + shift;
        a *= 0.5;
    }
    return v;
}

void main() {
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec3 baseAlbedo = texelColor.rgb * colDiffuse.rgb * fragColor.rgb;
    float alpha = texelColor.a * colDiffuse.a * fragColor.a;

    vec3 N = normalize(fragNormal);
    vec3 V = normalize(uViewPos - fragPosition);
    vec3 L = normalize(-uSunDir);

    float specPower = 28.0;
    float specStrength = 0.35;
    float rimStrength = 0.26;
    float subsurface = 0.0;
    float extraEmissive = 0.0;

    // Procedural Material Detail & Normal Micro-Relief
    if (uMaterialType == 1) {
        // MAT_ISLAND_ROCK: Craggy stratified basalt & limestone cliff bands
        float strata = sin(fragPosition.y * 0.42 + fbm3(fragPosition * 0.07) * 5.0);
        float crag = fbm3(fragPosition * 0.22);
        float fissure = smoothstep(0.22, 0.55, noise3D(fragPosition * 0.48));
        vec3 rockDark = baseAlbedo * 0.64;
        vec3 rockLight = baseAlbedo * 1.24 + vec3(0.05, 0.04, 0.03);
        baseAlbedo = mix(rockDark, rockLight, clamp(strata * 0.35 + crag * 0.65, 0.0, 1.0));
        baseAlbedo *= (0.72 + 0.28 * fissure);

        // Lichen / moss accents on upward-facing ledges
        float ledgeMoss = smoothstep(0.45, 0.82, N.y) * smoothstep(25.0, 95.0, fragPosition.y);
        baseAlbedo = mix(baseAlbedo, vec3(0.22, 0.42, 0.26), ledgeMoss * 0.45);

        // Normal perturbation for craggy rock facets
        vec3 rockPerturb = vec3(
            noise3D(fragPosition * 0.35 + vec3(1.7, 0.0, 0.0)) - 0.5,
            strata * 0.18,
            noise3D(fragPosition * 0.35 + vec3(0.0, 0.0, 2.3)) - 0.5
        );
        N = normalize(N + rockPerturb * 0.42);
        specPower = 14.0;
        specStrength = 0.16;
    } else if (uMaterialType == 2) {
        // MAT_ISLAND_TURF: Lush wind-swept archipelago meadow & alpine flora
        float meadow = fbm3(fragPosition * 0.28);
        float windSweep = sin(fragPosition.x * 0.18 + fragPosition.z * 0.14 + uTime * 1.6) * 0.5 + 0.5;
        vec3 lushGreen = baseAlbedo * vec3(0.88, 1.12, 0.85);
        vec3 goldenHeather = baseAlbedo * vec3(1.18, 1.05, 0.72);
        baseAlbedo = mix(lushGreen, goldenHeather, meadow * 0.48 + windSweep * 0.16);
        float microBlade = noise3D(fragPosition * 1.4);
        baseAlbedo *= (0.86 + 0.26 * microBlade);
        N = normalize(N + vec3((microBlade - 0.5) * 0.2, 0.0, (meadow - 0.5) * 0.2));
        specPower = 12.0;
        specStrength = 0.14;
    } else if (uMaterialType == 3) {
        // MAT_WOOD_HULL: Varnished timber planking with caulked seams & brass trim
        vec3 p = fragLocalPos * 2.0;
        float plankRow = fract(fragPosition.y * 1.25 + fragLocalPos.y * 2.0);
        float seam = smoothstep(0.0, 0.07, plankRow) * (1.0 - smoothstep(0.93, 1.0, plankRow));
        float grain = sin((fragPosition.z + fragPosition.x) * 4.5 + noise3D(fragPosition * 1.8) * 3.2) * 0.5 + 0.5;
        baseAlbedo *= (0.84 + 0.24 * grain) * (0.72 + 0.28 * seam);
        specPower = 36.0;
        specStrength = 0.42;
    } else if (uMaterialType == 4) {
        // MAT_METAL_ARMOR: Forged riveted iron, brass & Orichalcum plating
        float brushed = sin((fragPosition.x + fragPosition.z) * 14.0 + noise3D(fragPosition * 3.0) * 2.0) * 0.06;
        vec3 edgeDist = abs(fragLocalPos);
        float panelBevel = smoothstep(0.42, 0.49, max(edgeDist.x, max(edgeDist.y, edgeDist.z)));
        baseAlbedo = mix(baseAlbedo * (1.0 + brushed), baseAlbedo * 0.65, panelBevel * 0.55);
        specPower = 64.0;
        specStrength = 0.85;
        rimStrength = 0.42;
    } else if (uMaterialType == 5) {
        // MAT_GAS_ENVELOPE: Ribbed canvas-and-dope Zeppelin envelope with longitudinal stitched gores
        float goreRib = abs(sin(fragLocalPos.x * 18.0 + fragPosition.x * 0.8));
        float ringSeam = smoothstep(0.88, 0.98, abs(sin(fragPosition.z * 1.4)));
        float weave = noise3D(fragPosition * 3.2) * 0.09;
        baseAlbedo *= (0.88 + 0.14 * goreRib + weave);
        baseAlbedo = mix(baseAlbedo, baseAlbedo * 0.72, ringSeam * 0.45);
        N = normalize(N + vec3((goreRib - 0.5) * 0.18, 0.0, ringSeam * 0.15));
        subsurface = 0.25;
        specPower = 14.0;
        specStrength = 0.08;
        rimStrength = 0.16;
    } else if (uMaterialType == 6) {
        // MAT_LEVIATHAN_HIDE: Weathered hexagonal dragon-chitin armor plates & pulsing bioluminescent seams
        float scaleA = noise3D(fragPosition * 1.15);
        float scaleB = noise3D(fragPosition * 2.45 + vec3(4.2, 1.7, 8.1));
        float plateEdge = smoothstep(0.28, 0.42, scaleA) * (1.0 - smoothstep(0.72, 0.88, scaleB));
        float bioVein = smoothstep(0.74, 0.90, noise3D(fragPosition * 0.48 + vec3(0.0, uTime * 0.35, 0.0)));
        baseAlbedo = mix(baseAlbedo * 0.52, baseAlbedo * 1.14, plateEdge);
        vec3 veinGlow = mix(vec3(0.15, 0.95, 1.0), vec3(1.0, 0.55, 0.15), clamp(uEmissive, 0.0, 1.0));
        baseAlbedo += veinGlow * bioVein * 0.75;
        extraEmissive = bioVein * 0.85;
        N = normalize(N + vec3((scaleA - 0.5) * 0.38, (plateEdge - 0.5) * 0.28, (scaleB - 0.5) * 0.38));
        specPower = 24.0;
        specStrength = 0.20;
        rimStrength = 0.36;
    } else if (uMaterialType == 7) {
        // MAT_CLOUD_SEA: Soft matte atmospheric stratocumulus cloud deck with golden silver-lining
        float wisp = fbm3(fragPosition * 0.028 + vec3(uTime * 0.03, 0.0, uTime * 0.02));
        float billow = noise3D(fragPosition * 0.09);
        baseAlbedo *= (0.92 + 0.14 * wisp + 0.06 * billow);
        N = normalize(vec3(N.x * 0.28 + (wisp - 0.5) * 0.15, max(0.75, abs(N.y)), N.z * 0.28 + (billow - 0.5) * 0.15));
        subsurface = 0.65;
        specStrength = 0.0;
        rimStrength = 0.18;
    } else if (uMaterialType == 8) {
        // MAT_AETHER_CRYSTAL: Faceted sky-crystal & sanctuary resonance obelisk
        float facet = abs(sin(fragPosition.x * 2.2) * cos(fragPosition.y * 2.8) * sin(fragPosition.z * 2.2));
        float pulse = 0.5 + 0.5 * sin(uTime * 3.2 + fragPosition.y * 0.5);
        baseAlbedo = mix(baseAlbedo, vec3(0.55, 0.95, 1.0), facet * 0.38);
        extraEmissive = 0.45 + 0.35 * pulse;
        specPower = 80.0;
        specStrength = 0.95;
        rimStrength = 0.65;
    } else {
        // Subtle triplanar micro-variation so no geometry looks flat
        float micro = noise3D(fragPosition * 0.65) * 0.08;
        baseAlbedo *= (0.96 + micro);
    }

    vec3 H = normalize(L + V);

    // Hemisphere sky-to-cloud ambient lighting with subtle horizon bounce
    float hemiMix = N.y * 0.5 + 0.5;
    vec3 ambient = mix(uGroundAmbient, uSkyAmbient, hemiMix);

    // Wrapped diffuse + subsurface scattering for clouds & gas envelopes
    float NdotL = max(0.0, (dot(N, L) + 0.20) / 1.20);
    float backScatter = max(0.0, dot(-N, L)) * subsurface;
    vec3 diffuse = uSunColor * (NdotL + backScatter);

    // Blinn-Phong specular highlight + Fresnel rim light
    float spec = pow(max(0.0, dot(N, H)), specPower) * specStrength;
    float fresnel = pow(1.0 - max(0.0, dot(N, V)), 3.0) * rimStrength;
    vec3 rimLight = mix(uSkyAmbient, uSunColor, 0.35) * fresnel;

    float totalEmissive = uEmissive + extraEmissive;
    vec3 litColor = baseAlbedo * (ambient + diffuse + baseAlbedo * totalEmissive * 1.85) +
                    uSunColor * spec + rimLight;

    // Dynamic Bow Searchlight Cone illumination
    if (uSearchlightActive > 0.01) {
        vec3 toFrag = fragPosition - uSearchlightPos;
        float sDist = length(toFrag);
        if (sDist > 0.5 && sDist < 420.0) {
            vec3 sDir = toFrag / sDist;
            float coneDot = dot(sDir, normalize(uSearchlightDir));
            float coneMask = smoothstep(0.84, 0.965, coneDot);
            float atten = (1.0 - sDist / 420.0);
            atten = atten * atten;
            float sDiff = max(0.25, dot(N, -sDir));
            vec3 beamCol = vec3(1.0, 0.95, 0.80) * (coneMask * atten * sDiff * 1.65 * uSearchlightActive);
            litColor += baseAlbedo * beamCol + beamCol * 0.22;
        }
    }

    // Dynamic Boss / Anomaly Bioluminescent Point Light
    if (uBossLightIntensity > 0.01) {
        vec3 toLight = uBossLightPos - fragPosition;
        float bDist = length(toLight);
        if (bDist < 340.0) {
            vec3 bDir = toLight / max(bDist, 0.001);
            float bAtten = 1.0 - (bDist / 340.0);
            bAtten = bAtten * bAtten;
            float bDiff = max(0.2, dot(N, bDir));
            litColor += baseAlbedo * uBossLightColor * (bAtten * bDiff * uBossLightIntensity * 1.35);
        }
    }

    // 3D Acoustic Sonar Wavefront Ring projection
    if (uSonarStrength > 0.01) {
        float sonarDist = length(fragPosition - uSonarOrigin);
        float ringDelta = abs(sonarDist - uSonarRadius);
        float ringMask = smoothstep(14.0, 0.0, ringDelta);
        float gridScan = 0.6 + 0.4 * sin(fragPosition.y * 0.8 + sonarDist * 0.3);
        vec3 sonarCol = mix(vec3(0.15, 0.92, 1.0), vec3(1.0, 0.82, 0.25), ringMask * 0.5);
        litColor += sonarCol * (ringMask * gridScan * uSonarStrength * 1.45);
    }

    // Distance + low-altitude cloud sea atmospheric fog with golden solar in-scattering
    float dist = length(uViewPos - fragPosition);
    float distFog = 1.0 - exp(-dist * uFogDensity);
    float heightFog = clamp((70.0 - fragPosition.y) / 68.0, 0.0, 0.82);
    float totalFog = clamp(max(distFog, heightFog * 0.58), 0.0, 0.90);

    float sunAlign = pow(max(0.0, dot(-V, L)), 4.0);
    vec3 horizonFogColor = mix(uFogColor, uSunColor * 0.95 + uFogColor * 0.35, sunAlign * 0.32);

    vec3 fogged = mix(litColor, horizonFogColor, totalFog);
    finalColor = vec4(fogged, alpha);
}

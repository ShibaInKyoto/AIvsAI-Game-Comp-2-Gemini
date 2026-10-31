#include "render/ShaderPipeline.h"

namespace aethernaut {

namespace {

const char* kFallbackWorldVS = R"(#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
out vec3 fragPosition;
out vec3 fragLocalPos;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;
void main() {
    vec4 worldPos = matModel * vec4(vertexPosition, 1.0);
    fragPosition = worldPos.xyz;
    fragLocalPos = vertexPosition;
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    mat3 normalMatrix = transpose(inverse(mat3(matModel)));
    fragNormal = normalize(normalMatrix * vertexNormal);
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)";

const char* kFallbackWorldFS = R"(#version 330
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
uniform vec3 uSearchlightPos;
uniform vec3 uSearchlightDir;
uniform float uSearchlightActive;
uniform vec3 uBossLightPos;
uniform vec3 uBossLightColor;
uniform float uBossLightIntensity;
uniform vec3 uSonarOrigin;
uniform float uSonarRadius;
uniform float uSonarStrength;
out vec4 finalColor;
void main() {
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec3 baseAlbedo = texelColor.rgb * colDiffuse.rgb * fragColor.rgb;
    float alpha = texelColor.a * colDiffuse.a * fragColor.a;
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(uViewPos - fragPosition);
    vec3 L = normalize(-uSunDir);
    vec3 H = normalize(L + V);
    float hemiMix = N.y * 0.5 + 0.5;
    vec3 ambient = mix(uGroundAmbient, uSkyAmbient, hemiMix);
    float NdotL = max(0.0, (dot(N, L) + 0.18) / 1.18);
    vec3 diffuse = uSunColor * NdotL;
    float spec = pow(max(0.0, dot(N, H)), 28.0) * 0.35;
    float fresnel = pow(1.0 - max(0.0, dot(N, V)), 3.0) * 0.25;
    vec3 litColor = baseAlbedo * (ambient + diffuse + baseAlbedo * uEmissive * 1.8) + uSunColor * spec + uSkyAmbient * fresnel;
    float dist = length(uViewPos - fragPosition);
    float distFog = 1.0 - exp(-dist * uFogDensity);
    float heightFog = clamp((72.0 - fragPosition.y) / 65.0, 0.0, 0.85);
    float totalFog = clamp(max(distFog, heightFog * 0.65), 0.0, 0.92);
    finalColor = vec4(mix(litColor, uFogColor, totalFog), alpha);
}
)";

const char* kFallbackPostFS = R"(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec2 uResolution;
uniform float uBloomStrength;
uniform float uStormFlash;
out vec4 finalColor;
vec3 acesToneMap(vec3 x) {
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}
void main() {
    vec2 uv = fragTexCoord;
    vec2 texel = 1.0 / max(uResolution, vec2(1.0));
    vec4 base = texture(texture0, uv);
    vec3 bloomSum = vec3(0.0);
    const vec2 offsets[8] = vec2[](
        vec2(-2.5, -2.5), vec2(2.5, -2.5), vec2(-2.5, 2.5), vec2(2.5, 2.5),
        vec2(-4.5, 0.0), vec2(4.5, 0.0), vec2(0.0, -4.5), vec2(0.0, 4.5)
    );
    for (int i = 0; i < 8; ++i) {
        vec3 s = texture(texture0, uv + offsets[i] * texel).rgb;
        float lum = dot(s, vec3(0.2126, 0.7152, 0.0722));
        if (lum > 0.68) bloomSum += (s - vec3(0.60)) * 1.4;
    }
    vec3 color = base.rgb + bloomSum * 0.125 * uBloomStrength + vec3(0.55, 0.75, 1.0) * (uStormFlash * 0.25);
    vec2 centered = uv * 2.0 - 1.0;
    color *= clamp(1.0 - dot(centered, centered) * 0.14, 0.72, 1.0);
    finalColor = vec4(acesToneMap(color * 1.08), 1.0);
}
)";

} // namespace

void ShaderPipeline::init(int width, int height) {
    if (FileExists("assets/shaders/lit_world.vs.glsl") && FileExists("assets/shaders/lit_world.fs.glsl")) {
        worldShader = LoadShader("assets/shaders/lit_world.vs.glsl", "assets/shaders/lit_world.fs.glsl");
    } else {
        worldShader = LoadShaderFromMemory(kFallbackWorldVS, kFallbackWorldFS);
    }

    if (FileExists("assets/shaders/post_hdr.fs.glsl")) {
        postShader = LoadShader(nullptr, "assets/shaders/post_hdr.fs.glsl");
    } else {
        postShader = LoadShaderFromMemory(nullptr, kFallbackPostFS);
    }

    locViewPos_ = GetShaderLocation(worldShader, "uViewPos");
    locSunDir_ = GetShaderLocation(worldShader, "uSunDir");
    locSunColor_ = GetShaderLocation(worldShader, "uSunColor");
    locSkyAmbient_ = GetShaderLocation(worldShader, "uSkyAmbient");
    locGroundAmbient_ = GetShaderLocation(worldShader, "uGroundAmbient");
    locFogColor_ = GetShaderLocation(worldShader, "uFogColor");
    locFogDensity_ = GetShaderLocation(worldShader, "uFogDensity");
    locEmissive_ = GetShaderLocation(worldShader, "uEmissive");
    locMaterialType_ = GetShaderLocation(worldShader, "uMaterialType");
    locTime_ = GetShaderLocation(worldShader, "uTime");

    locSearchlightPos_ = GetShaderLocation(worldShader, "uSearchlightPos");
    locSearchlightDir_ = GetShaderLocation(worldShader, "uSearchlightDir");
    locSearchlightActive_ = GetShaderLocation(worldShader, "uSearchlightActive");

    locBossLightPos_ = GetShaderLocation(worldShader, "uBossLightPos");
    locBossLightColor_ = GetShaderLocation(worldShader, "uBossLightColor");
    locBossLightIntensity_ = GetShaderLocation(worldShader, "uBossLightIntensity");

    locSonarOrigin_ = GetShaderLocation(worldShader, "uSonarOrigin");
    locSonarRadius_ = GetShaderLocation(worldShader, "uSonarRadius");
    locSonarStrength_ = GetShaderLocation(worldShader, "uSonarStrength");

    locResolution_ = GetShaderLocation(postShader, "uResolution");
    locBloomStrength_ = GetShaderLocation(postShader, "uBloomStrength");
    locStormFlash_ = GetShaderLocation(postShader, "uStormFlash");

    targetW_ = width;
    targetH_ = height;
    sceneTarget_ = LoadRenderTexture(width, height);
    SetTextureFilter(sceneTarget_.texture, TEXTURE_FILTER_BILINEAR);
    initialized = true;

    setMaterialType(WorldMaterialType::Default);
    setEmissiveStrength(0.0f);
    updateDynamicLights(
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
        0.0f,
        {0.0f, 0.0f, 0.0f},
        {0.2f, 0.9f, 1.0f},
        0.0f,
        {0.0f, 0.0f, 0.0f},
        0.0f,
        0.0f
    );
}

void ShaderPipeline::shutdown() {
    if (!initialized) return;
    UnloadRenderTexture(sceneTarget_);
    UnloadShader(worldShader);
    UnloadShader(postShader);
    initialized = false;
}

void ShaderPipeline::checkResize(int width, int height) {
    if (!initialized || width <= 0 || height <= 0) return;
    if (width != targetW_ || height != targetH_) {
        UnloadRenderTexture(sceneTarget_);
        targetW_ = width;
        targetH_ = height;
        sceneTarget_ = LoadRenderTexture(width, height);
        SetTextureFilter(sceneTarget_.texture, TEXTURE_FILTER_BILINEAR);
    }
}

void ShaderPipeline::updateWorldUniforms(
    const Vec3& cameraPos,
    const Vec3& sunDir,
    const Vec3& sunColor,
    const Vec3& skyAmbient,
    const Vec3& groundAmbient,
    const Vec3& fogColor,
    float fogDensity,
    float simTime
) {
    if (!initialized) return;
    float vPos[3] = {cameraPos.x, cameraPos.y, cameraPos.z};
    float sDir[3] = {sunDir.x, sunDir.y, sunDir.z};
    float sCol[3] = {sunColor.x, sunColor.y, sunColor.z};
    float skyA[3] = {skyAmbient.x, skyAmbient.y, skyAmbient.z};
    float grdA[3] = {groundAmbient.x, groundAmbient.y, groundAmbient.z};
    float fCol[3] = {fogColor.x, fogColor.y, fogColor.z};

    SetShaderValue(worldShader, locViewPos_, vPos, SHADER_UNIFORM_VEC3);
    SetShaderValue(worldShader, locSunDir_, sDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(worldShader, locSunColor_, sCol, SHADER_UNIFORM_VEC3);
    SetShaderValue(worldShader, locSkyAmbient_, skyA, SHADER_UNIFORM_VEC3);
    SetShaderValue(worldShader, locGroundAmbient_, grdA, SHADER_UNIFORM_VEC3);
    SetShaderValue(worldShader, locFogColor_, fCol, SHADER_UNIFORM_VEC3);
    SetShaderValue(worldShader, locFogDensity_, &fogDensity, SHADER_UNIFORM_FLOAT);
    if (locTime_ >= 0) {
        SetShaderValue(worldShader, locTime_, &simTime, SHADER_UNIFORM_FLOAT);
    }
}

void ShaderPipeline::updateDynamicLights(
    const Vec3& searchlightPos,
    const Vec3& searchlightDir,
    float searchlightActive,
    const Vec3& bossLightPos,
    const Vec3& bossLightColor,
    float bossLightIntensity,
    const Vec3& sonarOrigin,
    float sonarRadius,
    float sonarStrength
) {
    if (!initialized) return;
    float slPos[3] = {searchlightPos.x, searchlightPos.y, searchlightPos.z};
    float slDir[3] = {searchlightDir.x, searchlightDir.y, searchlightDir.z};
    float blPos[3] = {bossLightPos.x, bossLightPos.y, bossLightPos.z};
    float blCol[3] = {bossLightColor.x, bossLightColor.y, bossLightColor.z};
    float snPos[3] = {sonarOrigin.x, sonarOrigin.y, sonarOrigin.z};

    if (locSearchlightPos_ >= 0) SetShaderValue(worldShader, locSearchlightPos_, slPos, SHADER_UNIFORM_VEC3);
    if (locSearchlightDir_ >= 0) SetShaderValue(worldShader, locSearchlightDir_, slDir, SHADER_UNIFORM_VEC3);
    if (locSearchlightActive_ >= 0) SetShaderValue(worldShader, locSearchlightActive_, &searchlightActive, SHADER_UNIFORM_FLOAT);

    if (locBossLightPos_ >= 0) SetShaderValue(worldShader, locBossLightPos_, blPos, SHADER_UNIFORM_VEC3);
    if (locBossLightColor_ >= 0) SetShaderValue(worldShader, locBossLightColor_, blCol, SHADER_UNIFORM_VEC3);
    if (locBossLightIntensity_ >= 0) SetShaderValue(worldShader, locBossLightIntensity_, &bossLightIntensity, SHADER_UNIFORM_FLOAT);

    if (locSonarOrigin_ >= 0) SetShaderValue(worldShader, locSonarOrigin_, snPos, SHADER_UNIFORM_VEC3);
    if (locSonarRadius_ >= 0) SetShaderValue(worldShader, locSonarRadius_, &sonarRadius, SHADER_UNIFORM_FLOAT);
    if (locSonarStrength_ >= 0) SetShaderValue(worldShader, locSonarStrength_, &sonarStrength, SHADER_UNIFORM_FLOAT);
}

void ShaderPipeline::setEmissiveStrength(float strength) {
    if (!initialized) return;
    SetShaderValue(worldShader, locEmissive_, &strength, SHADER_UNIFORM_FLOAT);
}

void ShaderPipeline::setMaterialType(WorldMaterialType matType) {
    if (!initialized || locMaterialType_ < 0) return;
    int val = static_cast<int>(matType);
    SetShaderValue(worldShader, locMaterialType_, &val, SHADER_UNIFORM_INT);
}

void ShaderPipeline::beginScenePass() {
    BeginTextureMode(sceneTarget_);
}

void ShaderPipeline::endSceneAndDrawPostProcess(int screenW, int screenH, bool enableBloom, float stormFlash) {
    EndTextureMode();

    float res[2] = {static_cast<float>(screenW), static_cast<float>(screenH)};
    float bloom = enableBloom ? 1.18f : 0.0f;
    SetShaderValue(postShader, locResolution_, res, SHADER_UNIFORM_VEC2);
    SetShaderValue(postShader, locBloomStrength_, &bloom, SHADER_UNIFORM_FLOAT);
    SetShaderValue(postShader, locStormFlash_, &stormFlash, SHADER_UNIFORM_FLOAT);

    BeginShaderMode(postShader);
    Rectangle srcRec = {0.0f, 0.0f, static_cast<float>(sceneTarget_.texture.width), -static_cast<float>(sceneTarget_.texture.height)};
    Rectangle dstRec = {0.0f, 0.0f, static_cast<float>(screenW), static_cast<float>(screenH)};
    DrawTexturePro(sceneTarget_.texture, srcRec, dstRec, {0.0f, 0.0f}, 0.0f, WHITE);
    EndShaderMode();
}

} // namespace aethernaut

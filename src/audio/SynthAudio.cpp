#include "audio/SynthAudio.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace aethernaut {

namespace {

constexpr int kSampleRate = 44100;

Sound buildSoundFromSamples(const std::vector<float>& samples) {
    Wave w{};
    w.frameCount = static_cast<unsigned int>(samples.size());
    w.sampleRate = kSampleRate;
    w.sampleSize = 16;
    w.channels = 1;

    short* pcm = static_cast<short*>(RL_MALLOC(samples.size() * sizeof(short)));
    for (size_t i = 0; i < samples.size(); ++i) {
        float clamped = std::clamp(samples[i], -1.0f, 1.0f);
        pcm[i] = static_cast<short>(clamped * 32000.0f);
    }
    w.data = pcm;
    Sound s = LoadSoundFromWave(w);
    UnloadWave(w);
    return s;
}

float randNoise() {
    return (static_cast<float>(std::rand() % 20000) / 10000.0f) - 1.0f;
}

} // namespace

void SynthAudio::init(bool enableAudio) {
    if (!enableAudio) {
        audioEnabled_ = false;
        return;
    }
    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        audioEnabled_ = false;
        return;
    }
    audioEnabled_ = true;

    sfxCannon_ = generateCannonBoom();
    sfxHarpoon_ = generateHarpoonLaunch();
    sfxLatch_ = generateHarpoonLatch();
    sfxSnap_ = generateCableSnap();
    sfxImpact_ = generateBlockImpact();
    sfxFracture_ = generateStructuralFracture();
    sfxRoar_ = generateLeviathanRoar();
    sfxChime_ = generateDiscoveryChime();
    musicLoop_ = generateSkyVoyageMusicLoop();
}

void SynthAudio::shutdown() {
    if (!audioEnabled_) return;
    UnloadSound(sfxCannon_);
    UnloadSound(sfxHarpoon_);
    UnloadSound(sfxLatch_);
    UnloadSound(sfxSnap_);
    UnloadSound(sfxImpact_);
    UnloadSound(sfxFracture_);
    UnloadSound(sfxRoar_);
    UnloadSound(sfxChime_);
    UnloadSound(musicLoop_);
    CloseAudioDevice();
    audioEnabled_ = false;
}

void SynthAudio::processWorldEvents(World& world, const Vec3& listenerPos, float sfxVol, float musicVol) {
    if (!audioEnabled_) {
        world.pendingAudioEvents.clear();
        return;
    }

    if (!IsSoundPlaying(musicLoop_) && musicVol > 0.01f) {
        SetSoundVolume(musicLoop_, musicVol * 0.45f);
        PlaySound(musicLoop_);
    } else {
        SetSoundVolume(musicLoop_, musicVol * 0.45f);
    }

    for (const auto& ev : world.pendingAudioEvents) {
        float dist = (ev.worldPos - listenerPos).length();
        float atten = std::clamp(1.0f - (dist / 950.0f), 0.1f, 1.0f) * sfxVol;
        if (atten <= 0.02f) continue;

        switch (ev.type) {
            case SimAudioEventType::CannonFire:
                SetSoundVolume(sfxCannon_, atten * 0.75f);
                PlaySound(sfxCannon_);
                break;
            case SimAudioEventType::HarpoonLaunch:
                SetSoundVolume(sfxHarpoon_, atten * 0.85f);
                PlaySound(sfxHarpoon_);
                break;
            case SimAudioEventType::HarpoonLatch:
                SetSoundVolume(sfxLatch_, atten * 0.9f);
                PlaySound(sfxLatch_);
                break;
            case SimAudioEventType::CableSnap:
                SetSoundVolume(sfxSnap_, atten * 0.95f);
                PlaySound(sfxSnap_);
                break;
            case SimAudioEventType::BlockImpact:
                SetSoundVolume(sfxImpact_, atten * 0.65f);
                PlaySound(sfxImpact_);
                break;
            case SimAudioEventType::StructuralFracture:
                SetSoundVolume(sfxFracture_, atten * 0.9f);
                PlaySound(sfxFracture_);
                break;
            case SimAudioEventType::LeviathanRoar:
                SetSoundVolume(sfxRoar_, atten * 1.0f);
                PlaySound(sfxRoar_);
                break;
            case SimAudioEventType::CratePickup:
            case SimAudioEventType::RepairPulse:
            case SimAudioEventType::TeslaDischarge:
                SetSoundVolume(sfxChime_, atten * 0.7f);
                PlaySound(sfxChime_);
                break;
        }
    }
    world.pendingAudioEvents.clear();
}

Sound SynthAudio::generateCannonBoom() {
    int count = static_cast<int>(kSampleRate * 0.55f);
    std::vector<float> s(static_cast<size_t>(count));
    float phase = 0.0f;
    for (int i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        float env = std::exp(-t * 7.5f);
        float freq = 135.0f * std::exp(-t * 9.0f) + 32.0f;
        phase += kTwoPi * freq / static_cast<float>(kSampleRate);
        s[static_cast<size_t>(i)] = (std::sin(phase) * 0.7f + randNoise() * 0.45f) * env;
    }
    return buildSoundFromSamples(s);
}

Sound SynthAudio::generateHarpoonLaunch() {
    int count = static_cast<int>(kSampleRate * 0.40f);
    std::vector<float> s(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        float env = std::exp(-t * 9.0f);
        float ratchet = std::sin(kTwoPi * (380.0f + std::sin(t * 140.0f) * 180.0f) * t);
        s[static_cast<size_t>(i)] = (ratchet * 0.5f + randNoise() * 0.35f) * env;
    }
    return buildSoundFromSamples(s);
}

Sound SynthAudio::generateHarpoonLatch() {
    int count = static_cast<int>(kSampleRate * 0.35f);
    std::vector<float> s(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        float env = std::exp(-t * 12.0f);
        float ring = std::sin(kTwoPi * 620.0f * t) + 0.5f * std::sin(kTwoPi * 980.0f * t);
        s[static_cast<size_t>(i)] = ring * 0.5f * env;
    }
    return buildSoundFromSamples(s);
}

Sound SynthAudio::generateCableSnap() {
    int count = static_cast<int>(kSampleRate * 0.45f);
    std::vector<float> s(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        float env = std::exp(-t * 8.0f);
        float freq = 940.0f * std::exp(-t * 5.0f);
        s[static_cast<size_t>(i)] = std::sin(kTwoPi * freq * t) * env * 0.7f;
    }
    return buildSoundFromSamples(s);
}

Sound SynthAudio::generateBlockImpact() {
    int count = static_cast<int>(kSampleRate * 0.30f);
    std::vector<float> s(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        float env = std::exp(-t * 14.0f);
        s[static_cast<size_t>(i)] = (std::sin(kTwoPi * 95.0f * t) * 0.6f + randNoise() * 0.5f) * env;
    }
    return buildSoundFromSamples(s);
}

Sound SynthAudio::generateStructuralFracture() {
    int count = static_cast<int>(kSampleRate * 0.75f);
    std::vector<float> s(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        float env = std::exp(-t * 4.5f);
        float groan = std::sin(kTwoPi * (68.0f - t * 25.0f) * t);
        s[static_cast<size_t>(i)] = (groan * 0.65f + randNoise() * 0.4f) * env;
    }
    return buildSoundFromSamples(s);
}

Sound SynthAudio::generateLeviathanRoar() {
    int count = static_cast<int>(kSampleRate * 1.4f);
    std::vector<float> s(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        float env = std::sin(std::min(1.0f, t * 4.0f) * (kPi * 0.5f)) * std::exp(-t * 1.8f);
        float mod = std::sin(kTwoPi * 5.5f * t) * 18.0f;
        float call = std::sin(kTwoPi * (82.0f + mod) * t) + 0.5f * std::sin(kTwoPi * (164.0f + mod) * t);
        s[static_cast<size_t>(i)] = call * 0.45f * env;
    }
    return buildSoundFromSamples(s);
}

Sound SynthAudio::generateDiscoveryChime() {
    int count = static_cast<int>(kSampleRate * 0.5f);
    std::vector<float> s(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        float env = std::exp(-t * 6.0f);
        float freq = (t < 0.16f) ? 523.25f : 659.25f;
        s[static_cast<size_t>(i)] = std::sin(kTwoPi * freq * t) * 0.4f * env;
    }
    return buildSoundFromSamples(s);
}

Sound SynthAudio::generateSkyVoyageMusicLoop() {
    const float duration = 8.0f;
    int count = static_cast<int>(kSampleRate * duration);
    std::vector<float> s(static_cast<size_t>(count));

    const float chords[4][3] = {
        {146.83f, 220.00f, 293.66f}, // D5 open fifth
        {130.81f, 196.00f, 261.63f}, // C5
        {110.00f, 164.81f, 220.00f}, // A4
        {130.81f, 196.00f, 293.66f}  // Csus2
    };

    for (int i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        int bar = static_cast<int>(t / 2.0f) % 4;
        float barTime = std::fmod(t, 2.0f);
        float padEnv = std::sin((barTime / 2.0f) * kPi);

        float pad = 0.0f;
        for (int c = 0; c < 3; ++c) {
            pad += std::sin(kTwoPi * chords[bar][c] * t) * 0.12f;
        }

        // Gentle sky-wind shimmer
        float pulse = std::sin(kTwoPi * (chords[bar][2] * 2.0f) * t) * 0.04f * std::max(0.0f, std::sin(t * kTwoPi * 2.0f));
        s[static_cast<size_t>(i)] = (pad * padEnv + pulse);
    }
    return buildSoundFromSamples(s);
}

} // namespace aethernaut

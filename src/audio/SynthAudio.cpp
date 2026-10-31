#include "audio/SynthAudio.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace aethernaut {

namespace {

constexpr int kSampleRate = 44100;

Sound buildSoundFromSamples(const std::vector<float>& monoSamples, bool addReverbTail = true) {
    size_t n = monoSamples.size();
    std::vector<float> left(n, 0.0f);
    std::vector<float> right(n, 0.0f);

    // Apply stereo spatial widening + multi-tap comb reverb
    const int d1L = 1499, d2L = 2213, d3L = 3187;
    const int d1R = 1627, d2R = 2389, d3R = 3413;
    for (size_t i = 0; i < n; ++i) {
        float dry = monoSamples[i];
        float wetL = 0.0f;
        float wetR = 0.0f;
        if (addReverbTail) {
            if (i >= d1L) wetL += left[i - d1L] * 0.28f;
            if (i >= d2L) wetL += left[i - d2L] * 0.19f;
            if (i >= d3L) wetL += left[i - d3L] * 0.12f;

            if (i >= d1R) wetR += right[i - d1R] * 0.28f;
            if (i >= d2R) wetR += right[i - d2R] * 0.19f;
            if (i >= d3R) wetR += right[i - d3R] * 0.12f;
        }
        left[i] = std::tanh(dry * 0.85f + wetL * 0.35f);
        right[i] = std::tanh(dry * 0.85f + wetR * 0.35f);
    }

    Wave w{};
    w.frameCount = static_cast<unsigned int>(n);
    w.sampleRate = kSampleRate;
    w.sampleSize = 16;
    w.channels = 2;

    short* pcm = static_cast<short*>(RL_MALLOC(n * 2 * sizeof(short)));
    for (size_t i = 0; i < n; ++i) {
        float cL = std::clamp(left[i], -1.0f, 1.0f);
        float cR = std::clamp(right[i], -1.0f, 1.0f);
        pcm[i * 2 + 0] = static_cast<short>(cL * 32000.0f);
        pcm[i * 2 + 1] = static_cast<short>(cR * 32000.0f);
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
        SetSoundVolume(musicLoop_, musicVol * 0.48f);
        PlaySound(musicLoop_);
    } else {
        SetSoundVolume(musicLoop_, musicVol * 0.48f);
    }

    for (const auto& ev : world.pendingAudioEvents) {
        Vec3 rel = ev.worldPos - listenerPos;
        float dist = rel.length();
        float atten = std::clamp(1.0f - (dist / 980.0f), 0.1f, 1.0f) * sfxVol;
        if (atten <= 0.02f) continue;

        // Compute stereo pan [0.0 = right, 0.5 = center, 1.0 = left in Raylib]
        float pan = std::clamp(0.5f - (rel.x / std::max(45.0f, dist)) * 0.42f, 0.1f, 0.9f);

        auto playSpatial = [&](Sound& snd, float gain, float pitchVar) {
            SetSoundVolume(snd, atten * gain);
            SetSoundPan(snd, pan);
            SetSoundPitch(snd, pitchVar);
            PlaySound(snd);
        };

        float jitter = 0.94f + (static_cast<float>(std::rand() % 120) / 1000.0f);

        switch (ev.type) {
            case SimAudioEventType::CannonFire:
                playSpatial(sfxCannon_, 0.78f, jitter);
                break;
            case SimAudioEventType::HarpoonLaunch:
                playSpatial(sfxHarpoon_, 0.85f, jitter);
                break;
            case SimAudioEventType::HarpoonLatch:
                playSpatial(sfxLatch_, 0.90f, 1.0f);
                break;
            case SimAudioEventType::CableSnap:
                playSpatial(sfxSnap_, 0.95f, 1.0f);
                break;
            case SimAudioEventType::BlockImpact:
                playSpatial(sfxImpact_, 0.68f, jitter);
                break;
            case SimAudioEventType::StructuralFracture:
                playSpatial(sfxFracture_, 0.92f, 0.96f);
                break;
            case SimAudioEventType::LeviathanRoar:
                playSpatial(sfxRoar_, 1.0f, 0.95f);
                break;
            case SimAudioEventType::CratePickup:
            case SimAudioEventType::RepairPulse:
            case SimAudioEventType::TeslaDischarge:
                playSpatial(sfxChime_, 0.75f, 1.0f);
                break;
        }
    }
    world.pendingAudioEvents.clear();
}

Sound SynthAudio::generateCannonBoom() {
    int count = static_cast<int>(kSampleRate * 0.65f);
    std::vector<float> s(static_cast<size_t>(count));
    float phase = 0.0f;
    float lowNoise = 0.0f;
    for (int i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        float env = std::exp(-t * 6.8f);
        float punch = std::exp(-t * 32.0f);
        float freq = 148.0f * std::exp(-t * 10.5f) + 34.0f;
        phase += kTwoPi * freq / static_cast<float>(kSampleRate);
        lowNoise = lowNoise * 0.84f + randNoise() * 0.16f;
        s[static_cast<size_t>(i)] = (std::sin(phase) * 0.68f + std::sin(phase * 2.0f) * 0.25f * punch + lowNoise * 0.65f) * env;
    }
    return buildSoundFromSamples(s, true);
}

Sound SynthAudio::generateHarpoonLaunch() {
    int count = static_cast<int>(kSampleRate * 0.45f);
    std::vector<float> s(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        float env = std::exp(-t * 8.5f);
        float ratchet = std::sin(kTwoPi * (410.0f + std::sin(t * 155.0f) * 195.0f) * t);
        float chain = std::sin(kTwoPi * 1240.0f * t) * std::exp(-t * 18.0f);
        s[static_cast<size_t>(i)] = (ratchet * 0.48f + chain * 0.28f + randNoise() * 0.28f) * env;
    }
    return buildSoundFromSamples(s, true);
}

Sound SynthAudio::generateHarpoonLatch() {
    int count = static_cast<int>(kSampleRate * 0.42f);
    std::vector<float> s(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        float env = std::exp(-t * 10.5f);
        float ring = std::sin(kTwoPi * 587.33f * t) + 0.55f * std::sin(kTwoPi * 880.0f * t) + 0.35f * std::sin(kTwoPi * 1174.66f * t);
        s[static_cast<size_t>(i)] = ring * 0.46f * env;
    }
    return buildSoundFromSamples(s, true);
}

Sound SynthAudio::generateCableSnap() {
    int count = static_cast<int>(kSampleRate * 0.50f);
    std::vector<float> s(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        float env = std::exp(-t * 7.5f);
        float freq = 980.0f * std::exp(-t * 5.2f);
        s[static_cast<size_t>(i)] = (std::sin(kTwoPi * freq * t) * 0.65f + std::sin(kTwoPi * freq * 1.5f * t) * 0.25f) * env;
    }
    return buildSoundFromSamples(s, true);
}

Sound SynthAudio::generateBlockImpact() {
    int count = static_cast<int>(kSampleRate * 0.32f);
    std::vector<float> s(static_cast<size_t>(count));
    float filt = 0.0f;
    for (int i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        float env = std::exp(-t * 13.5f);
        filt = filt * 0.78f + randNoise() * 0.22f;
        s[static_cast<size_t>(i)] = (std::sin(kTwoPi * 98.0f * t) * 0.58f + filt * 0.55f) * env;
    }
    return buildSoundFromSamples(s, true);
}

Sound SynthAudio::generateStructuralFracture() {
    int count = static_cast<int>(kSampleRate * 0.85f);
    std::vector<float> s(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        float env = std::exp(-t * 4.2f);
        float groan = std::sin(kTwoPi * (72.0f - t * 24.0f) * t) + 0.45f * std::sin(kTwoPi * (144.0f - t * 42.0f) * t);
        s[static_cast<size_t>(i)] = (groan * 0.58f + randNoise() * 0.32f) * env;
    }
    return buildSoundFromSamples(s, true);
}

Sound SynthAudio::generateLeviathanRoar() {
    int count = static_cast<int>(kSampleRate * 1.65f);
    std::vector<float> s(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        float env = std::sin(std::min(1.0f, t * 3.8f) * (kPi * 0.5f)) * std::exp(-t * 1.55f);
        float mod = std::sin(kTwoPi * 5.2f * t) * 16.0f;
        float call = std::sin(kTwoPi * (73.42f + mod) * t)
                   + 0.58f * std::sin(kTwoPi * (146.83f + mod * 1.5f) * t)
                   + 0.32f * std::sin(kTwoPi * (220.0f + mod * 2.0f) * t);
        s[static_cast<size_t>(i)] = call * 0.42f * env;
    }
    return buildSoundFromSamples(s, true);
}

Sound SynthAudio::generateDiscoveryChime() {
    int count = static_cast<int>(kSampleRate * 0.65f);
    std::vector<float> s(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        float env = std::exp(-t * 5.2f);
        float f1 = (t < 0.14f) ? 587.33f : ((t < 0.28f) ? 739.99f : 880.0f);
        float bell = std::sin(kTwoPi * f1 * t) + 0.35f * std::sin(kTwoPi * f1 * 2.0f * t);
        s[static_cast<size_t>(i)] = bell * 0.38f * env;
    }
    return buildSoundFromSamples(s, true);
}

Sound SynthAudio::generateSkyVoyageMusicLoop() {
    const float duration = 16.0f;
    int count = static_cast<int>(kSampleRate * duration);
    std::vector<float> s(static_cast<size_t>(count));

    // 4-bar heroic sky-voyage progression (D Dorian / Aeolian brass + aether crystalline arpeggio)
    const float chords[4][4] = {
        {73.42f, 146.83f, 220.00f, 329.63f}, // D5 add9
        {65.41f, 130.81f, 196.00f, 293.66f}, // Cmaj9
        {58.27f, 116.54f, 174.61f, 261.63f}, // Bbmaj7
        {55.00f, 110.00f, 164.81f, 246.94f}  // Asus2/E
    };

    const float arpNotes[8] = {
        293.66f, 329.63f, 440.00f, 587.33f,
        523.25f, 440.00f, 349.23f, 329.63f
    };

    float windFilter = 0.0f;
    for (int i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        int bar = static_cast<int>(t / 4.0f) % 4;
        float barTime = std::fmod(t, 4.0f);
        float padEnv = std::sin((barTime / 4.0f) * kPi);

        // Warm brass-pad harmonic voices
        float pad = 0.0f;
        for (int c = 0; c < 4; ++c) {
            float f = chords[bar][c];
            float detune = 1.0f + 0.0018f * std::sin(kTwoPi * 0.35f * t + static_cast<float>(c));
            pad += (std::sin(kTwoPi * f * detune * t) + 0.28f * std::sin(kTwoPi * f * 2.0f * t)) * 0.085f;
        }

        // Crystalline Aether arpeggio pulse (every 0.5s)
        int stepIdx = static_cast<int>(t / 0.5f) % 8;
        float stepTime = std::fmod(t, 0.5f);
        float arpEnv = std::exp(-stepTime * 5.8f);
        float arpFreq = arpNotes[stepIdx];
        float arp = (std::sin(kTwoPi * arpFreq * t) + 0.35f * std::sin(kTwoPi * arpFreq * 2.0f * t)) * 0.065f * arpEnv;

        // Soft trade-wind breeze bed
        windFilter = windFilter * 0.96f + randNoise() * 0.04f;
        float wind = windFilter * (0.045f + 0.02f * std::sin(kTwoPi * 0.125f * t));

        s[static_cast<size_t>(i)] = pad * padEnv + arp + wind;
    }
    return buildSoundFromSamples(s, true);
}

} // namespace aethernaut

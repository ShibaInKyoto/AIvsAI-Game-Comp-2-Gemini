// Procedural Sound Effects & Dynamic Synthwave Audio Engine
class SoundEngine {
  private ctx: AudioContext | null = null;
  private masterGain: GainNode | null = null;
  private sfxGain: GainNode | null = null;
  private musicGain: GainNode | null = null;
  private filterNode: BiquadFilterNode | null = null;
  private isMusicPlaying = false;
  private musicStep = 0;
  private musicInterval: any = null;
  private currentSector = 1;
  private sfxVolume = 0.7;
  private musicVolume = 0.5;

  private initContext() {
    if (!this.ctx) {
      const AudioCtx = window.AudioContext || (window as any).webkitAudioContext;
      if (AudioCtx) {
        this.ctx = new AudioCtx();
        this.masterGain = this.ctx.createGain();
        this.sfxGain = this.ctx.createGain();
        this.musicGain = this.ctx.createGain();
        this.filterNode = this.ctx.createBiquadFilter();

        this.filterNode.type = "lowpass";
        this.filterNode.frequency.value = 18000;

        this.sfxGain.gain.value = this.sfxVolume;
        this.musicGain.gain.value = this.musicVolume;

        this.sfxGain.connect(this.filterNode);
        this.musicGain.connect(this.filterNode);
        this.filterNode.connect(this.masterGain);
        this.masterGain.connect(this.ctx.destination);
      }
    }
    if (this.ctx && this.ctx.state === "suspended") {
      this.ctx.resume();
    }
  }

  public setVolumes(sfx: number, music: number) {
    this.sfxVolume = sfx;
    this.musicVolume = music;
    if (this.sfxGain) this.sfxGain.gain.value = sfx;
    if (this.musicGain) this.musicGain.gain.value = music;
  }

  public setTimeDilation(slomo: boolean) {
    if (!this.filterNode || !this.ctx) return;
    const now = this.ctx.currentTime;
    if (slomo) {
      this.filterNode.frequency.cancelScheduledValues(now);
      this.filterNode.frequency.linearRampToValueAtTime(800, now + 0.1);
    } else {
      this.filterNode.frequency.cancelScheduledValues(now);
      this.filterNode.frequency.linearRampToValueAtTime(18000, now + 0.2);
    }
  }

  // Laser zap sound
  public playLaser(pitchMultiplier = 1.0) {
    this.initContext();
    if (!this.ctx || !this.sfxGain || this.sfxVolume <= 0) return;

    const now = this.ctx.currentTime;
    const osc = this.ctx.createOscillator();
    const gain = this.ctx.createGain();

    osc.type = "sawtooth";
    osc.frequency.setValueAtTime(880 * pitchMultiplier, now);
    osc.frequency.exponentialRampToValueAtTime(110, now + 0.12);

    gain.gain.setValueAtTime(0.18, now);
    gain.gain.exponentialRampToValueAtTime(0.001, now + 0.12);

    osc.connect(gain);
    gain.connect(this.sfxGain);

    osc.start(now);
    osc.stop(now + 0.13);
  }

  // Heavy railgun / plasma cannon blast
  public playRailgun() {
    this.initContext();
    if (!this.ctx || !this.sfxGain || this.sfxVolume <= 0) return;

    const now = this.ctx.currentTime;
    const osc = this.ctx.createOscillator();
    const gain = this.ctx.createGain();

    osc.type = "triangle";
    osc.frequency.setValueAtTime(320, now);
    osc.frequency.exponentialRampToValueAtTime(45, now + 0.25);

    gain.gain.setValueAtTime(0.4, now);
    gain.gain.exponentialRampToValueAtTime(0.001, now + 0.25);

    osc.connect(gain);
    gain.connect(this.sfxGain);

    osc.start(now);
    osc.stop(now + 0.26);
  }

  // Explosion sound with noise buffer
  public playExplosion(isLarge = false) {
    this.initContext();
    if (!this.ctx || !this.sfxGain || this.sfxVolume <= 0) return;

    const now = this.ctx.currentTime;
    const duration = isLarge ? 0.6 : 0.35;

    // Sub-bass boom
    const osc = this.ctx.createOscillator();
    const oscGain = this.ctx.createGain();
    osc.type = "sine";
    osc.frequency.setValueAtTime(isLarge ? 120 : 160, now);
    osc.frequency.exponentialRampToValueAtTime(28, now + duration);

    oscGain.gain.setValueAtTime(isLarge ? 0.6 : 0.35, now);
    oscGain.gain.exponentialRampToValueAtTime(0.001, now + duration);

    osc.connect(oscGain);
    oscGain.connect(this.sfxGain);
    osc.start(now);
    osc.stop(now + duration);

    // Procedural noise burst
    const bufferSize = this.ctx.sampleRate * duration;
    const buffer = this.ctx.createBuffer(1, bufferSize, this.ctx.sampleRate);
    const data = buffer.getChannelData(0);
    for (let i = 0; i < bufferSize; i++) {
      data[i] = (Math.random() * 2 - 1) * Math.exp(-3 * (i / bufferSize));
    }

    const noise = this.ctx.createBufferSource();
    noise.buffer = buffer;

    const noiseFilter = this.ctx.createBiquadFilter();
    noiseFilter.type = "lowpass";
    noiseFilter.frequency.setValueAtTime(isLarge ? 400 : 800, now);
    noiseFilter.frequency.exponentialRampToValueAtTime(80, now + duration);

    const noiseGain = this.ctx.createGain();
    noiseGain.gain.setValueAtTime(isLarge ? 0.5 : 0.25, now);
    noiseGain.gain.exponentialRampToValueAtTime(0.001, now + duration);

    noise.connect(noiseFilter);
    noiseFilter.connect(noiseGain);
    noiseGain.connect(this.sfxGain);

    noise.start(now);
    noise.stop(now + duration);
  }

  // Dash warp whoosh
  public playDash() {
    this.initContext();
    if (!this.ctx || !this.sfxGain || this.sfxVolume <= 0) return;

    const now = this.ctx.currentTime;
    const osc = this.ctx.createOscillator();
    const gain = this.ctx.createGain();

    osc.type = "sine";
    osc.frequency.setValueAtTime(140, now);
    osc.frequency.exponentialRampToValueAtTime(680, now + 0.12);
    osc.frequency.exponentialRampToValueAtTime(200, now + 0.2);

    gain.gain.setValueAtTime(0.3, now);
    gain.gain.exponentialRampToValueAtTime(0.001, now + 0.2);

    osc.connect(gain);
    gain.connect(this.sfxGain);

    osc.start(now);
    osc.stop(now + 0.21);
  }

  // Data shard pickup chime
  public playShardPickup() {
    this.initContext();
    if (!this.ctx || !this.sfxGain || this.sfxVolume <= 0) return;

    const now = this.ctx.currentTime;
    const osc = this.ctx.createOscillator();
    const gain = this.ctx.createGain();

    osc.type = "sine";
    const baseFreq = 523.25; // C5
    const freqs = [baseFreq, baseFreq * 1.25, baseFreq * 1.5];
    const picked = freqs[Math.floor(Math.random() * freqs.length)];

    osc.frequency.setValueAtTime(picked, now);
    osc.frequency.exponentialRampToValueAtTime(picked * 1.4, now + 0.08);

    gain.gain.setValueAtTime(0.12, now);
    gain.gain.exponentialRampToValueAtTime(0.001, now + 0.09);

    osc.connect(gain);
    gain.connect(this.sfxGain);

    osc.start(now);
    osc.stop(now + 0.1);
  }

  // Level up fanfare
  public playLevelUp() {
    this.initContext();
    if (!this.ctx || !this.sfxGain || this.sfxVolume <= 0) return;

    const notes = [261.63, 329.63, 392.0, 523.25, 659.25]; // C major pentatonic
    notes.forEach((freq, idx) => {
      if (!this.ctx) return;
      const startTime = this.ctx.currentTime + idx * 0.07;
      const osc = this.ctx.createOscillator();
      const gain = this.ctx.createGain();

      osc.type = "triangle";
      osc.frequency.setValueAtTime(freq, startTime);

      gain.gain.setValueAtTime(0.25, startTime);
      gain.gain.exponentialRampToValueAtTime(0.001, startTime + 0.25);

      osc.connect(gain);
      gain.connect(this.sfxGain!);

      osc.start(startTime);
      osc.stop(startTime + 0.26);
    });
  }

  // Dynamic procedural synthwave background music engine
  public startMusic(sector = 1) {
    this.initContext();
    this.currentSector = sector;
    if (this.isMusicPlaying) return;
    this.isMusicPlaying = true;
    this.musicStep = 0;

    // Root bass notes per sector progression (A minor, F, G, E minor)
    const progressions: Record<number, number[]> = {
      1: [110.0, 110.0, 87.31, 98.0], // A2, A2, F2, G2
      2: [123.47, 98.0, 87.31, 110.0], // B2, G2, F2, A2
      3: [73.42, 87.31, 98.0, 110.0], // D2, F2, G2, A2
      4: [65.41, 73.42, 87.31, 98.0], // C2, D2, F2, G2
      5: [55.0, 65.41, 73.42, 82.41], // A1, C2, D2, E2
    };

    const arpScales: Record<number, number[]> = {
      1: [220, 261.63, 329.63, 392.0, 440, 523.25],
      2: [246.94, 293.66, 369.99, 440, 493.88],
      3: [293.66, 349.23, 440, 523.25, 587.33],
      4: [261.63, 329.63, 392.0, 493.88, 523.25],
      5: [220, 261.63, 329.63, 392.0, 440, 659.25],
    };

    const stepDurationMs = 140; // ~107 BPM 16th notes

    this.musicInterval = setInterval(() => {
      if (!this.ctx || !this.musicGain || this.musicVolume <= 0 || !this.isMusicPlaying) return;

      const now = this.ctx.currentTime;
      const prog = progressions[this.currentSector] || progressions[1];
      const arp = arpScales[this.currentSector] || arpScales[1];

      const barIndex = Math.floor((this.musicStep / 16) % prog.length);
      const rootNote = prog[barIndex];
      const stepInBar = this.musicStep % 16;

      // 1. Kick on 0, 4, 8, 12
      if (stepInBar % 4 === 0) {
        const kickOsc = this.ctx.createOscillator();
        const kickGain = this.ctx.createGain();
        kickOsc.type = "sine";
        kickOsc.frequency.setValueAtTime(140, now);
        kickOsc.frequency.exponentialRampToValueAtTime(35, now + 0.12);

        kickGain.gain.setValueAtTime(0.3, now);
        kickGain.gain.exponentialRampToValueAtTime(0.001, now + 0.12);

        kickOsc.connect(kickGain);
        kickGain.connect(this.musicGain);
        kickOsc.start(now);
        kickOsc.stop(now + 0.13);
      }

      // 2. Snare / Clack on 4, 12
      if (stepInBar === 4 || stepInBar === 12) {
        const snareOsc = this.ctx.createOscillator();
        const snareGain = this.ctx.createGain();
        snareOsc.type = "triangle";
        snareOsc.frequency.setValueAtTime(220, now);
        snareOsc.frequency.exponentialRampToValueAtTime(80, now + 0.08);

        snareGain.gain.setValueAtTime(0.15, now);
        snareGain.gain.exponentialRampToValueAtTime(0.001, now + 0.08);

        snareOsc.connect(snareGain);
        snareGain.connect(this.musicGain);
        snareOsc.start(now);
        snareOsc.stop(now + 0.09);
      }

      // 3. Rolling Bassline on 8th notes
      if (stepInBar % 2 === 0) {
        const bassOsc = this.ctx.createOscillator();
        const bassGain = this.ctx.createGain();
        bassOsc.type = "sawtooth";
        const bassFreq = stepInBar === 6 || stepInBar === 14 ? rootNote * 1.5 : rootNote;
        bassOsc.frequency.setValueAtTime(bassFreq, now);

        bassGain.gain.setValueAtTime(0.2, now);
        bassGain.gain.exponentialRampToValueAtTime(0.001, now + 0.1);

        const bassFilter = this.ctx.createBiquadFilter();
        bassFilter.type = "lowpass";
        bassFilter.frequency.value = 550;

        bassOsc.connect(bassFilter);
        bassFilter.connect(bassGain);
        bassGain.connect(this.musicGain);

        bassOsc.start(now);
        bassOsc.stop(now + 0.11);
      }

      // 4. Arpeggiator sparkle
      const noteIndex = (this.musicStep * 3) % arp.length;
      const arpFreq = arp[noteIndex];
      const arpOsc = this.ctx.createOscillator();
      const arpGain = this.ctx.createGain();

      arpOsc.type = "square";
      arpOsc.frequency.setValueAtTime(arpFreq, now);

      arpGain.gain.setValueAtTime(0.06, now);
      arpGain.gain.exponentialRampToValueAtTime(0.001, now + 0.08);

      const arpFilter = this.ctx.createBiquadFilter();
      arpFilter.type = "bandpass";
      arpFilter.frequency.value = 1400;

      arpOsc.connect(arpFilter);
      arpFilter.connect(arpGain);
      arpGain.connect(this.musicGain);

      arpOsc.start(now);
      arpOsc.stop(now + 0.09);

      this.musicStep++;
    }, stepDurationMs);
  }

  public stopMusic() {
    this.isMusicPlaying = false;
    if (this.musicInterval) {
      clearInterval(this.musicInterval);
      this.musicInterval = null;
    }
  }
}

export const sound = new SoundEngine();

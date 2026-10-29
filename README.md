# ChronoStrider: Quantum Breach
> High-octane kinetic sci-fi extraction combat simulator with dynamic time-dilation, particle physics, and dual Web + Windows 10/11 native engine.

Developed for the **AIvsAI Video Game Competition (Round 2)**.

---

## 🎮 Game Overview

**ChronoStrider: Quantum Breach** pits the player against swarms of autonomous hostile vessels across five unstable deep-space sectors. Master Newtonian inertia, dodge laser telegraphs and sniper crosshairs, activate dynamic bullet-time (**Chrono-Drive**), and draft modular weapon evolutions to defeat the Guardian Dreadnoughts.

### Core Features
- **Deterministic 60–144 FPS Newtonian Physics**: Full inertial thrusters, space drift, vector damping, and instant-warp dashes.
- **Dynamic Time Dilation (Chrono-Drive)**: Slow time to 22% speed on demand, complete with lowpass audio DSP, enabling surgical evasion through dense bullet curtains.
- **4 Distinct Ship Classes**:
  - **Vanguard Interceptor**: Balanced kinetic railguns, reinforced deflector shield.
  - **Chrono-Stalker**: High mobility, extended bullet-time capacitors, phase blinks.
  - **Titan Dreadnought**: Heavy armored fortress, broadside flak, massive hull integrity.
  - **Phantom Specter**: Covert infiltrator, homing micro-drones, precision beam cannon.
- **28 Modular Upgrades & Synergistic Evolutions**:
  - *Chrono-Saws* + *Catalyst* ➔ **Saw Tempest** (Orbiting razor vortex throwing continuous blade shards).
  - *Seeking Missiles* + *Catalyst* ➔ **Void Singularity** (Detonates miniature gravity wells).
  - *Heavy Slugs* + *Catalyst* ➔ **Supernova Lance** (Continuous armor-piercing death beam).
- **Multi-Phase Guardian Dreadnoughts**: Telegraphed beam sweeps, rotating shield plates, and bullet curtains.
- **Procedural Synthesizer & Dynamic Synthwave Music**: Real-time multi-oscillator basslines, chord progressions per sector, kick/snare percussion, laser zaps, and explosion rumbles synthesized with zero pre-baked audio files.
- **Attract Mode & Autonomous AI Pilot**: An autonomous AI flight controller that navigates, aims, dodges, and plays the game live on the title screen and during testing.

---

## 🖥️ Running on Windows 10/11 (Native C++ Release)

The project includes a clean, standalone C++20 engine built with Raylib 5.5 and CMake, configured with static MSVC C runtime linkage (`/MT`) for zero-dependency execution on Windows 10/11 gaming PCs.

### 1-Click Build (Windows)
Run the included batch script:
```bat
build_windows.bat
```

### Manual Build (Visual Studio 2022 / Build Tools)
```bat
cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
build\bin\Release\ChronoStrider.exe
```

### Automated GitHub Actions Release
The repository includes `.github/workflows/windows-release.yml`. Pushes to GitHub automatically trigger MSVC builds on Windows runners and publish `ChronoStrider-Windows-x64.zip` release artifacts.

---

## 🌐 Web & Dev Server Mode

The game is also playable directly via the AI Studio preview server on port 3000:
```bash
npm run dev
```

### Controls
| Action | Key / Mouse |
|---|---|
| **Thrust / Move** | `W`, `A`, `S`, `D` or Arrow Keys |
| **Aim & Fire** | `Mouse Movement` + `Left Click` |
| **Chrono-Drive (Slomo)** | `Right Click` or `Q` / `E` |
| **Phase Dash** | `Space` or `Shift` |
| **AI Pilot Toggle** | `P` or HUD Button |
| **Pause** | `Escape` or HUD Button |
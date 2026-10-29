# AETHERNAUT: 3D Sky-Fleet Architect & Colossal Leviathan Hunter
> **Native C++20 & OpenGL 3.3 3D Physics-Driven Airship Engineering, Open-Sky Naval Broadside, and Colossal Creature-Hunting Action-Simulation for Windows 10/11.**

Developed for the **AI vs. AI Video Game Competition (Round 2)** (`ShibaInKyoto/AIvsAI-Game-Comp-2-Gemini`).

---

##Screenshots & Gameplay Modes

| Command Deck & 3D Flagship Turntable | Colossal 3D Sky-Leviathan Harpoon Hunt |
| :---: | :---: |
| ![Title Screen](docs/screenshots/title.png) | ![Leviathan Hunt](docs/screenshots/leviathan_hunt.png) |
| **Interactive 3D Drydock Shipyard & Voxel Architect** | **Sky-Armada 3D Fleet Broadside Skirmish** |
| ![Drydock Shipyard](docs/screenshots/drydock_shipyard.png) | ![Fleet Skirmish](docs/screenshots/fleet_skirmish.png) |

---

## Game Overview & Core Engineering Pillar

**AETHERNAUT** is a 100% native **C++20 + OpenGL 3.3 (Raylib 5.5 + Custom GLSL 330 Shaders)** 3D action-engineering and sky-naval hunting game inspired by *From the Depths*, *Besiege*, *Guns of Icarus*, and *Shadow of the Colossus*.

Every airship in the sky—both the player's flagship and every AI-controlled vessel—is constructed from a physical **3D Voxel Block Grid (`15 × 11 × 23`)** where every block contributes real mass, buoyancy lift, power generation/consumption, thrust, rotational torque, and localized armor:

### 1. Real-Time 3D Buoyancy, Center-of-Mass / Center-of-Lift & BFS Structural Fracture
- **Dynamic 3D Physics & Listing Trim**: Whenever blocks are placed in the Drydock or blown off in combat, the ship recomputes its **Total Mass**, **3D Center of Mass (CoM)**, and **3D Center of Lift (CoL)**. If port-side *Aether Lift Cells* are punctured by enemy broadsides, the vessel physically rolls and lists toward the damaged side; if total lift drops below weight, the airship begins sinking toward the cloud abyss.
- **3D Breadth-First Search (BFS) Structural Connectivity Fracture**: Every time a block is destroyed in combat, a 6-neighbor 3D BFS flood-fill traverses outward from the *Captain's Command Bridge*. Any wing, outrigger pontoon, or gun battery whose structural struts have been severed is **physically detached from the ship** and tumbles into the clouds below as rotating 3D falling debris (`DetachedDebris`).

### 2. Colossal Multi-Segment 3D Sky-Leviathans & Physical Harpoon Winches
Hunt four species of multi-segmented 3D **Sky-Leviathans** swimming through the cloud sea using sinusoidal 3D spine kinematics:
1. **Elder Sky-Whale (*Cetus Aetheria*)** — 18-segment armored behemoth with sweeping pectoral fins and sonic Aether pulse attacks.
2. **Crimson Storm-Wyrm (*Draconis Fulmen*)** — 24-segment high-speed serpentine dragon that weaves through storm clouds and spits chain-lightning bolts.
3. **Obsidian Dread-Manta (*Raja Vorago*)** — 16-segment wide-winged fortress manta with heavy carapace armor.
4. **The Maelstrom Sovereign (*Apex Charybdis*)** — 30-segment Elder Apex boss guarding the northern Maelstrom.

- **Hooke's Law Harpoon Tether Physics**: Fire **Steam Harpoon Winches (`Right Click`)** to embed steel cables into specific 3D segments of a Leviathan. Once tethered, the Leviathan drags your airship through the sky while your winches apply counter-drag and reel the beast in (`Hold [R]`). Shatter outer carapace plates with broadsides to expose glowing **Aether Core weak spots ($2.4\times$ critical damage)**.

### 3. Interactive 3D Sky-Drydock Shipyard & Custom Blueprint Persistence
- Dock at any **Sky-Port Island** (`[B]`) or launch **3D Drydock Sandbox Mode** to design custom airships in 3D using **12 functional block types**:
  - `[1]` *Heartwood Timber Hull* | `[2]` *Ironclad Bulkhead* | `[3]` *Pressurized Aether Lift Cell* | `[4]` *Aetherium Reactor Core*
  - `[5]` *Brass Turbine Propeller* | `[6]` *Gyro-Stabilizer & Rudder Sail* | `[7]` *Broadside Siege Cannon* | `[8]` *Steam Harpoon Winch*
  - `[9]` *Voltaic Arc Spire (Tesla)* | `[0]` *Sky-Sweeper Flak Turret* | `[-]` *Automated Steam Repair Rig* | `[=]` *Captain's Command Bridge*
- Features **X-Axis Symmetry Mirroring (`[M]`)**, live **3D Center-of-Mass / Center-of-Lift Engineering Gizmos (`[G]`)**, **6 built-in multi-deck hull presets**, and disk persistence (`[F5]` Save / `[F9]` Load to `custom_ship.aeth`).

### 4. Custom GLSL 330 Shaders & Real-Time Procedural PCM Audio Synthesizer
- **Custom GLSL 330 Pipeline**: `assets/shaders/lit_world.vs.glsl`, `lit_world.fs.glsl` (directional sun + sky/ground hemisphere ambient + Blinn-Phong specular + Fresnel rim + exponential height/distance cloud fog), and `post_hdr.fs.glsl` (9-tap HDR bloom + ACES filmic tone-mapping + storm lightning flashes).
- **Zero-Dependency C++ Audio Synth (`SynthAudio`)**: Generates 44.1 kHz stereo PCM audio in real time for cannon broadside booms, mechanical harpoon winch ratchets, cable snaps, timber/iron structural fractures, FM-synthesized Leviathan roars, and dynamic sky-voyage brass/string ambient chords.

---

## Controls

| Action | Flight / Combat Mode | 3D Drydock Shipyard Mode |
| :--- | :--- | :--- |
| **Throttle Forward / Reverse** | `W` / `S` (or `Up` / `Down`) | — |
| **Rudder Yaw Port / Starboard** | `A` / `D` (or `Left` / `Right`) | — |
| **Ascend / Descend Cruise Altitude** | `Space` / `Left Ctrl` (or `C`) | — |
| **Fire Broadside Cannons & Turrets** | `Hold Left Mouse Button (LMB)` | `LMB`: Place 3D Voxel Block |
| **Fire Steam Harpoon Winches** | `Right Mouse Button (RMB)` | `RMB`: Remove Block (Refunds Cost) |
| **Reel In / Cut Harpoon Cables** | `Hold [R]` Reel In / `[F]` Cut Cables | `[R]`: Rotate Block Facing (90°) |
| **Orbit / Zoom 3D Camera** | `Q` / `E` Orbit, `Mouse Wheel` Zoom | `RMB Drag` or `Q`/`E` Orbit, `Wheel` Zoom |
| **Open / Exit 3D Drydock Shipyard** | `[B]` (Near Port or in Sandbox) | `[B]` or `ESC` to Undock |
| **Walk on Deck / Return to Helm** | `[TAB]` (Walk & Passively Repair Hull) | `[M]` X-Mirror \| `[G]` CoM/CoL Gizmos |
| **Save / Load Custom Ship Blueprint** | — | `[F5]` Save Ship \| `[F9]` Load Ship |
| **World Navigation Chart & Bounties** | `[M]` Toggle Archipelago Map | `[1]..[0], [-], [=]` Select Block |
| **AI Autopilot / Performance HUD** | `[P]` Toggle AI Pilot \| `[F3]` Perf HUD | `[F3]` Perf HUD |

---

## Building & Running on Windows 10/11 (Native x64 C++20)

The codebase has zero external runtime dependencies—`CMakeLists.txt` automatically fetches Raylib 5.5 and links the static MSVC runtime (`/MT`) on Windows so `Aethernaut.exe` runs out-of-the-box on any Windows 10/11 PC.

### Option A: One-Click Windows Build Script
```bat
build_windows.bat
```

### Option B: Manual CMake Build (Visual Studio 2022 / MSVC)
```bat
cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
build\bin\Release\Aethernaut.exe
```

### Option C: Linux / Cross-Platform CMake Build
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/bin/Aethernaut
```

### Command-Line Options (Testing, Benchmarking & Screenshot Automation)
```text
Aethernaut.exe [options]
  --scene title|flight|drydock|leviathan|skirmish   Launch directly into a specific mode
  --ship 0..5                                       Select starting flagship blueprint (0=Zephyr .. 4=Dreadnought)
  --size <WxH>                                      Set resolution (default: 1600x900)
  --fullscreen / --windowed                         Toggle fullscreen or windowed mode
  --no-audio                                        Run with silent audio device
  --perf                                            Enable FPS & physics telemetry overlay
  --autotest <seconds>                              Run autonomous AI pilot for N seconds and exit cleanly
  --screenshot <filepath.png>                       Export rendered OpenGL frame to PNG
  --screenshot-at <seconds>                         Timestamp (in seconds) to capture screenshot
```

---

## Repository Architecture

```text
├── CMakeLists.txt                  # C++20 CMake build (Aethernaut, aethernaut_sim, tests, benchmark)
├── build_windows.bat               # 1-click Windows 10/11 MSVC Release build script
├── .github/workflows/              # Automated Windows x64 Release CI/CD packaging
├── assets/shaders/                 # Custom GLSL 330 directional/hemisphere/fog & HDR bloom shaders
├── src/
│   ├── main.cpp                    # Application lifecycle, 120Hz fixed-step loop, camera & scene routing
│   ├── core/
│   │   ├── Math3D.h / .cpp         # Vec3, Int3, Quat, Ray3D, AABB3D, Ray-AABB/Sphere, PCG32 RNG
│   │   ├── SpatialHash3D.h / .cpp  # O(1) 3D spatial hash grid for broadphase collision queries
│   │   └── AppConfig.h / .cpp      # CLI parser for scenes, resolutions, autotest & screenshot flags
│   ├── sim/
│   │   ├── BlockRegistry.h / .cpp  # 12 airship block types with mass, lift, thrust, power & armor specs
│   │   ├── Airship.h / .cpp        # 3D voxel grid, CoM/CoL buoyancy physics & 3D BFS fracture solver
│   │   ├── ShipBlueprints.h / .cpp # 6 multi-deck ship presets + .aeth custom blueprint save/load
│   │   ├── Leviathan.h / .cpp      # 4 species of multi-segmented 3D Sky-Leviathans & spine kinematics
│   │   ├── Ballistics.h / .cpp     # 3D projectiles, Hooke's Law harpoon tethers & ballistic lead solver
│   │   ├── Archipelago.h / .cpp    # 14 floating 3D sky-islands, drydock ports, crystals & storm cells
│   │   ├── AIController.h / .cpp   # Autonomous 3D broadside & obstacle-avoidance flight AI
│   │   └── World.h / .cpp          # Complete 120Hz headless-testable 3D world simulation
│   ├── render/
│   │   ├── ShaderPipeline.h / .cpp # Offscreen HDR framebuffer & GLSL 330 shader uniform management
│   │   ├── MeshGen.h / .cpp        # Procedural 3D primitive & articulated block model cache
│   │   └── WorldRenderer.h / .cpp  # 3D sky-dome, craggy islands, airships, Leviathans & tethers
│   ├── audio/
│   │   └── SynthAudio.h / .cpp     # Real-time 44.1kHz stereo procedural PCM sound & music synthesizer
│   └── ui/
│       ├── DrydockEditor.h / .cpp  # Interactive 3D voxel shipyard editor, symmetry & diagnostics UI
│       └── HudRenderer.h / .cpp    # 3D ballistic lead pips, live hull voxel schematic, radar & chart
├── tests/
│   └── test_main.cpp               # Deterministic unit & integration tests (Math3D, BFS fracture, AI)
└── tools/
    └── sim_benchmark.cpp           # Headless 120Hz simulation throughput benchmark (~9,000+ sim FPS)
```
# AETHERNAUT: 3D Sky-Fleet Architect & Colossal Leviathan Hunter
> **Native C++20 & OpenGL 3.3 3D Physics-Driven Airship Engineering, Open-Sky Naval Broadside, and Colossal Creature-Hunting Action-Simulation for Windows 10/11.**

Developed for the **AI vs. AI Video Game Competition (Round 2)** (`ShibaInKyoto/AIvsAI-Game-Comp-2-Gemini`).

---

## Screenshots & Gameplay Modes

| Command Deck & 3D Flagship Turntable | Colossal 3D Sky-Leviathan Harpoon Hunt |
| :---: | :---: |
| ![Title Screen](docs/screenshots/title.png) | ![Leviathan Hunt](docs/screenshots/leviathan_hunt.png) |
| **Interactive 3D Drydock Shipyard & Voxel Architect** | **Sky-Armada 3D Fleet Broadside Skirmish** |
| ![Drydock Shipyard](docs/screenshots/drydock_shipyard.png) | ![Fleet Skirmish](docs/screenshots/fleet_skirmish.png) |

---

## Game Overview & Core Engineering Pillars

**AETHERNAUT** is a 100% native **C++20 + OpenGL 3.3 (Raylib 5.5 + Custom GLSL 330 Shaders)** 3D action-engineering and sky-naval hunting game inspired by *From the Depths*, *Besiege*, *Guns of Icarus*, and *Shadow of the Colossus*.

Every airship in the sky—both the player's flagship and every AI-controlled vessel—is constructed from a physical **3D Voxel Block Grid (`15 × 11 × 23`)** where every block contributes real mass, buoyancy lift, power generation/consumption, rechargeable 3D energy shield capacity, thrust, rotational torque, and localized armor:

### 1. Real-Time 3D Buoyancy, Energy Shield Domes & BFS Structural Fracture
- **Dynamic 3D Physics & Listing Trim**: Whenever blocks are placed in the Drydock or blown off in combat, the ship recomputes its **Total Mass**, **3D Center of Mass (CoM)**, and **3D Center of Lift (CoL)**. If port-side *Aether Lift Cells* are punctured by enemy broadsides, the vessel physically rolls and lists toward the damaged side; if total lift drops below weight, the airship begins sinking toward the cloud abyss.
- **3D Hexagonal Energy Shield Domes**: Equipping *Aether Shield Dome Projectors* surrounds your vessel with a rechargeable 3D energy barrier that intercepts incoming shells before hull blocks take structural damage.
- **3D Breadth-First Search (BFS) Structural Connectivity Fracture**: Every time a block is destroyed in combat, a 6-neighbor 3D BFS flood-fill traverses outward from the *Captain's Command Bridge* and *Aetherium Reactor Cores*. Any wing, outrigger pontoon, or gun battery whose structural struts have been severed is **physically detached from the ship** and tumbles into the clouds below as rotating 3D falling debris (`DetachedDebris`).

### 2. 6 Colossal Multi-Segment 3D Sky-Leviathans & Physical Shock Harpoon Winches
Hunt six species of multi-segmented 3D **Sky-Leviathans** (plus endless scaling **Prestige Apex Leviathans** via `[N]`) swimming through the cloud sea using sinusoidal 3D spine kinematics:
1. **Elder Sky-Whale (*Cetus Aetheria*)** — 18-segment armored behemoth with sweeping pectoral fins and sonic Aether pulse attacks.
2. **Crimson Storm-Wyrm (*Draconis Fulmen*)** — 24-segment high-speed serpentine dragon that weaves through storm clouds and spits chain-lightning bolts.
3. **Obsidian Dread-Manta (*Raja Vorago*)** — 16-segment wide-winged fortress manta with heavy carapace armor.
4. **The Maelstrom Sovereign (*Apex Charybdis*)** — 30-segment Elder Apex boss guarding the northern Maelstrom.
5. **Glacial Cryo-Kraken (*Architeuthis Borealis*)** — 22-segment crystalline frost titan that hurls sub-zero cryo-lances.
6. **Solar Phoenix-Serpent (*Ignis Quetzal*)** — 26-segment blazing solar serpent wreathed in superheated corona plasma.

- **Hooke's Law Harpoon & Shock Tether Physics (Leviathans & Enemy Airships!)**: Fire **Steam Harpoon Winches** or **Voltaic Shock Harpoons (`Right Click`)** to embed steel cables into specific 3D segments of a Leviathan *or* grapple onto enemy Pirate Airships! Shock Harpoons channel continuous 36-DPS electric arcs down the taut cable while slowing tethered targets. Hold `[R]` to reel the winch cable in (or press `[F]` to cut cables before they snap under extreme tension). Shatter outer carapace plates with broadsides, *Flame Mortars*, or *Homing Torpedo Pods* to expose glowing **Aether Core weak spots ($2.4\times$ critical damage)**.

### 3. Interactive 3D Sky-Drydock Shipyard (20 Block Types & 10 Flagship Blueprints)
- Dock at any **Sky-Port Island** (`[B]`) or launch **3D Drydock Sandbox Mode** to design custom airships in 3D using **20 functional block types**:
  - `[1]` *Heartwood Timber Hull* | `[2]` *Ironclad Bulkhead* | `[3]` *Aether Lift Cell* | `[4]` *Aetherium Reactor Core*
  - `[5]` *Brass Turbine Propeller* | `[6]` *Gyro-Stabilizer & Rudder* | `[7]` *Broadside Siege Cannon* | `[8]` *Steam Harpoon Winch*
  - `[9]` *Voltaic Arc Spire (Tesla)* | `[0]` *Sky-Sweeper Flak Turret* | `[-]` *Automated Steam Repair Rig* | `[=]` *Captain's Command Bridge*
  - `[[]` *Orichalcum Gilded Plate* | `[]]` *Aether Shield Dome Projector* | `[;]` *Solaris Plasma Lance* | `[']` *Aether-Ramjet Afterburner*
  - `[,]` *Aether-Seeker Torpedo Pod* | `[.]` *Voltaic Shock Harpoon* | `[/]` *Chrono-Aether Capacitor* | `[\]` *Dragon-Breath Flame Mortar*
- Features **X-Axis Symmetry Mirroring (`[M]`)**, live **3D Center-of-Mass / Center-of-Lift Engineering Gizmos (`[G]`)**, **10 built-in multi-deck hull presets** (including *Nemesis Torpedo Cruiser*, *Prometheus Fire-Galleon*, and *Celestial Ark-Carrier*), and disk persistence (`[F5]` Save / `[F9]` Load to `custom_ship.aeth`).

### 4. Island Commodity Exchange (`[J]`), Ancient Relics (`[L]`), Citadels & Fleet Orders (`[Z]`)
- **Sky-Island Commodity Market & Guild Bounty Ledger (`[J]`)**: Trade 4 regional commodities (*Sky-Oak Timber*, *Ironclad Ore*, *Refined Aether-Spice*, and *Leviathan Ambergris*) across 14 islands with dynamic biome supply/demand pricing, and track all 6 Sky-Guild Leviathan Bounty Contracts.
- **Fortified Sky-Citadel Sieges & Island Liberation**: Hostile Obsidian Crags, Maelstrom Monoliths, and outer Pirate Strongholds are defended by **3D Sky-Bastion Flak Towers** that track and shell approaching vessels. Destroying all Bastion Towers on a fortified island **liberates the Citadel**, erecting an Allied Sky-Beacon, opening a new Drydock Port, awarding `+350 Gold` & `+18 Aetherium`, and granting a permanent **20% commodity purchase discount** at that island!
- **5 Ancient Aether-Relics (`[L]` Relic Vault Modal)**: Awaken 5 permanent ancient fleet artifacts (*Chronos Aether-Dial*, *Leviathan Heartstone*, *Orichalcum Gyro-Compass*, *Storm-Caller Conduit*, and *Imperial Trade Charter*) for passive living-crystal hull regeneration, electrified harpoon tethers, reload haste, and trade bonuses.
- **6 Custom Fleet Hull Liveries (`[Y]` / `[P]` in Drydock)**: Customize your airship's 3D hull & gas-cell paint scheme across *Standard Oak & Brass*, *Imperial Azure Guard*, *Crimson Corsair Armada*, *Solaris Gilded Sovereign*, *Abyssal Void-Stalker*, and *Emerald Sky-Ranger*.
- **Fleet Admiral Squadron Orders (`[Z]`) & Dynamic Weather (`[T]`)**: Issue real-time tactical commands to your hired AI Escort Wingmen (*Close Escort Formation*, *Focus Flagship Target*, *Free Hunt & Harpoon*, *Defensive Perimeter*) and navigate 4 dynamic weather states (*Clear Trade Winds*, *Cirrus Mist*, *Ion-Aether Squall*, *Supercell Tempest*).
- **5 Recruitable & Promotable Officers (`[K]` Quartermaster Modal)**:
  - *Master Gunner Vance* (+16% Reload Speed & +10% Damage/Rank)
  - *Chief Engineer Lin* (+25% Shield Dome Capacity & +30% Auto-Repair Speed/Rank)
  - *Harpoon Master Ahab* (+35% Harpoon Cable Strength & +30% Reel Speed/Rank)
  - *Helmsman Kael* (+18% Turbine Thrust & +22% Turn Torque/Rank)
  - *Scholar-Navigator Lyra* (+25% Gold/Aetherium Yield & +12 Cargo Hold Capacity/Rank)
- **Active Tactical Ship Abilities**:
  - **`Hold [Left Shift]` Aether Overdrive Afterburner**: Surges forward thrust by +75% using rechargeable Overdrive Capacitor fuel.
  - **`[X]` Emergency Damage Control & Shield Surge**: Instant burst repair across all damaged blocks, douses deck fires, and overcharges the 3D Energy Shield Dome.
  - **`[C]` Acoustic Aether-Sonar Ping**: Emits an expanding 3D spherical sonar wave through the clouds that highlights targets for **+25% Bonus Damage** for 10 seconds.

### 5. Custom GLSL 330 Shaders, Dynamic Sky-Atmospheres & Procedural PCM Audio
- **Custom GLSL 330 Pipeline & 4 Sky-Atmosphere Presets (`[V]`)**: Cycle between *Golden Dawn*, *High Azure Noon*, *Crimson Sundown*, and *Starlit Aether Night* with directional sun/moon lighting, hemisphere ambient, Fresnel rim, exponential cloud fog, and 9-tap HDR ACES bloom.
- **Zero-Dependency C++ Audio Synth (`SynthAudio`)**: Generates 44.1 kHz stereo PCM audio in real time for cannon broadside booms, mechanical harpoon winch ratchets, cable snaps, timber/iron structural fractures, FM-synthesized Leviathan roars, and dynamic sky-voyage brass/string ambient chords.

---

## Controls

| Action | Flight / Combat Mode | 3D Drydock Shipyard Mode |
| :--- | :--- | :--- |
| **Throttle Forward / Reverse** | `W` / `S` (or `Up` / `Down`) | — |
| **Rudder Yaw Port / Starboard** | `A` / `D` (or `Left` / `Right`) | — |
| **Ascend / Descend Cruise Altitude** | `Space` / `Left Ctrl` | — |
| **Aether Overdrive Afterburner** | `Hold Left Shift` | — |
| **Fire Broadside Cannons & Torpedoes**| `Hold Left Mouse Button (LMB)` | `LMB`: Place 3D Voxel Block |
| **Fire Steam / Shock Harpoon Winches**| `Right Mouse Button (RMB)` | `RMB`: Remove Block (Refunds Cost) |
| **Reel In / Cut Harpoon Cables** | `Hold [R]` Reel In / `[F]` Cut Cables | `[R]`: Rotate Block Facing (90°) |
| **Damage Control / Acoustic Sonar** | `[X]` Damage Control \| `[C]` Sonar Ping | — |
| **Commodity Market & Guild Bounties** | `[J]` Open Trade & Contracts Ledger | — |
| **Ancient Relic Vault & Hull Livery** | `[L]` Relic Vault \| `[Y]` Cycle Livery | `[Y]` or `[P]` Cycle Hull Livery |
| **Squadron Orders / Dynamic Weather** | `[Z]` Cycle Fleet Order \| `[T]` Weather | — |
| **Crew Quartermaster / Hire Escort** | `[K]` Officer Roster \| `[U]` Hire Wingman | — |
| **Summon Apex Boss / Sky Atmosphere**| `[N]` Summon Leviathan \| `[V]` Sky Time | `[V]` Cycle Sky Atmosphere |
| **Orbit / Zoom 3D Camera** | `Q` / `E` Orbit, `Mouse Wheel` Zoom | `RMB Drag` or `Q`/`E` Orbit, `Wheel` Zoom |
| **Open / Exit 3D Drydock Shipyard** | `[B]` (Near Port or in Sandbox) | `[B]` or `ESC` to Undock |
| **Walk on Deck / Return to Helm** | `[TAB]` (Walk & Passively Repair Hull) | `[M]` X-Mirror \| `[G]` CoM/CoL Gizmos |
| **Save / Load Blueprint & Campaign** | `[F6]` Save Campaign \| `[F7]` Load | `[F5]` Save Ship \| `[F9]` Load Ship |
| **World Navigation Chart & Codex** | `[M]` World Map \| `[H]` Hunter's Codex | `[1..0, -, =, [, ], ;, ', ,, ., /, \]` |
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
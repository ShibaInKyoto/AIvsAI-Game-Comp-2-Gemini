# AETHERNAUT: Architectural & Engineering Design Document

## 1. High-Level Vision
**Aethernaut: Sky-Fleet Architect & Leviathan Hunter** is a 3D physics-driven airship engineering, open-sky naval broadside, and colossal creature-hunting game written in **C++20** with **OpenGL 3.3 (Raylib 5.5)** and **custom GLSL 330 shaders**.

Unlike arena shooters or survivor-likes, gameplay revolves around three interconnected physical systems:
1. **Voxel-Block 3D Airship Engineering & Buoyancy Physics**: Every airship is a 3D grid of physical components (*Heartwood Hulls*, *Ironclad Bulkheads*, *Pressurized Aether Lift Cells*, *Aetherium Reactor Cores*, *Brass Turbine Propellers*, *Gyro-Rudders*, *Broadside Cannons*, *Steam Harpoon Winches*, *Tesla Arc Spires*, *Flak Turrets*, and *Automated Repair Cranes*).
2. **Localized Structural Fracture & Listing Dynamics**: Taking localized broadside damage destroys individual 3D blocks. Losing lift cells on one side shifts the **Center of Lift (CoL)** away from the **Center of Mass (CoM)**, inducing physical roll/pitch listing. A **3D Breadth-First Search (BFS)** connectivity solver detects any sub-structures severed from the Command Bridge / Reactor Cores and spawns falling 3D rigid-body debris.
3. **Colossal Multi-Segment 3D Sky-Leviathans & Harpoon Tethers**: Roaming the 3D sky archipelago are 16-to-30-segment creatures animated via 3D follow-the-leader spine kinematics. Players fire **Steam Harpoon Winches** that attach Hooke's Law spring-damper steel cables between the ship's winch block and a specific segment on the Leviathan, allowing players to reel in the beast or be towed in its wake.

---

## 2. Module Architecture (`src/`)

### `src/core/` — Math, Spatial Indexing & Configuration
- `Math3D.h/.cpp`: Value-type 3D linear algebra (`Vec3`, `Int3`, `Quat`, `Ray3D`, `AABB3D`), ray-box and ray-sphere intersection routines, and a deterministic `Rng32` (PCG32) generator.
- `SpatialHash3D.h/.cpp`: $O(1)$ 3D spatial hash table for broadphase sphere queries.
- `AppConfig.h/.cpp`: CLI parser supporting headless autotesting, automated screenshot capture, seed locking, and direct scene/blueprint booting.

### `src/sim/` — Headless-Capable 120Hz 3D Simulation (`aethernaut_sim`)
The entire simulation library (`aethernaut_sim`) has **zero dependency on OpenGL or windowing headers**, enabling fast unit tests (`aethernaut_tests`) and headless simulation benchmarks (`aethernaut_benchmark`).
- `BlockRegistry.h/.cpp`: Immutable specification table for all 12 functional block types.
- `Airship.h/.cpp`:
  - Flat 3D contiguous voxel storage (`kShipGridX * kShipGridY * kShipGridZ`) for cache locality.
  - `recomputeMetrics()`: Computes total mass, Center of Mass, Center of Lift, buoyancy ratio, thrust, turning torque, and reactor MW power efficiency.
  - `severDisconnectedBlocks()`: 6-neighbor 3D BFS flood-fill from Command Bridge / Reactor nodes; converts unreachable blocks into tumbling `DetachedDebris`.
  - `raycastBlocks()`: Transforms world rays into ship-local space via inverse quaternion and tests occupied voxel AABBs, returning both the hit block and adjacent placement face for the 3D Drydock Shipyard.
- `Leviathan.h/.cpp`: Multi-segment 3D kinematic spine solver with per-segment destructible carapace armor and exposed Aether Core weak spots ($2.4\times$ critical multiplier).
- `Ballistics.h/.cpp`: Gravity-affected 3D projectiles, two-iteration predictive 3D ballistic lead solver (`computeBallisticLeadPoint`), and Hooke's Law spring-tension harpoon tether state.
- `Archipelago.h/.cpp`: Procedural 3D floating islands across 5 biomes, Drydock Sky-Ports, harvestable Aetherium Crystal veins, wind vector fields, and moving storm cells.
- `AIController.h/.cpp`: Autonomous naval captain steering airships into tangential broadside orbits while avoiding 3D island cliffs.

### `src/render/` — OpenGL 3.3 & Custom GLSL 330 Pipeline
- `ShaderPipeline.h/.cpp`: Compiles `assets/shaders/lit_world.{vs,fs}.glsl` (directional sun + hemisphere sky/cloud ambient + Blinn-Phong specular + Fresnel rim lighting + exponential distance & cloud-height fog) and `assets/shaders/post_hdr.fs.glsl` (9-tap HDR bloom + ACES filmic tone mapping + storm lightning flash + vignette). Includes embedded C++ shader strings as a zero-file fallback.
- `WorldRenderer.h/.cpp`: Hierarchical 3D rendering of articulated ship blocks (spinning 4-blade propellers, aim-tracking cannon barrels, glowing reactor cores), multi-segment Leviathans with animated jaws and pectoral fins, catenary harpoon cables, and volumetric cloud layers.

### `src/audio/` — Real-Time Procedural Wave Synthesis
- `SynthAudio.h/.cpp`: Generates 16-bit 44.1 kHz PCM waveforms at startup for cannon broadsides, steam harpoon ratchets, steel cable snaps, timber fractures, FM-modulated Leviathan calls, and an ambient sky-voyage chord progression.

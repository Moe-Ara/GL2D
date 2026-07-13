<div align="center">
  <h1>GL2D</h1>
  <p>Modern C++ 2D engine with layered rendering, gameplay systems, and editor-ready foundations.</p>
  <p>
    <img src="docs/TheLostHeroin.gif" alt="Engine demo showcase" width="900"/>
  </p>
</div>

## Overview
GL2D is a C++ 2D engine focused on high-performance rendering, modular gameplay systems,
and data-driven content workflows. It provides explicit render layering, camera control,
physics, animation, lighting, audio, and tooling hooks to build 2D games efficiently.

## Engine Highlights
- Explicit render layers + stable sorting for deterministic draw order
- HDR lighting, bloom, filmic tone mapping, color grading, and vignette
- Parallax-ready camera with follow modes and world bounds
- Sprite + tilemap rendering with batching
- Animation system with state machines and frame events

## What This Demonstrates 
- Engine architecture design: systems + component model, clear data flow
- Graphics programming: batching, render targets, lighting passes, shaders
- Gameplay systems: camera, physics, input, animation state machines
- Tooling mindset: managers/loaders, data-driven setup, debug overlays
- Performance awareness: sorting, batching, broadphase collision

## Systems
- **Rendering**: sprite batching, render layers, HDR post-processing, normal maps, light cookies
- **Camera**: delayed/smoothed follow, dead zones, predictive look-ahead, bounded shake, cinematic zoom, and world bounds
- **Physics**: rigid bodies, oriented box/circle/capsule narrowphase, BVH broadphase, casts, triggers, hinges, and ropes
- **Character control**: fixed-step simulation, buffered/coyote jumps, variable jump height, analog air control
- **Traversal**: ECS climbable volumes for ladders/vines and momentum-preserving rope traversal
- **Input**: action bindings, event bus, command registry, keyboard/gamepad mapping
- **Animation**: animator, animation state machine, per-frame callbacks
- **Audio**: music/SFX/dialogue, buses, spatial listener, crossfades (miniaudio)
- **Particles**: sprite-based particle rendering pipeline
- **AI utilities**: typed behavior trees, deterministic raster navmesh paths, perception queries, and combat timing
- **Content**: texture/sprite managers, tilemap data, level loader
- **Content safety**: weak resource registries, validated prefab references, schema-checked levels, and authored trigger callbacks
- **UI**: hierarchical anchored layout, validated JSON screens, safe texture ownership, and core-profile batched rendering
- **Debug**: overlay, collider and sensor debug draws

## Example Engine Workflow
1) Load sprites, tilemaps, and animations through managers/loaders.
2) Build a `Scene` with entities + components.
3) Run `RenderSystem` to gather and sort draw calls by layer/z.
4) Apply lighting via render targets + lighting pass.
5) Update physics, controllers, and animations each frame.

## In Progress / Planned (Editor)
- Scene/prefab authoring
- Gizmos for translate/rotate/scale
- Inspector for component editing
- Asset browser + animation preview
- Scene serialization + live reload
- Tilemap and navmesh tooling
- Lighting authoring tools

## Architecture
- **Core**: `Scene` hosts the generational ECS registry alongside legacy entities during migration
- **Render Pipeline**: `RenderSystem` extracts both ECS and legacy sprites into one stable render queue
- **Lighting**: floating-point scene and lighting targets feed bloom and tone-mapped presentation
- **Gameplay**: controllers and components update entities; physics resolves collisions
- **Data**: managers and loaders drive content setup (sprites, tilemaps, levels)

## Representative Code Areas
- Render pipeline: `src/RenderingSystem/Renderer.*`, `src/RenderingSystem/RenderSystem.*`
- Lighting: `src/RenderingSystem/LightingPass.*`, `src/RenderingSystem/RenderTarget.*`
- Camera: `src/Graphics/Camera/Camera.*`
- Physics: `src/Physics/*`
- Animation: `src/Graphics/Animation/*`, `src/GameObjects/Components/AnimatorComponent.*`
- Audio: `src/AudioSystem/AudioManager.*`
- Input: `src/InputSystem/*`

## Build

GL2D requires CMake 3.23+, a C++23 compiler, GLFW, OpenGL, GLEW, and GLM.
The core engine and demo do not require Qt or Boost.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Optional targets are controlled independently:

- `GL2D_BUILD_DEMO` (default `ON`) builds The Lost Heroin demo.
- `GL2D_BUILD_EDITOR` (default `OFF`) builds the Qt 5 editor.
- `GL2D_BUILD_TESTS` (default `OFF`) builds the Boost.Test suite and enables CTest.
- `GL2D_BUILD_BENCHMARKS` (default `OFF`) builds focused performance benchmarks.

For a minimal engine-only build, configure with `-DGL2D_BUILD_DEMO=OFF`.
To run the test suite, configure with `-DGL2D_BUILD_TESTS=ON`, build, then run
`ctest --test-dir build --output-on-failure`.

## Run (Demo)
The demo executable is generated under `Demos/The Lost Heroin` in the selected
build directory (with the platform-specific executable suffix).

`ECSPlatformer` is also built when demos are enabled. It is the reference executable
for the fixed-step ECS character motor, swept collision, moving-surface contacts, and
ECS render extraction.

## Project Layout
- `src/` engine source
- `Demos/The Lost Heroin/` demo scene
- `assets/` shared content
- `docs/` documentation and showcase media

## Runtime ownership

Scenes own their entities and entities own their components. References and raw
pointers returned by these APIs are non-owning and must not outlive their owner.
Entity creation and destruction requested during a scene update are applied at a
safe phase boundary, so callbacks may mutate the scene without invalidating the
active update traversal.

The ECS migration design is documented in [ECSArchitecture.md](docs/ECSArchitecture.md);
fixed-step and character-control invariants in [CharacterMotor.md](docs/CharacterMotor.md);
animation graphs and events in [ECSAnimation.md](docs/ECSAnimation.md); and HDR
presentation in [PostProcessing.md](docs/PostProcessing.md). ECS-native light
authoring is covered by [ECSLighting.md](docs/ECSLighting.md), and deterministic
HDR particle effects by [ECSParticles.md](docs/ECSParticles.md). Collision,
broadphase, stepping, triggers, and cast contracts are in
[Physics.md](docs/Physics.md); ladder and rope authoring is covered by
[ClimbingAndRopes.md](docs/ClimbingAndRopes.md). Resource ownership, level
validation, component factories, and authored triggers are documented in
[ContentRuntime.md](docs/ContentRuntime.md). Behavior-tree contracts, pathfinding
limits, perception, and combat timing are in
[AI.md](docs/AI.md). Core-profile diagnostic drawing and renderer frame contracts
are documented in [DebugRendering.md](docs/DebugRendering.md). UI tree ownership,
parent-relative layout, rendering, and font behavior are in [UI.md](docs/UI.md).
Follow composition, look-ahead, shake, and zoom tuning are covered by
[CameraGameFeel.md](docs/CameraGameFeel.md).

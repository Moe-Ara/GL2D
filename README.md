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
- Off-screen render target with lighting/composite pass
- Parallax-ready camera with follow modes and world bounds
- Sprite + tilemap rendering with batching
- Animation system with state machines and frame events

## What This Demonstrates (Recruiter View)
- Engine architecture design: systems + component model, clear data flow
- Graphics programming: batching, render targets, lighting passes, shaders
- Gameplay systems: camera, physics, input, animation state machines
- Tooling mindset: managers/loaders, data-driven setup, debug overlays
- Performance awareness: sorting, batching, broadphase collision

## Systems
- **Rendering**: sprite batching, render layers, post-process lighting pass, normal maps, light cookies
- **Camera**: follow modes, dead zones, world bounds, view projection helpers
- **Physics**: rigid bodies, colliders (AABB/circle/capsule), triggers, broadphase quadtree
- **Input**: action bindings, event bus, command registry, keyboard/gamepad mapping
- **Animation**: animator, animation state machine, per-frame callbacks
- **Audio**: music/SFX/dialogue, buses, spatial listener, crossfades (miniaudio)
- **Particles**: sprite-based particle rendering pipeline
- **AI Utilities**: navmesh debug, perception helpers, steering utilities
- **Content**: texture/sprite managers, tilemap data, level loader
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
- **Core**: `Scene`, `Entity`, and `Component` model with update/render loops
- **Render Pipeline**: `RenderSystem` submits sprites to `Renderer`, sorted by layer and z
- **Lighting**: render target + lighting pass, then composite to screen
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

## Build (Windows)
```bash
cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug --target TheLostHeroin -j 10
```

## Run (Demo)
```bash
cmake-build-debug\Demos\The Lost Heroin\TheLostHeroin.exe
```

## Project Layout
- `src/` engine source
- `Demos/The Lost Heroin/` demo scene
- `assets/` shared content
- `docs/` documentation and showcase media

## Notes
If you add new demo source files, reconfigure CMake so it picks up new sources.

<div align="center">
  <h1>GL2D</h1>
  <p>Modern C++ 2D engine focused on rendering, tools, and gameplay systems.</p>
  <p>
    <img src="docs/TheLostHeroin.gif" alt="Engine demo showcase" width="900"/>
  </p>
</div>

## Overview
GL2D is a C++ 2D engine with a focus on layered rendering, camera systems, physics,
and tooling to build 2D games efficiently.

## Features
- Sprite rendering with explicit render layers
- Camera system with follow modes and world bounds
- Parallax background layers
- Basic physics (rigid bodies, colliders)
- Input bindings and action events
- Animation system (spritesheets, state machines)
- Lighting pipeline (lights, cookies, normal maps)
- Tilemap rendering
- Particles
- Audio (runtime hooks)
- Debug overlay and gizmos
- Post-process lighting pass (off-screen render target)
- Texture caching and sprite cloning
- Action-driven input mapping (keyboard + gamepad)
- Scene utilities for content-driven setup

## In Progress / Planned (Editor)
- Level editor (scene/prefab authoring)
- Gizmos for translate/rotate/scale
- Inspector for components and properties
- Asset browser and sprite/animation preview
- Scene serialization + live reload
- Tilemap and navmesh tooling
- Lighting authoring tools

## Showcase
The demo scene in `Demos/The Lost Heroin` highlights the engine systems in action.

## Build (Windows)
```bash
cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug --target TheLostHeroin -j 10
```

## Run (Demo)
```bash
cmake-build-debug\Demos\The Lost Heroin\TheLostHeroin.exe
```

## Controls (Demo)
- `F3` toggle debug overlay
- `Esc` close window

## Project Layout
- `src/` engine source
- `Demos/The Lost Heroin/` demo scene
- `assets/` shared content
- `docs/` documentation and showcase media

## Engine Architecture
- **Core**: `Scene`, `Entity`, and `Component` model with update/render loops.
- **Rendering**: `Renderer` + `RenderSystem` collect sprites, sort by layer/z, and draw via shaders.
- **Camera**: follow modes, world bounds, and view/projection for culling and parallax.
- **Physics**: rigid bodies, colliders, and basic collision/trigger handling.
- **Input**: action binding system with per-frame event polling.
- **Content**: sprite/texture managers, animation state machines, tilemaps.
- **Debug**: overlay, collider/gizmo debug draws, and diagnostics.

## Notes
If you add new demo source files, reconfigure CMake so it picks up new sources.

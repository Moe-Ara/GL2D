<div align="center">
  <h1>GL2D</h1>
  <p>2D game engine + demo scene (The Lost Heroin)</p>
  <p>
    <img src="docs/TheLostHeroin.gif" alt="The Lost Heroin demo" width="900"/>
  </p>
</div>

## Overview
GL2D is a C++ 2D engine with a demo scene showcasing parallax backgrounds, camera follow,
lighting, and sprite-based rendering.

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

## Demo
The main showcase is the demo in `Demos/The Lost Heroin`.

## Build (Windows)
```bash
cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug --target TheLostHeroin -j 10
```

## Run
```bash
cmake-build-debug\Demos\The Lost Heroin\TheLostHeroin.exe
```

## Controls
- `F3` toggle debug overlay
- `Esc` close window

## Project Layout
- `src/` engine source
- `Demos/The Lost Heroin/` demo project
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

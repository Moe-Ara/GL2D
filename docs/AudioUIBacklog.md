# Audio & UI Backlog (Production-Ready)

The remaining core systems to round out the engine are audio playback and a lightweight UI layer.

---

## 1. Sound System

**Goal:** Authoritative audio service for SFX, music, and spatialized playback with data-driven control.

### Steps
1) Add an `AudioEngine` wrapper (OpenAL/SDL_mixer/FMOD placeholder) with init/shutdown, listener state, and mixing thread safety.
2) Introduce `SoundBank` / `SoundHandle` APIs: load WAV/OGG, cache handles by id, and expose volume/pitch/loop flags.
3) Implement `AudioSource` component for entities (2D spatial panning, distance attenuation) and a non-spatial `MusicChannel` with crossfade.
4) Provide an event API: fire-and-forget `playOneShot(id, pos, volume)` plus tracked handles for stop/pause/seek.
5) Data-driven config: `assets/config/audio_banks.json` listing sounds, volumes, categories (SFX/Music/UI), and optional random variants.
6) Add debug counters (active sources, voices) and a mute toggle per category; expose to console/overlay.

### Risks
* Thread-safety around engine callbacks (mix thread vs game thread); guard with queues.
* Resource leaks on hot-reload; ensure handles ref-count and unload correctly.
* Latency/underrun on some platforms; configurable buffer sizes needed.

---

## 2. UI System

**Goal:** Lightweight 2D UI layer for HUD/menus with input focus, layout, and styling.

### Steps
1) Build a UI renderer: batched quads/textured sprites reusing the existing quad pipeline; add a simple text renderer (bitmap/atlas font).
2) Define a widget tree (`Panel`, `Label`, `Button`, `Slider`), with transform/anchor/layout (vertical/horizontal stacks, padding, alignment).
3) Input routing: focus/hover/press state driven by mouse/keyboard actions from `InputService`; support navigation (tab/arrow) and click/drag.
4) Skinning/theming: JSON/TOML style file for colors, fonts, margins, and hover/pressed states; allow per-widget overrides.
5) Events/callbacks: signal system for `onClick`, `onValueChanged`; integrate with gameplay commands or a console bridge.
6) Debug mode: draw UI bounds, focus state, and hierarchy overlay; toggle via hotkey/console command.

### Risks
* Z-order and render order clashing with in-world sprites; keep UI in a dedicated render pass and coordinate space.
* Text rendering performance; cache glyphs and measure draw counts.
* Input edge cases (focus loss, drag-release outside widget); add clear state reset paths.

---

# 13 — GL2D Feature Requirements

Per-chapter engine needs, classified: **HAVE** (shipping in GL2D today),
**PARTIAL** (exists, needs extension), **BUILD** (new engine work). The build
column drives the engine roadmap (see ../design/TODO.md and
docs/EngineStatus.md).

## Capability matrix

| Capability | Status | Used first | Notes |
|---|---|---|---|
| Parallax layers (factor 0–1.2, tiled) | **HAVE** | P | `ParallaxLayer2D` + `ParallaxSystem2D` |
| Feelings-driven grade/camera/gameplay | **HAVE** | P | five authored states (implemented in the demo now) |
| HDR + bloom + vignette + color grade | **HAVE** | P | post pipeline |
| Sprite lighting + normal maps + cookies | **HAVE** | 1 | lamp/beam via cookies |
| Particle systems (deterministic, ECS) | **HAVE** | P | ambient identities per chapter |
| Trigger volumes (enter/exit/once) | **HAVE** | P | camera/feeling/SE triggers |
| Character controller (coyote, buffer, ledge-hang) | **HAVE** | P | tuned per feeling |
| Climbing volumes (ladders/kelp) | **HAVE (ECS)** | P2 | legacy-player bridge needed → BUILD item 6 |
| Rope physics (swings, hinges) | **HAVE** | 3 | kelp-ropes |
| Water buoyancy volumes | **HAVE** | 1.2 | Ghost Tide floats = `WaterSystem`; visual layer is BUILD |
| Fixed-step determinism + interpolation alpha | **HAVE/PARTIAL** | all | render interpolation not yet wired (EngineStatus #1) |
| Audio buses, spatial SFX, crossfades | **HAVE** | P | miniaudio |
| Behavior trees / perception | **HAVE** | 4 | Undertow patrol brain (simple) |
| Scene clear color | **HAVE** | P | authored per chapter |
| **Camera rails / spline dollies** | **BUILD** | P3 | authored lateral tracks, locked-Y, lead-ahead rails |
| **Camera framing volumes** (zoom/state per region, blend) | **PARTIAL** | P1 | today: triggers + feelings zoomMul; need priority/blend stack |
| **Letterboxing** | **BUILD** | 6 | animated aspect bars via post pass |
| **Cutscene/timeline system** (tracks: camera, anim, audio, SE) | **BUILD** | P1 | door reveal, Warden, tilt, ignition |
| **Ghost Tide rendering** (translucent volume, surface glitter, caustics) | **BUILD** | 1.2 | shader + mesh over WaterSystem volumes |
| **Fog planes per parallax band** | **BUILD** | 1 | quad + shader, palette-driven |
| **Heat-shimmer / distortion band** | **BUILD** | 2 | screen-space UV warp region |
| **Mirror-reflection regions** (offset re-render or SDF trick) | **BUILD** | 2.2 | reflection shows *different* content (storm sky): actually a second parallax set drawn flipped in pool masks — cheaper than true reflection |
| **Wind force volumes** on bodies + cloth-ish sprites | **PARTIAL** | 2 | force generators exist; author volumes + sail sprite shader |
| **Light propagation chains** (scripted sequence lighting) | **BUILD (game-side)** | 3 | gameplay script over ECS lights |
| **Darkness/light-radius gameplay** (visibility mask) | **BUILD** | 4 | radial mask pass over lighting target |
| **Sound-occlusion / mix-hole ducking** | **BUILD (audio-side)** | 4 | bus ducking by proximity — AudioManager extension |
| **Two-state world tilt** (ship list) | **BUILD (game-side)** | 5 | authored transform root lerp + physics re-settle |
| **Frozen-body + one-shot release ("Time Stubs")** | **BUILD** | 6 | freeze = kinematic hold; release = restore dynamics, replay authored impulse, re-freeze on rest |
| **Audio-layer thaw zones** | **PARTIAL** | 6 | trigger + bus fades (mostly authoring) |
| **Swim/buoyant player state** | **PARTIAL** | 1.2 | WaterSystem + controller swim mode |
| **Save/checkpoint system** | **BUILD** | all | scene-state snapshot at CPs |
| **Localization-free text pipeline** | trivial | credits | text only in menus/credits |

## Per-chapter pulls (what each chapter forces the engine to grow)

- **Prologue:** cutscene timeline (door reveal), framing volumes, fog plane.
- **Ch1:** Ghost Tide render + swim state (the big one, early on purpose),
  bell→tide scripting, chord-on-touch audio hooks.
- **Ch2:** distortion band, mirror-region trick, wind volumes + sail forces,
  widest-zoom perf validation (parallax + particles at 0.65×).
- **Ch3:** camera rails, light-chain scripting, Warden timeline (multi-layer
  rigged sprite on a track), climbing bridge to player.
- **Ch4:** darkness mask + light-radius gameplay, mix-hole audio ducking,
  Undertow BT + flow-field particle body, checkpoint density stress test.
- **Ch5:** world-tilt state machine, mass slide choreography (physics under
  timeline control), interior reverb zones.
- **Ch6:** Time Stubs (freeze/release), letterboxing, storm-second audio
  gating, ghost-light accumulation, the lamp beam (cookie sweep).
- **Finale:** underwater grade + buoyant traversal, rising particle fields,
  credits timeline.

## Engine-first build order (so the game pulls the engine, not vice versa)

1. Cutscene timeline + camera rails/framing stack (used by every chapter).
2. Ghost Tide visual + swim (unlocks Ch1 vertical slice).
3. Save/checkpoints (needed before any playtest longer than one sitting).
4. Darkness mask + audio ducking (unlocks Ch4).
5. Time Stubs (unlocks Ch6).
6. Everything else per chapter pull list.

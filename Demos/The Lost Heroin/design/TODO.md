# LOWTIDE — Implementation TODO

Status ledger between the design package (docs 00–13) and the running demo.
Ordered so every milestone ends in something playable. Engine-level items
cross-reference `docs/EngineStatus.md`.

## Implemented today (running in the demo)

- ✅ **Five authored emotional states** (`assets/config/feelings.json`:
  still / wonder / fear / grief / resolve) with the grade, camera, lighting,
  and movement values from doc 00 — loaded through `FeelingsLoader`,
  validated, owned by a `FeelingsController` in `Game`.
- ✅ **Chapter mood wiring**: crossing a chapter gate blends the whole scene
  (post grade, vignette, bloom, zoom, camera damping, light multipliers,
  player speed/acceleration) into that chapter's state. Boot state is
  `still`. Demo mapping: BG_01→still, BG_02→wonder, BG_03→grief, BG_04→fear.
- ✅ Asset-override fix: demo-authored configs now override engine samples of
  the same name in the build assets (CMake copy order).
- ✅ Already in place from engine work: parallax layer component/system,
  trigger-driven chapter changes, friction/restitution materials, scene
  clear color, tiled ground world, tuned character movement.

## Milestone 0 — Prologue vertical slice (greybox)

*Goal: P1–P3 playable grey, ending on the door-reveal. Proves the game.*

- [ ] Greybox scene format: author P/1 layouts as data (extend the existing
  level loader) instead of C++ in `SceneBuilder` — platforms, ladders,
  triggers, feeling zones from JSON. **(game+engine: level schema v2)**
- [ ] **Cutscene timeline system** (engine): tracks for camera, feeling,
  audio, entity animation, scripted motion; blocks or frees player input per
  clip. Needed by beat P1 and every chapter after. *(EngineStatus roadmap —
  highest game-blocking item)*
- [ ] **Camera framing volumes + rail segments** (engine): zoom/state per
  region with priority blending; locked-Y rails for the mole/boardwalk
  shots. Today's trigger+feeling zoom covers maybe half of doc 05.
- [ ] Interior room + porch greybox, ladder shaft, flats walk, footprint
  decals, door-reveal timeline.
- [ ] **Save/checkpoint system** (engine): snapshot player position, chapter,
  feeling state at CP markers; silent autosave.

## Milestone 1 — Chapter 1 slice (the hook: Ghost Tide)

- [ ] **Ghost Tide rendering** (engine): translucent volume + surface
  glitter over `WaterSystem` buoyancy volumes; rise/fall animation driven by
  gameplay. The game's signature image — build early.
- [ ] **Swim/buoyant player state** (game): controller mode inside tide
  volumes (WaterSystem already supplies forces; controller needs a swim
  stance + animation hooks).
- [ ] Tide-bell interactable: Call-proximity ring → tide rise to authored
  line → decay timer audible. Puzzles #1/#2 greybox.
- [ ] **The Call verb** (game): held input binding, hum audio layers,
  light-echo particle ring, proximity queries for responsive objects
  (bells/polyps/releasables all subscribe to one `CallEvent`).
- [ ] Reach verb consolidation: one contextual action (push/hold/touch)
  replacing ad-hoc interactions.

## Milestone 2 — Chapters 2–3 systems

- [ ] Wind force volumes + sail bodies (engine PARTIAL: force generators
  exist; author volumes, luff/fill sprite states).
- [ ] Mirror-pool trick (engine): flipped second parallax set drawn inside
  pool masks (stencil or scissor + flip transform) — *not* true reflection.
- [ ] Heat-shimmer distortion band (engine: screen-space UV warp region in
  post pass).
- [ ] Light-chain scripting (game): polyp graph, chain propagation, decay,
  gates, lure-fish follow agent (existing BT + steering).
- [ ] The Warden: multi-layer rigged silhouette on a timeline track.
- [ ] Climbing bridge (engine): legacy player ↔ ECS `ClimbingSystem2D`
  unification — or move the player fully onto the ECS motor (preferred;
  EngineStatus migration step 2).

## Milestone 3 — Chapter 4 systems (the dark)

- [ ] Darkness/visibility mask (engine): radial light-radius pass over the
  lighting target; jelly radius ↔ calm mapping (feelings-driven).
- [ ] Audio mix-hole ducking + proximity buses (engine: AudioManager zones).
- [ ] Undertow agent: flow-field particle body + simple BT (patrol,
  investigate sound, recoil from light); light-steal and smash-cut
  checkpoint behaviors.
- [ ] Checkpoint density tooling: CP markers every ≤ 25 s validated by a
  traversal script.

## Milestone 4 — Chapters 5–6 systems

- [ ] Two-state world tilt (game): authored root-transform lerp with physics
  re-settle; furniture mass choreography under timeline control.
- [ ] Interior reverb zones (engine audio).
- [ ] **Time Stubs** (engine+game): freeze = kinematic hold of an authored
  body set; release = restore dynamics + authored impulse + 1-s gated audio
  clip; re-freeze on rest. One-shot, per-object, order-sensitive.
- [ ] Letterboxing (engine: animated aspect bars in post).
- [ ] Ghost-light accumulation, lamp beam (light cookie sweep), crank-hold
  stations, the kneel interaction.

## Milestone 5 — Finale + presentation

- [ ] Underwater grade + buoyant ascent traversal; rising light fields.
- [ ] Credits timeline; post-credits held shot.
- [ ] Render interpolation (EngineStatus #1) — required before any public
  build; the slow cinematic walks will show stepping without it.
- [ ] Menu/title/pause minimalism pass (UI system exists).

## Content production (parallel track, per doc 11)

- [ ] Mara sprite set (~20 clips) — the single largest asset item; commission
  early, animate against greybox timing.
- [ ] Chapter art: timber kit → salt bands → coral kit → trench kit → ship
  kit → lighthouse; 8 bespoke poster scenes last.
- [ ] Audio: 7 chapter beds, bell/polyp/clockwork sets, 5 score cues, the
  one recorded name.

## Explicitly deferred / cut candidates

- True reflective water (mirror trick suffices), multi-point contact
  manifolds (no stacking gameplay), any dialogue system (none in design),
  localization beyond credits text, photo mode (post-ship polish).

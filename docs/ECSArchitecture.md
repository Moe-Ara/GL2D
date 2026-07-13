# GL2D ECS Architecture

GL2D is migrating from polymorphic entity-owned components to data-oriented ECS
systems. The migration is incremental: `Scene` owns an `ECS::Registry` alongside
legacy entities until each complete runtime slice has moved. New gameplay runtime
code should target the registry unless it must integrate with an unmigrated system.

## Invariants

- `ECS::Entity` is a non-owning generational handle. Always validate stored handles
  with `Registry::alive`; never retain component references across structural changes.
- Components are movable values containing state, not update methods or scene ownership.
- Systems own behavior and query the exact component set they require.
- Component addition and removal are forbidden inside a query. Entity destruction is
  safe and deferred until the outermost query completes.
- Cross-entity relationships store `ECS::Entity`, not raw pointers.
- Resources such as textures, animation clips, and audio assets remain manager-owned;
  ECS components store lightweight handles or shared immutable resources.

## Intended system flow

Each frame uses explicit phases so input and simulation stay responsive and rendering
interpolates stable state:

1. Sample input into intent components.
2. Run a fixed-step gameplay and physics simulation.
3. Resolve contacts, triggers, and state transitions.
4. Advance animation state and emit frame events.
5. Build an interpolated render snapshot.
6. Submit lighting, particles, post-processing, UI, and debug overlays.

Structural changes requested during systems should eventually flow through a command
buffer. Until that migration step lands, systems may safely destroy entities during a
query but must add/remove components between system phases.

## Migration order

1. Transform, sprite, and lighting data plus render extraction.
2. Input intent, character motor, sensors, and deterministic fixed-step control.
3. Rigid bodies, colliders, triggers, water, ropes, and platform relationships.
4. Animator state, transitions, frame events, and visual effects. The core ECS
   animation graph and per-instance presentation path are now active; remaining
   legacy metadata users migrate through `AnimationGraphLoader2D`.
5. AI perception/navigation/combat and remaining gameplay components.
6. Prefabs, level loading, serialization, editor inspection, and legacy removal.

Each step must ship as a complete vertical slice with tests, sanitizer coverage,
runtime smoke tests, and before/after performance measurements. Legacy APIs are only
removed once all in-repository callers have migrated and the replacement is documented.

## Parallax layers

`ParallaxLayer2D` + `ParallaxSystem2D` position background/foreground layers
relative to the camera. It is a **presentation** pass: `RenderSystem::renderScene`
runs it once per rendered frame (not per fixed step) just before extraction, so
layer transforms are current for both culling and submission.

`factor` is the fraction of camera travel a layer shows on screen:

- `{0, 0}` pins the layer to the camera (an infinitely distant sky).
- `{1, 1}` locks the layer to the world (moves exactly with gameplay geometry).
- `> 1` makes a foreground parallax past the world.

The layer's anchor therefore moves with the camera by `(1 - factor)`. For a tiled
strip, author one entity per tile sharing `basePosition`, `baseCameraCenter`,
`tileWidth`, and `tileCount`, with `tileIndex` running `0..tileCount-1`; the
system wraps the strip around the current view so a tile leaving one edge
reappears on the other, keeping the screen covered no matter how far the camera
travels. Pair each tile with a `Transform2D` and a `SpriteRender` for drawing.

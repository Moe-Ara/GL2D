# AI decision and navigation utilities

GL2D's AI layer supplies decision, navigation, perception, and combat primitives.
It deliberately does not own entities or run a second world scheduler. Games keep
agent state in ECS components (or in an explicitly owned gameplay context) and
invoke these utilities from their normal fixed-step systems.

## Behaviour trees

`AI::BehaviourTree<TContext>` owns its complete node graph. Build composite nodes
with `makeSelector` or `makeSequence`, then attach children with `addChild`.
Actions return `AI::NodeStatus`; conditions return `bool`. Empty callbacks, null
decorator children, invalid repeat counts, and invalid time deltas are rejected.

Selectors and sequences remember a running child and resume it on the next tick.
The built-in decorators have explicit semantics:

- `Inverter` swaps success and failure while preserving running.
- `Succeeder` maps either terminal result to success.
- `Failer` maps either terminal result to failure.
- `Cooldown` returns failure while blocked and starts its timer only after child
  success. If the timer expires during a tick, the child may run in that tick.
- `Repeater` evaluates at most one child cycle per tick. A finite repeater returns
  success after the requested number of terminal child results; `-1` repeats
  indefinitely.

`reset()` clears composite, cooldown, and repeater runtime state recursively. Node
runtime fields are private so callers cannot corrupt a running tree.

## Navigation

`NavRaster` is an authored or generated grid of walkable cells. Dimensions and
cell size must be positive, and all geometry must be finite. Checked cell access
throws for programmer errors; `worldToCell` reports ordinary out-of-bounds queries
with `false` and sets both output coordinates to `-1`.

`PolyNavMesh::buildFromRaster` merges walkable cells into axis-aligned rectangular
polygons, builds shared portals, and uses deterministic A* plus a funnel pass for
path queries. `findPath` returns an empty path when either endpoint is outside the
walkable mesh or the regions are disconnected. `polygons()` exposes immutable
geometry for editor/debug rendering without coupling navigation to OpenGL.

The current raster mesh models a point agent on a 2D walkable surface. It does not
inflate obstacles for an agent radius and does not infer platformer actions such
as jumping, dropping through platforms, climbing, or flying. Those transitions
must be authored as gameplay graph edges before using it for a platformer enemy;
do not treat an arbitrary geometric route as executable character input.

## Perception

`AI::hasLineOfSight` ray-casts against collision layers. Pass the querying entity
as `ignore` and the observed entity as `target`; hitting the target's own collider
first counts as visible. Without `target`, the query treats colliders before the
endpoint as occluders.

`AI::canHear` performs a radius overlap, excludes triggers, applies a layer mask,
and optionally returns the heard entities. The output vector is always cleared,
including zero-radius queries and validation failures. These functions currently
query the legacy entity/collider collection; an ECS-native spatial query should
replace that adapter when the general physics migration reaches perception.

## Combat timing

`AICombatBrain` is a deterministic range and cooldown gate. Call `update(dt)` once
per fixed simulation step even when no target exists, then call `tryAttack` when an
attack is requested. Attack attempts consume cooldown only when the target is in
range. `CombatComponent` follows the same rule and remains inactive until an
explicit target position is assigned; `clearTarget()` prevents accidental attacks
at a default world position.

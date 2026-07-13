# Character Motor and Simulation Timing

GL2D simulates gameplay at 120 Hz by default through `Scene::advance`. Rendering
still runs once per display frame. The fixed-step clock clamps unusually long frames,
limits catch-up work, and reports discarded time so tooling can identify stalls.
Games should call `Scene::advance` or `Scene::updateWorld`; `Scene::update` intentionally
runs one exact simulation step for tests and controlled tools.

## ECS motor data

The ECS character pipeline separates intent, tuning, runtime state, body velocity,
and collision contact:

- `CharacterIntent` is written once from sampled input and consumes button edges once.
- `CharacterMotorConfig` contains designer-facing movement values.
- `CharacterMotorState` exposes facing, locomotion, landing, and jump events.
- `KinematicBody2D` carries velocity into the physics/collision phase.
- `GroundContact2D` is written by collision detection before the next motor step.

`CharacterMotorSystem` implements acceleration, stronger direction reversal, air
control, coyote time, jump buffering, variable jump height, faster falling, and a
terminal fall speed. It does not move transforms directly: collision-aware physics
owns position integration and writes authoritative contacts.

`KinematicCharacterPhysicsSystem` is the authoritative position/contact phase for
ECS characters. It provides swept AABB collision against static and moving surfaces,
collision masks, floor/wall/ceiling contacts, ground probing, spawn depenetration,
and moving-platform velocity inheritance. Sweeping prevents fast characters from
tunneling through thin floors or walls at the fixed simulation rate.

## Input rules

Input is sampled once per rendered frame and assigned an `actionFrame` sequence.
Controllers process each sequence once even when several fixed simulation steps run.
Keyboard repeat messages are ignored because button edges must represent physical
press/release transitions, not operating-system repeat settings.

## Legacy migration

The legacy `CharacterController` now uses the same responsiveness principles while
the player physics slice is migrated: coyote time, buffered jumps, jump cutting, and
separate acceleration/deceleration are active. Dynamic characters receive gravity
only from `PhysicsEngine`; per-body gravity scale preserves character tuning without
the former double-gravity bug. Character rigid-body damping is zero because the motor
already owns horizontal deceleration and damping would weaken jumps.

## Executable reference

The `ECSPlatformer` demo is a fully ECS-authoritative vertical slice: input writes
`CharacterIntent`, the motor writes velocity, kinematic collision writes transform
and contacts, and render extraction consumes ECS transform/sprite components. It is
kept intentionally small so control and collision behavior can be evaluated without
legacy gameplay components masking problems.

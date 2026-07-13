# Physics contracts

GL2D has two physics paths while the ECS migration is in progress:

- Legacy entities use `RigidBody`, `ColliderComponent`, `PhysicsEngine`, and
  `TriggerSystem`.
- ECS characters use `KinematicBody2D` and
  `KinematicCharacterPhysicsSystem`; this path is tuned for responsive platform
  movement rather than general rigid-body simulation.

Do not attach both paths to the same character. One system must own its transform
and velocity.

## Fixed-step use

Call `Scene::advance` or `Scene::updateWorld` from a game loop. The scene simulates
at 120 Hz by default and may run multiple fixed steps for one rendered frame.
`Scene::update` performs one exact simulation step and is intended for tests and
controlled tools.

All physics values must be finite. Invalid masses, gravity, forces, damping,
inertia, time steps, shape bounds, and cast arguments fail immediately with an
exception instead of entering the solver.

## Collision shapes and normals

The legacy narrowphase supports box, circle, and capsule pairs. A box is
axis-aligned in its local space and becomes an oriented rectangle after its
transform is applied. Its `getAABB()` result is only the conservative world-space
bound used for broadphase queries. Exact box collisions use oriented-box SAT;
circle-box and capsule-box pairs are tested in box-local space.

`CollisionDispatcher::dispatch(a, b)` returns a `Hit` whose normal always points
from `b` toward `a`. Moving `a` by `normal * penetration` separates the pair.
Calling the dispatcher with the arguments reversed produces the opposite normal
and the same penetration. Tangential contact is not reported as penetration.

Collider transforms and rigid-body pointers are non-owning. `ColliderComponent`
and `RigidBodyComponent` bind them to the entity-owned `TransformComponent`.
Never retain those pointers beyond the owning entity's lifetime.

## Broadphase

Rigid-body and trigger pair generation rebuilds a flat, median-split BVH for the
current fixed step. This has no authored world boundary, accepts dynamic worlds,
and emits each candidate pair once. BVH bounds are conservative; every candidate
still passes through collision filtering and exact narrowphase.

`Quadtree` remains available as a spatial-query utility but is not the rigid-body
pair generator.

## Body types and stepping

- `STATIC` bodies have zero inverse mass and do not integrate.
- `KINEMATIC` bodies integrate their assigned linear and angular velocity but do
  not receive forces, gravity, or collision impulses.
- `DYNAMIC` bodies receive forces, gravity, exponential damping, constraints, and
  collision response.

`CollisionDetection::SUBSTEPPED` adaptively divides a step when a dynamic body
would travel more than half its smallest broadphase feature. It is bounded to 64
substeps. `CONTINUOUS` is a source-compatible alias for this mode; GL2D does not
currently claim an exact time-of-impact CCD solver. Use `DISCRETE` only for bodies
whose motion is safely smaller than nearby collision features.

Force generators execute once per fixed step. Their loads are applied consistently
over all adaptive substeps, so increasing the substep count does not multiply a
force or consume a generator repeatedly.

## Contact materials

Each `RigidBody` carries a friction coefficient (`setFriction`, default `0.4`,
any non-negative value) and a restitution/bounciness (`setRestitution`, default
`0.0`, clamped to `[0, 1]`). The impulse solver combines the two contacting
bodies per contact:

- **Restitution** uses the maximum of the two values, so one bouncy surface makes
  a contact bouncy. Below a small closing speed (`0.5 m`) restitution is
  suppressed so resting stacks settle instead of buzzing.
- **Friction** uses the geometric mean (`sqrt(fA * fB)`), so a frictionless body
  keeps frictionless contacts regardless of what it touches. The tangential
  impulse is clamped to the Coulomb cone (`|jt| <= mu * jn`) and can only remove
  energy, never add it.

Because friction and restitution scale with the *normal impulse* (closing
velocity), a body resting on a surface with near-zero closing speed receives
near-zero friction — a controller-driven character that sets its own horizontal
velocity is not dragged by ground contact. For such characters, set the body's
friction and restitution to `0` to keep the contact fully inert.

The solver still resolves single-point contacts (no multi-point manifold),
provides no sleeping, and does not claim exact time-of-impact CCD. Gameplay that
depends on precise tall stacking should be validated against these limits.

## Triggers

Collision queries are pure: asking whether two trigger colliders overlap never
consumes a one-shot trigger. `TriggerSystem` exclusively owns enter/exit state and
one-shot lifetime. Trigger broadphase uses the BVH, but enter and exit decisions use
exact narrowphase; conservative rotated bounds cannot keep a separated trigger
active.

Destroying an entity unregisters its active trigger keys at the scene mutation
boundary. Trigger callbacks may request scene mutations because the scene defers
them until the active traversal is safe.

## Casts

`PhysicsCasts` provides closest ray casts, swept axis-aligned boxes, swept capsules,
and circle overlaps. Filters can exclude an entity, include or ignore triggers,
and select collider layers. A cast hit's `distance` is travel distance; an overlap
hit's `distance` is penetration depth.

Swept shape casts deterministically bracket the first exact overlap inside a
conservative ray interval and refine it. This is robust for gameplay probes, but it
is not a general convex exact-TOI solver. Casts currently scan the supplied entity
list and are best used for focused gameplay queries rather than bulk pair finding.


# Climbing and ropes

GL2D has an ECS-native climbable path for character gameplay and a legacy
rigid-body rope prefab. They solve different problems: climbables are stable
designer-authored volumes, while ropes are simulated chains that can swing and
transfer velocity to a hanging player.

## ECS climbables

A climbable entity requires:

- `Transform2D`
- `AabbCollider2D`
- `Climbable2D`

The collider is a non-solid detection volume. Its category and mask participate in
the same two-way filtering used by character collision. `Climbable2D::axis` is a
non-zero world-space direction, so `{0, 1}` authors a ladder and a normalized
diagonal direction authors a sloped vine or climb path. `snapToAxis` asks the motor
to pull the character toward the center line at `climbSnapSpeed`.

A climbing character requires `ClimbingState2D` in addition to the components used
by the ECS character motor. Feed vertical input into `CharacterIntent::climbAxis`:
positive values move along the authored axis and negative values move against it.
The character enters only while overlapping a compatible climbable and receiving
vertical input. It does not auto-grab merely because it crossed a volume.

The scene order is intentional:

1. `ClimbingSystem2D` selects a deterministic overlapping climbable and updates
   climb state.
2. `CharacterMotorSystem` suppresses gravity, accelerates along the path, and
   applies lateral snapping.
3. `KinematicCharacterPhysicsSystem` moves the character and resolves solids.
4. Animation parameters expose the `climbing` boolean.

Jump detaches, applies the normal character jump speed, and requires the climb axis
to return to neutral before the character can grab again. This prevents a held
direction from reattaching on the following fixed step.

The `ECSPlatformer` demo contains the reference ladder, input mapping, and climb
animation graph.

## Rope prefab

`Prefabs::RopePrefab::instantiate` creates a chain from a registered segment
prefab. The segment prefab must provide a collider; the rope prefab supplies body
mass, damping, inertia, dimensions, rotation, layer filtering, links, and hinge
configuration.

Important authoring rules:

- `direction` is normalized and must be finite and non-zero.
- segment dimensions, count, mass, damping, limits, and torque values are validated
  before entity creation.
- start and end anchors must already belong to the target scene and have the
  required transform/body components.
- when no explicit start anchor is supplied, the prefab creates a static anchor.
- connected segments share `collisionLayer` and exclude that layer from their
  masks, preventing unstable self-collision while retaining world/player contacts.
- `segmentSpacing` is represented in the connected joint anchors, so the solver
  does not collapse authored gaps on its first step.
- hinge angles are radians; entity transform rotations are degrees.

Each segment receives `RopeSegmentComponent`, including previous/next links and
live world endpoints. The scene clears those links, hinge targets, and transform
followers before entity destruction so remaining entities cannot dereference a
destroyed neighbor.

The rigid-body solver collects every `HingeComponent` on an entity. This matters
for a tail attached both to its preceding segment and to an end anchor. Constraints
run for eight iterations per physics substep to reduce chain stretch. A rectangular
segment inertia is assigned from its configured length, thickness, and mass.

## Hanging and traversal

`RopeHangComponent` is the legacy player adapter. It requires an explicit grab
action, chooses the nearest segment in the detection radius, and then follows the
segment's current geometry. Vertical input traverses previous/next links instead of
using a stale rope-wide line.

While hanging, the player body becomes kinematic and its main controller is
disabled. Releasing restores the previous body type, gravity scale, and controller
state. A jump release inherits the current segment velocity and adds the configured
upward release speed, preserving swing momentum. If the segment is destroyed or
removed from the supplied world, hanging ends safely.

The legacy component reads analog vertical input from the action event's `value.x`,
matching `InputService` axis encoding. Digital `MoveUp` and `MoveDown` actions are
also supported.

# Player Movement Tuning

The demo player now reads `assets/config/player_settings.json` when the controller is attached. Customize the sections below (values expressed in world units unless noted otherwise) to tune movement, combat, and health without recompiling:

- `movement.ledgeProbeDistance`: how far ahead/down the ledge sensor searches for grab-able geometry (`0.6`).
- `movement.ledgeHorizontalOffset`: horizontal shift from the character center when casting toward a ledge (`0.25`).
- `movement.ledgeVerticalOffset`: upward offset applied before probing so the sensor starts above the grab point (`0.65`).
- `movement.hangVerticalOffset`: how far below the ledge the player is held while hanging (`0.1`).
- `movement.hangClimbOffset`: upward offset applied when climbing up from a hang to place the feet on top of the surface (`0.4`).
- `combat.attackRange`: melee reach used by `CombatComponent` (`90.0`)
- `combat.attackCooldown`: seconds between attack attempts (`0.4`)
- `combat.attackDamage`: base damage triggered (`16.0`)
- `health.maxHp`: actor HP total (`140.0`)
- `health.armor`: damage mitigation add (`6.0`)
- `health.regenRate`: passive HP/sec recovered (`0.5`)

Values are converted through `PhysicsUnits::toUnits` before being applied, so movement keys behave like the previous hardcoded defaults; combat and health values are passed directly to the relevant components.

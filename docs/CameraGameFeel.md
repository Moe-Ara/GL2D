# Camera game feel

GL2D's camera keeps gameplay state separate from presentation. Character input
and physics update the target normally; the camera samples that target, applies
follow composition, and adds bounded visual effects only when building the view
matrix.

## Follow pipeline

The follow position is resolved in this order:

1. Sample the target history using the configured follow delay.
2. Apply the selected follow mode and dead zone.
3. Add velocity-based look-ahead.
4. Smooth toward the composed position using frame-rate-independent damping.
5. Clamp the base camera position to world bounds.
6. Add transient shake, rotation, and zoom presentation offsets.

`HardLock` places the camera at the sampled target immediately. It does not use
damping or look-ahead. A configured follow delay still applies, because it is an
explicit sampling choice. `CenterOnTarget` uses damping and look-ahead, while
`DeadZone` moves only when the target leaves the authored region.

Follow delay should stay subtle in normal gameplay. Rough starting points are:

- responsive platforming: `0.02-0.05` seconds;
- exploration: `0.04-0.08` seconds;
- heavy or dreamlike sequences: `0.08-0.15` seconds.

Long delays make navigation harder. Use damping and dead zones for most of the
weight, with delay as a finishing touch.

## Configuration example

```cpp
Camera camera{1280.0f, 720.0f};
camera.setTarget(&playerTransform, {0.0f, 80.0f});
camera.setFollowMode(CameraFollowMode::DeadZone);
camera.setDeadZoneSize({72.0f, 42.0f});
camera.setDamping(7.5f);
camera.setFollowDelay(0.04f);
camera.setLookAheadMultiplier(0.12f);
camera.setLookAheadLimits({110.0f, 55.0f});
camera.setLookAheadSmoothing(8.0f);
camera.setShakeLimits({24.0f, 16.0f}, 1.5f);
```

The target pointer is non-owning. Clear it with `setTarget(nullptr)` before its
`Transform` is destroyed.

## Cinematic effects

Use short effects at meaningful gameplay transitions:

```cpp
// Landing or strong hit: world-space translation, duration, roughness.
camera.shake(10.0f, 0.18f, 20.0f, impactPosition);

// A restrained landing/combat lens impulse.
camera.pulseZoom(0.02f, 0.18f);

// Cutscene or room reveal.
camera.transitionToZoom(1.25f, 0.6f);
```

Shake uses smooth deterministic noise with a quadratic falloff. Concurrent
effects compose, then the aggregate is clamped by `setShakeLimits`; the camera
also caps the number of live effects so event spam cannot grow memory or produce
unbounded motion. `clearEffects()` is useful when changing scenes or disabling
camera motion for accessibility.

`CameraEvent::Shake`, `Pulse`, `ZoomTo`, and `SnapTo` map to the same behavior.
`ZoomTo::duration` controls easing time. Feeling snapshots can provide persistent
`zoomMul`, `followSpeedMul`, `offset`, `shakeMagnitude`, and `shakeRoughness`
overrides; those values remain separate from the baseline configuration.

## Resize and bounds

Call `setViewportSize` with framebuffer dimensions after a resize. View extents
and world-bound clamping use the effective zoom and rotation, including active
effects. If a world is smaller than the view on one axis, the camera centers that
axis rather than oscillating between impossible limits.

`CameraDebugData` reports raw and delayed target positions, look-ahead, follow
delay, and the final transient effect offsets. These values are intended for
debug overlays and tuning tools.

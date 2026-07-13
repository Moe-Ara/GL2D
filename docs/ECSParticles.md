# ECS particles

GL2D simulates particles at the scene's fixed step and renders them into the
HDR lighting target before bloom and tone mapping. Particle RGB values may
exceed `1.0` for emissive effects.

## Continuous emitter

An emitter entity requires `Transform2D` and `ParticleEmitter2D`. Add
`ParticleRender2D` when it should be visible:

```cpp
ParticleEmitterConfig config{};
config.spawnRate = 60.0f;
config.minLifeTime = 0.3f;
config.maxLifeTime = 0.8f;
config.startColor = {0.2f, 0.8f, 2.0f, 0.7f};
config.endColor = {0.05f, 0.2f, 1.0f, 0.0f};
config.endSizeMultiplier = 0.2f;
config.randomSeed = 42;

const ECS::Entity entity = registry.create();
registry.emplace<ECS::Transform2D>(entity).position = {100.0f, 50.0f};
registry.emplace<ECS::ParticleEmitter2D>(entity, 128, config);
registry.emplace<ECS::ParticleRender2D>(entity).blendMode =
    Rendering::ParticleBlendMode::Additive;
```

The emitter anchor follows its transform. Spawned particles remain in world
space, which prevents trails from sliding when the source moves. `localOffset`
and `targetOffset` are transformed with the entity.

Simulation and presentation are separate components. Headless simulations do
not carry texture or sorting state, and removing `ParticleRender2D` hides an
effect without changing its lifetime.

## Bursts and transient effects

Use `requestBurst`, not `ParticleEmitter::burst`, from ECS gameplay code. The
request is applied after the next fixed-step transform synchronization:

```cpp
auto& particles = registry.get<ECS::ParticleEmitter2D>(entity);
particles.emitting = false;
particles.autoDestroyWhenFinished = true;
particles.requestBurst(24);
```

Deferred ECS destruction keeps the active system query safe. A transient entity
is removed only after emission is disabled and its final particle expires.

## Blend modes and rendering

- `Alpha` is appropriate for smoke, dust, and opaque fragments.
- `Additive` is appropriate for sparks, magic, fire, and other light-emitting
  effects. Additive particles contribute naturally to HDR bloom.

The renderer preserves submission order when blend modes change, culls
off-camera particles before building vertex data, and restores the caller's
OpenGL blend state after the particle pass.

`ParticleRender2D::order` provides deterministic ordering between emitters.
Its optional shared `texture` selects an authored particle sprite; a null
texture uses GL2D's procedural soft radial texture. `tint` changes an emitter's
presentation without mutating simulated particle colors.

## Determinism and validation

Random generation uses `randomSeed` and fixed-step updates, making replays and
tests reproducible. Configuration is validated before pool allocation. Invalid
capacities, unordered ranges, negative rates, non-finite values, or invalid
colors fail with a `ParticleException` containing the relevant constraint.

Drag uses exponential decay, while color and size curves use normalized age.
This avoids frame-rate-dependent damping and supports convincing fade/grow or
fade/shrink effects without per-frame allocations.

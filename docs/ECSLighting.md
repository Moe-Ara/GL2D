# ECS lighting

An ECS-authored light is an entity with `Transform2D` and `Light2D`.
`Transform2D::position` is its world anchor; local offset and direction rotate
with the transform. Scale affects the local offset but does not silently resize
the authored radius.

```cpp
const ECS::Entity light = registry.create();
registry.emplace<ECS::Transform2D>(light).position = {320.0f, 180.0f};
registry.emplace<ECS::Light2D>(light,
    ECS::Light2D::point(400.0f, {0.2f, 0.7f, 1.0f}, 1.6f));
```

Use the `point`, `directional`, and `spot` factories to communicate intent and
start from safe defaults. A cookie is an optional shared texture reference; its
strength must remain in `0..1`.

Add `LightAnimation2D` only to lights that need pulse, flicker, or sweep. This
keeps static light storage compact and makes animation an explicit ECS concern.
Extraction is deterministic for a supplied time and rejects invalid, negative,
or non-finite data before it reaches the GPU.

Point and spot lights are culled using their radius, so lights just outside the
camera still contribute when their influence overlaps the view. Directional
lights are never spatially culled.

The renderer does not inject hidden fill lights. Set the scene's explicit
ambient term when constructing an environment:

```cpp
scene.setAmbientLight({0.08f, 0.1f, 0.16f});
```

Ambient values are linear HDR colors and must be finite and non-negative.

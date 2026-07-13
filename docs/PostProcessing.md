# HDR post-processing

GL2D renders sprites and normals to a floating-point scene target, resolves
lighting into a second HDR target, and performs presentation as a final pass:

1. bright-pixel extraction and half-resolution separable bloom;
2. ACES filmic tone mapping;
3. saturation, contrast, and color tint;
4. aspect-correct vignette;
5. display gamma conversion.

This ordering preserves light and emissive values above `1.0` until tone
mapping. Sprite colors and animation tints may intentionally exceed `1.0` to
produce emission and bloom.

## Scene configuration

Each scene owns its presentation settings. Configure them after constructing
the scene:

```cpp
Scene scene;
auto& post = scene.postProcess();
post.exposure = 1.1f;
post.bloomThreshold = 0.9f;
post.bloomStrength = 0.4f;
post.vignetteStrength = 0.15f;
```

`enabled` disables artistic grading, vignette, and bloom as a group, while
`bloomEnabled` disables only bloom. Required HDR tone mapping and display gamma
conversion always remain active. Bloom runs at half resolution and `bloomIterations` is limited to
`0..16`. Invalid or non-finite settings are rejected before rendering with a
clear exception rather than being forwarded to OpenGL.

Feelings may temporarily override `colorTint`, `vignetteStrength`, and
`bloomStrength`. Scene settings remain unchanged and become active again when
the feeling no longer supplies the corresponding value.

## Rendering order

Post-processing is applied before debug overlays. This keeps diagnostics crisp
and prevents debug geometry from contaminating bloom. Game UI should follow the
same rule unless it is deliberately part of the world image.

# GPU resource lifetime

OpenGL object names belong to the context that created them. GL2D therefore
ties rendering resources to a `Rendering::Renderer` instead of keeping
process-wide framebuffer, shader, tilemap, lighting, or particle state.

## Ownership order

Create the window before GPU-backed objects and destroy those objects before
the window:

```cpp
Graphics::Window window{1280, 720, "Game"};
Rendering::Renderer renderer;

// Game loop and scene rendering.
```

Normal block-scope destruction already gives the correct reverse order. If
either object is held by a smart pointer, reset the renderer and all textures
before destroying the window. A current context is required while destructors
release OpenGL resources.

The renderer lazily owns the scene HDR target, lighting target, lighting pass,
post-processing pipeline, particle renderer, and tilemap renderer. Consequently,
two renderer/window pairs do not share context-local object names or transient
render state. `RenderSystem::renderScene` also reads feeling overrides from the
scene being rendered; rendering another scene cannot mutate global lighting.

## Textures

`TextureManager::loadTexture` requires a current GLFW context. Its cache key is:

- current OpenGL context;
- path;
- color-space variant (linear or sRGB).

Repeated requests using the same three values share a texture. A request from a
different context creates a different OpenGL texture, even for the same file.
Use sRGB for color/albedo art and linear sampling for normal maps, masks, and
other data textures.

Texture ownership must not cross the lifetime of its creating context. If a
texture is accidentally released while another context is current, GL2D avoids
deleting its numeric name in that unrelated context; the owning driver context
will reclaim it when destroyed. Treat that safeguard as damage containment, not
as a substitute for correct destruction order.

## Resize and failure behavior

Render-target resize is transactional. GL2D allocates and validates the new
framebuffer before replacing the old one. If allocation or completeness checks
fail, the previous target remains usable and the exception includes the OpenGL
status or size limit.

Framebuffer dimensions must be positive and cannot exceed
`GL_MAX_TEXTURE_SIZE`. Sprite, particle, tilemap, and frame inputs are validated
before upload so invalid sizes, non-finite transforms, and index overflows fail
near their caller instead of producing undefined rendering.

## Tilemap cache contract

Tilemap meshes are renderer-owned and cached by component. Replace tilemap data
through `TilemapComponent::setData` when authored data changes; consider data
assigned to a component immutable while it is being rendered. Entity transforms
remain live and are applied as model matrices, so moving, rotating, or scaling a
tilemap does not require rebuilding its mesh.

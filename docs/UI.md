# UI runtime

GL2D's UI is a retained tree of `UIElement` objects that produces immutable
`UIRenderCommand` values each frame. The UI renderer consumes those commands
after the scene render. It uses an OpenGL core-profile shader and batches
consecutive commands that share a texture.

## Ownership

- Screens and parents own elements with `std::shared_ptr`.
- Render commands retain texture assets with `std::shared_ptr`; queued commands
  therefore cannot outlive their textures.
- Children are owned by their parent. `addChild` rejects null, duplicate, and
  cyclic relationships.
- `UIRenderer` owns its GPU objects and must be destroyed while its OpenGL
  context is current.

## Layout

Root transforms are resolved against `UIScreen::canvasSize`. Child transforms
are resolved against the rectangle of their immediate parent:

```
world position = parent origin
               + anchor * parent size
               + position
               - pivot * element size
```

`position` is therefore a local offset. `(0, 0)` anchors to the parent's lower
left; `(0.5, 0.5)` anchors to its center; `(1, 1)` anchors to its upper right.
The same resolved bounds drive both rendering and pointer hit-testing.

Canvas dimensions and transform values must be finite. Canvas dimensions must
be positive and element sizes cannot be negative. Invalid JSON fields fail
during load with a contextual `UIException` rather than being silently ignored.

## Rendering and fonts

Create one `UIRenderer` after the OpenGL context has been initialized. Call
`render(commands, framebufferWidth, framebufferHeight)` after world rendering.
Commands use framebuffer-space coordinates with the origin at the lower left.

`setFont(path, pixelHeight)` builds a TrueType atlas. Loading is transactional:
if a replacement font fails, the active atlas remains usable. When no font is
configured, the renderer uses its small built-in ASCII debug font. That fallback
is suitable for diagnostics and development UI; shipped games should configure
a project font.

UI textures, colors, rectangles, text scales, z-indices, and framebuffer sizes
are validated before rendering. Text commands have a bounded input size to keep
malformed content from causing unbounded per-frame allocations.

## Example

```cpp
UI::UIScreen screen;
screen.canvasSize = {1280.0f, 720.0f};

UI::UITransform panelTransform;
panelTransform.size = {360.0f, 180.0f};
panelTransform.anchor = {0.5f, 0.5f};
panelTransform.pivot = {0.5f, 0.5f};

auto panel = std::make_shared<UI::UIElement>("pause_panel", panelTransform);

UI::UITransform buttonTransform;
buttonTransform.position = {20.0f, 20.0f};
buttonTransform.size = {320.0f, 56.0f};
auto button = std::make_shared<UI::Button<>>(
    "resume", buttonTransform, [] { /* resume game */ });

panel->addChild(button);
screen.roots.push_back(panel);
```

Call `screen.update` once per frame with a pointer state, then pass
`screen.collectRenderCommands()` to the renderer.

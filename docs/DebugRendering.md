# Debug rendering and diagnostics

World-space diagnostics use the same core-profile-safe batch path as sprites.
`Rendering::DebugDraw2D` provides lines, points, rectangles, polylines, circles,
and arrows. Call these functions only between `Renderer::beginFrame` and
`Renderer::endFrame`; invalid or degenerate diagnostic geometry is ignored.

`DebugOverlay` is disabled by default so release and demo presentation does not
silently pay for grids, collider outlines, sensor probes, and velocity arrows.
Use `DebugOverlay::toggle()` for an input binding or `setEnabled(bool)` from a
tool. When enabled, `RenderSystem` queues the overlay, collider wireframes, and
ground/wall probes into one batch after post-processing.

The renderer enforces frame ownership:

- a frame cannot begin while another frame is active;
- sprites cannot be submitted outside a frame;
- a frame cannot end twice;
- view/projection matrices, colors, sprite transforms, and UVs must be finite.

These violations throw with contextual messages. The batch explicitly selects
alpha blending and returns the active texture unit to zero after drawing, avoiding
hidden state dependencies between the scene, post-processing, debug, and UI paths.

Gameplay components must never issue OpenGL calls during `update`. If a component
needs visualization, expose immutable diagnostic data and let a render/debug
system submit it. This keeps fixed-step simulation deterministic and allows
headless tests to run without a graphics context.

# Rendering System Backlog (Production-Ready)

The system should deliver:
* Batched 2D rendering for sprites and tilemaps with z-index/layering.
* Parallax support and camera transforms (orthographic).
* Region-based tint/ambient/color grading hooks.
* Debug overlay rendering.
* Asset/material caching (textures, atlases, shaders).

---

## 1. Render Architecture

**Goal:** A clear pipeline from scene data to GPU draws.

### Requirements
1) **RenderContext**
   - Owns GL state, shaders, shared VAOs/VBOs for quads and tile batches.
   - Manages projection/view matrices per camera.
   - Provides frame begin/end, scissor/culling controls.

2) **RenderQueue**
   - Collects draw commands (sprite, tilemap, overlay) with material info and z-index.
   - Sorts by z-index, then texture/material to batch.

3) **Material/Texture Atlas**
   - Abstraction over `Texture` + sampler state; support atlas regions.
   - Cache lookups by id; provide UVs for atlas regions.

4) **Renderable Components**
   - `SpriteComponent`: holds atlas region/texture id, tint, zIndex, optional normal map handle.
   - `TilemapComponent`: holds tile atlas id, tile grid, zIndex, collision flag; can build mesh for visible tiles.
   - Debug overlay: line/quad batching for gizmos.

5) **Camera**
   - Orthographic projection; parallax layers adjust view based on scroll factors.
   - Apply camera bounds/dead zone/look-ahead.

### Risks
* Overdraw/too many state changes without batching.
* Sorting instability if z-index ties are not handled consistently.

---

## 2. Sprite Rendering

**Goal:** Efficiently draw thousands of sprites.

### Steps
1) **Data model**
   - Extend `SpriteComponent` with: `std::string textureId/atlasRegion`, `glm::vec4 tint`, `int zIndex`, optional `glm::vec4 uv` override.
   - Preserve pointer to `GameObjects::Sprite` if needed, but move to id-based lookup for batching.

2) **Batching**
   - Shared quad VBO/IBO; dynamic instance buffer with model matrix + UV + tint.
   - Sort `SpriteDraw` items by texture/atlas, then issue instanced draws per batch.

3) **Transform**
   - Combine owner `TransformComponent` model matrix with sprite size/UV; feed to instance data.

4) **Shader**
   - Basic textured quad shader with tint; optional normal map hook for future lighting.

5) **Debug**
   - Optionally render sprite bounds in debug overlay.

### Risks
* Mixing pointer-based sprites and atlas ids; choose one path for consistency.
* GPU buffer reallocation if instance buffer not sized/reserved.

---

## 3. Tilemap Rendering

**Goal:** Draw large tile layers efficiently with culling.

### Steps
1) **Data model**
   - `TilemapComponent` stores tile grid, tile size, atlas id, zIndex, collision flag.

2) **Mesh build**
   - On load or when tile data changes, build a vertex buffer (positions + UVs) for all tiles.
   - Frustum-cull tiles against camera; stream only visible tiles each frame or pre-bake chunks.

3) **Batching**
   - Group draws by atlas id; one draw per tile layer chunk.
   - Support zIndex ordering with sprites (render before/after based on z).

4) **Parallax**
   - Apply parallax offsets via view matrix if tile layer is tagged in `LevelLayer`.

### Risks
* Large VBO rebuilds per frame if not chunked.
* Floating z fighting if sprites share same zIndex—define consistent offset rules.

---

## 4. Layering & Sorting

**Goal:** Deterministic draw order.

### Steps
1) Gather renderables from the current `Level`: sprites, tilemaps, overlays.
2) Compute an effective sort key: `(zIndex, materialId, type)`.
3) Stable-sort the queue each frame.
4) For parallax layers, adjust view matrix before emitting their draws.

### Risks
* Misaligned zIndex ranges between systems; document ranges (e.g., backgrounds negative, gameplay 0..100, UI high).

---

## 5. Camera Integration

**Goal:** Camera drives view/projection; parallax honored.

### Steps
1) Build an ortho projection from camera bounds; view from camera position/look-ahead.
2) Apply dead zone/look-ahead adjustment for follow mode.
3) Parallax per layer: `parallaxView = view * translate(scroll * cameraPos)`.
4) Expose camera matrices to shaders.

### Risks
* Inconsistent world units vs. camera bounds; document units (pixels/meters).
* Parallax math drift if scroll not clamped.

---

## 6. Region Tint/Ambient

**Goal:** Support area-based atmosphere.

### Steps
1) From `LevelData::regions`, determine active region(s) per draw (e.g., based on transform position).
2) Apply tint/ambient to sprite/tile colors in shader (uniform or per-instance).
3) For overlap, choose topmost or blend (define rule).

### Risks
* Region lookup per sprite can be O(n); use spatial partitioning if needed.

---

## 7. Debug Overlay Rendering

**Goal:** Visualize bounds/triggers/camera.

### Steps
1) Build a simple line/quad batcher for gizmos.
2) Render after scene with a flat-color shader; no depth.
3) Draw: entity bounds, trigger volumes (color by state), camera bounds, grid, region boxes.

### Risks
* Overlay draw order clutter—allow toggles and filtering.

---

## 8. Asset & Material Management

**Goal:** Centralize texture/atlas/shader lifetimes.

### Steps
1) Texture/Atlas manager: load atlas, expose UV lookup by region id.
2) Material cache: key by (shader, texture, samplers, blend mode).
3) Ensure RenderQueue holds material ids, not raw pointers.

### Risks
* Asset cache invalidation on hot-reload; plan for refresh.

---

## 9. API Surface & Integration

**Goal:** Make it easy to render a Level.

### Steps
1) Add `RenderSystem::render(const Level& level, const Camera& camera)` entry.
2) Provide helper `enqueueSprite(Entity&)`, `enqueueTilemap(Entity&)` called from systems.
3) Tie into main loop: `renderSystem.beginFrame() -> gather -> render -> endFrame()`.

### Risks
* Mixing immediate and queued draws; stick to queued for consistency.

---

## 10. Validation & Tooling

**Goal:** Confidence in correctness/performance.

### Steps
1) Add a test scene with many sprites/tile layers to measure batching and FPS.
2) Add debug counters (draw calls, batches, textures bound).
3) Visualize z-index sorting order in the debug overlay.

---

## 11. Screen Buffers / Render Targets

**Goal:** Support off-screen rendering for post-process, scaling, and capture.

### Steps
1) Add a `RenderTarget` wrapper (color texture + optional depth/stencil) with create/destroy/resize/bind.
2) Integrate into Renderer/ParticleRenderer: `beginFrame` binds the target; `endFrame` blits/draws a fullscreen quad to the default framebuffer.
3) Wire resize handling via the window framebuffer callback to resize attachments.
4) Expose config: enable/disable, render scale (e.g., 0.75), and clear color override.

### Risks
* Forgetting to unbind FBO can break UI/debug overlays.
* Resolution mismatch if resize events are missed; assert/guard against zero-sized attachments.
* MSAA resolve path needed if multisampling is enabled.

---

## Docs

- `docs/RenderingGuide.md`: pipeline overview, z-index conventions, parallax usage, region tint rules, how to add a new renderable component.
- Update this backlog when adding lighting/normal maps.

# Level Building System Backlog (Production-Ready)

The following plan assumes the engine uses:
* Lightweight ECS-style composition with `Transform`, `SpriteRenderer`, `Collider`, `ScriptComponent`, etc.
* Batched sprite rendering via OpenGL + texture atlases + orthographic projection (GLM).
* Shared asset ownership via `std::shared_ptr` and per-instance overrides stored in components.
* A future roadmap including lighting (normal maps), scripting, and physics.

---

## Architectural Overview

```
                        ┌────────────────────┐
                        │ Prefab Catalog     │
                        │ (shared specs)     │
                        └────────┬───────────┘
                                 │ lookups
                                 ▼
 ┌──────────────┐    ┌────────────────────┐    ┌────────────────────────┐
 │ Level Schema │ -> │ Level Loader       │ -> │ Runtime Level Manager  │
 └──────────────┘    └────────────────────┘    └────────────┬───────────┘
                                                         spawn entities
                                                         register triggers
                                                         configure camera

                       ┌────────────────────┐
                       │ Level Editor       │
                       │ (authoring tool)   │
                       └────────┬───────────┘
                                │ saves schema
                                ▼
                        Level Files (.json)
```

Hidden dependency callouts:
* Renderer must support instancing/batching across thousands of prefab instances.
* Asset pipeline must guarantee atlas slots and shader variants exist before prefabs reference them.
* Trigger runtime depends on cinematic/event systems and must fail gracefully if scripts are missing.
* Future lighting/physics will need hooks in the schema and prefabs (normals, collision data).

---

## 1. Prefab / Root Catalog

**Goal:** One authoritative source of reusable “roots” so instances stay lightweight.

### Requirements
1. **Data model**
   - `PrefabDefinition` struct containing:
     - `std::string id;`
     - `std::shared_ptr<Texture>` or atlas region descriptor.
     - Animation/state machine references.
     - Default component list (Transform defaults, SpriteRenderer config, Collider shapes, Script metadata).
     - Optional lighting data (normal map handles, emissive values) and future physics params.
     - Allowed override mask (e.g., position, tint, script params). Store as bitset for runtime fast checks.
2. **Catalog service**
   - `PrefabCatalog::registerDefinition(PrefabDefinition&& def)` validates unique IDs and required assets.
   - `const PrefabDefinition& getPrefab(const std::string_view id)`; log + throw on missing entries.
   - Keep catalog immutable at runtime to avoid threading races.
3. **Asset loading**
   - Prefab definition files (JSON/TOML). Example:
     ```json
     {
       "id": "env.tree.pineTall",
       "atlasRegion": "forestAtlas:12",
       "components": [
         { "type": "SpriteRenderer", "zIndex": -5, "tint": [0.9, 0.95, 1.0] },
         { "type": "Collider", "shape": "AABB", "size": [32, 192], "flags": ["static"] }
       ],
       "overrides": ["Transform", "SpriteRenderer.tint", "Script.params"]
     }
     ```
   - Loader should verify texture/animation references exist; integrate with asset hot-reload.
4. **Memory considerations**
   - Prefab definitions are shared pointers; instances store only `PrefabId`, `Transform`, and override data.
   - For rendering, compute atlas UV once in prefab and reuse when populating `SpriteRenderer`.
5. **ECS integration**
   - Provide helper `Entity instantiatePrefab(const PrefabDefinition&, const PrefabOverrides&)`.
   - Ensure components are attached using engine’s ECS creation API to avoid manual ownership leaks.

### Risks
* Missing asset references causing runtime crashes → mitigate by validating at catalog load time.
* Prefab updates invalidating existing level files → version each prefab and enforce migrations.

---

## 2. Level Schema Definition

**Goal:** Versioned, future-proof representation of authored content.

### Schema Outline
```json
{
  "schemaVersion": 2,
  "metadata": { "name": "ForestIntro", "author": "mohamad", "build": "2025-11-20" },
  "camera": {
    "bounds": [ -512, -256, 4096, 1024 ],
    "followMode": "DeadZone",
    "lookAhead": { "multiplier": 0.15, "limit": [ 200, 120 ] }
  },
  "layers": [
    { "id": "bg_parallax01", "type": "parallax", "texture": "env/bg_forest.png", "scroll": [0.3, 0.1] }
  ],
  "instances": [
    {
      "prefabId": "env.tree.pineTall",
      "m_transform": { "pos": [120, 64], "scale": [1.0, 1.0], "rot": 0.0 },
      "overrides": { "SpriteRenderer.tint": [0.8, 0.9, 1.0], "Script.params": { "shake": true } }
    }
  ],
  "triggers": [
    {
      "id": "intro_cam_shake",
      "shape": { "type": "AABB", "pos": [300, 0], "size": [128, 256] },
      "event": "camera.shake",
      "params": { "intensity": 0.6, "duration": 0.5 },
      "activation": "onEnter"
    }
  ]
}
```

### Technical Considerations
1. Use JSON Schema or C++ schema validation library to enforce types.
2. Maintain `schemaVersion` and provide migration scripts (`tools/level_migrate.py v1 v2`).
3. Coordinate conventions:
   - Origin at bottom-left (world units matching meters or pixels? Document clearly).
   - Y-up to align with GLM coordinate expectations.
4. Future hooks:
   - Reserve fields for lighting volumes, physics volumes, dialogue tracks.
   - Support `customData` blob per instance for game-specific logic (store as JSON object).

### Risks
* Schema drift vs. editor version mismatch → embed `editorVersion` and refuse to load incompatible levels.
* Numeric precision for large worlds → use floats but consider double for camera bounds metadata.

---

## 3. Level Loader Implementation

**Goal:** Deterministic instantiation of ECS entities + systems from serialized data.

### Steps
1. `LevelLoader::load(const std::string& path, PrefabCatalog&, AssetManager&) -> LevelHandle`
   - Read file, validate with schema. Fail fast with descriptive errors (line, column, field path).
   - Iterate `instances`:
     - Lookup prefab; if missing, log error, optionally spawn placeholder prefab.
     - Create ECS entity (`Entity entity = registry.create();`).
     - Attach components per prefab defaults, then apply overrides (ensure type-safe conversions).
     - Register entity in `Level` object with metadata (so editor/debug overlay can map entity ↔ authoring data).
   - Configure camera based on `camera` block (dead zone, bounds). Update `Camera` component/system.
   - Register triggers: create `TriggerComponent` with shape data; link to `TriggerSystem`.
2. Provide streaming-friendly API: `LevelLoader::loadAsync(...)` to support future background loading.
3. Error Handling:
   - Distinguish between fatal errors (missing required asset) and soft warnings (out-of-bounds instance).
   - Collect diagnostics set returned to editor/CLI.
4. Performance:
   - Use pooled memory for temporary JSON parsing buffers.
   - Batch entity creation (reserve registry capacity) to avoid reallocation.
   - For rendering, ensure sprites reuse sprite batches; update `SpriteRenderer` with atlas index.

### Interactions
* Prefab catalog for shared data.
* Renderer for sprite batching (apply UVs during component setup).
* Camera system for bounds/follow parameters.
* Trigger/event system for hooking triggers to cinematic/effects.
* Future systems (lighting/physics) should have optional data blocks; loader must skip gracefully when unavailable.

### Risks
* Out-of-sync asset versions causing mismatched components → include hash of prefab definition in level file to detect.
* Long load times for large JSON files → consider binary cache once schema stabilizes.

---

## 4. Authoring Tool (Editor) MVP

**Goal:** Fast iteration environment for designers/yourself.

### Feature Breakdown
1. **Mode Toggle**
   - `--editor` CLI flag or runtime console command enters edit mode (disables gameplay systems, enables editor UI).
   - Editor uses same renderer/camera but overlays gizmos (ImGui or custom UI).
2. **Prefab Palette**
   - Query `PrefabCatalog` for categories/tags.
   - Provide search/filter, thumbnails (generated from atlas or preview rendering).
3. **Placement & Gizmos**
   - Grid snapping with configurable cell size; optional angle snapping.
   - Gizmos for translate/rotate/scale (ImGuizmo or custom).
   - Duplication, multi-select, group transforms.
4. **Layers & Hierarchy**
   - Tree view showing parallax layers, instances, triggers.
   - Lock/hide layers to avoid accidental edits.
5. **Inspector**
   - Shows prefab ID, instance overrides, script parameters.
   - Validate entries in-place (clamp ranges, show warnings).
6. **Undo/Redo**
   - Command stack capturing edits (`AddInstance`, `TransformInstance`, `DeleteTrigger`).
   - Persist stack per scene; support unlimited undo depth (within memory constraints).
7. **Save/Load**
   - Serialize to schema; pretty-print JSON for human diffing.
   - Autosave with `.autosave` suffix; track dirty state.
8. **Performance**
   - Editor must handle thousands of instances; use spatial partitioning (quad-tree) to cull gizmo rendering.
   - Provide freeze/thaw for expensive components (e.g., animated prefabs) while editing.

### Hidden Dependencies
* Needs input system integration for editor shortcuts; ensure conflicts with gameplay bindings are resolved.
* Requires robust asset preview pipeline (thumbnail caching) especially when atlases change.

---

## 5. Runtime Level Manager

**Goal:** Central authority over level lifecycle, streaming, and hot-reload.

### Design
1. `class LevelManager`
   - Holds `std::unique_ptr<Level>` currentLevel.
   - `bool loadLevel(const std::string& path);`
   - `void unloadLevel();`
   - `void queueTransition(const std::string& nextPath, TransitionParams params);`
   - `void update(double dt)` handles transition state machine (fade out/in, streaming).
2. **Hot Reload**
   - Watch level file + referenced prefabs (timestamp or file hashing).
   - On change, call `loadLevel` and preserve persistent systems (player state) as needed.
3. **Events**
   - Use event bus to broadcast `LevelLoaded`, `LevelUnloaded`, `LevelWillUnload`.
   - Systems like AI spawners, audio, scripting subscribe to manage their own state.
4. **Memory/Performance**
   - Unload must destroy ECS entities, release GPU resources (if any).
   - Provide streaming hooks to load parallax backgrounds or large meshes asynchronously.
5. **Error Handling**
   - If load fails, fallback to previous level or show debug overlay error.
   - Keep error log accessible via console for designer feedback.

### Future-Proofing
* Level streaming/segmentation for large worlds (load neighbor chunks).
* Save/load integration (serialize player + level state).

---

## 6. Trigger / Event Authoring Support

**Goal:** Data-driven hooks to tie gameplay, cinematics, and camera effects.

### Implementation Steps
1. **Schema**
   - Support shapes: AABB, circle, polygon. Store local-space coordinates to allow moving triggers with prefabs.
   - Activation logic: `onEnter`, `onExit`, `whileInside`, `manual`.
   - Event payload references `EventGraph` ID or script function.
2. **Runtime**
   - `TriggerSystem` iterates `TriggerComponent`s, checks collisions using the engine’s collision queries (even if basic).
   - On activation, send `TriggerEvent` to event bus (camera, dialogue, scripting).
   - Debounce to prevent spamming (cooldowns).
3. **Editor**
   - Visualization: tinted volume outlines, handles for resizing.
   - Inspector for event params (dropdown for known events).
4. **Future Hooks**
   - Integration with cinematic timeline editor.
   - Trigger-driven asset streaming (preload area assets when player approaches).

### Risks
* Without physics, collision detection may be approximate; ensure triggers use same m_transform as player controller to avoid mismatch.
* Complex event graphs may require dependency resolution; design for composition early.

---

## 7. Debug Visualization Overlay

**Goal:** Real-time insight into level data/state while playing or editing.

### Features
1. Toggle via `F3` or console command; overlay draws using existing sprite batch (colored lines/quads).
2. Visual elements:
   - Bounding boxes with prefab ID labels.
   - Trigger shapes with current activation state color (e.g., green idle, yellow armed, red active).
   - Camera bounds, dead zone, look-ahead vector arrow.
   - World grid and coordinate axes.
3. Performance:
   - Use GPU instancing for overlays if possible; otherwise batch line vertices into one draw call.
   - Culling via camera frustum to avoid drawing off-screen debug data.
4. Diagnostics panel (ImGui):
   - List missing prefabs, invalid overrides, hot-reload status.
   - Display level metadata (version, author, file path).

---

## 8. Validation / Tooling Pipeline

**Goal:** Automated QA for authored content before runtime.

### CLI Tool (`tools/validate_levels.py` or C++ equivalent)
1. Input: directory or individual level file.
2. Checks:
   - JSON schema validation.
   - Prefab ID existence (query catalog).
   - Override type safety (e.g., color arrays of length 3).
   - Camera bounds sanity (min < max, contains spawn area).
   - Trigger envelopes (non-zero size, valid event IDs).
   - Optional: static analysis for overlapping interactables or unreachable triggers.
3. Output:
   - Structured report (JSON/HTML) with severity levels.
   - Console summary for CI logs.
4. Integration:
   - Hook into pre-commit (run on changed level files).
   - CI job ensuring `tools/validate_levels` passes before merge.

### Performance
* Cache prefab lookups to avoid reloading catalog per file.
* Parallelize validation over multiple level files (Python multiprocessing or C++ threads).

---

## 9. Documentation & Workflow

**Goal:** Keep contributors aligned and reduce onboarding friction.

### Deliverables
1. `docs/LevelAuthoringGuide.md`
   - Prefab definition workflow (folder structure, naming, asset import).
   - Level editor tutorial (hotkeys, best practices, troubleshooting).
   - Validation tool usage, interpreting reports.
   - Glossary for schema fields and override options.
2. `docs/PrefabSpecification.md`
   - Detailed component mapping (SpriteRenderer fields, Collider flags, Script params).
   - Examples for advanced cases (animated props, scripted triggers).
3. Keep docs versioned with schema; include changelog when fields are added/removed.

---

## Cross-System Considerations

| Area            | Key Considerations                                                                                                                                         |
|-----------------|------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Rendering       | Prefabs should reference atlas regions; loader must batch sprites by texture to minimize draw calls. Future lighting requires storing normal map handles. |
| Memory          | Use shared ownership for heavy assets, but keep per-level allocations in arenas for easy cleanup on unload.                                               |
| ECS Integration | Ensure component creation honors ECS constraints—e.g., register systems for new component types used in prefabs.                                           |
| Physics         | Even if deferred, reserve schema fields for colliders so physics drop-in later doesn’t require mass refactor.                                              |
| Scripting       | Triggers/events should route through script system; include script binding names in schema.                                                                 |
| Error Handling  | Each stage (catalog load, level load, editor save) must collect and surface errors with context so designers can act immediately.                          |
| Scalability     | Design loader/editor to handle thousands of instances—optimize data structures, avoid O(n²) operations.                                                     |

---

## Next Steps Checklist
1. Implement `PrefabCatalog` (LEV-001).
2. Finalize JSON schema + migration tooling (LEV-002).
3. Build Level Loader + Level Manager integration (LEV-003/LEV-005).
4. Stand up editor MVP (LEV-004) and tie into schema save/load.
5. Extend triggers/events, debug overlay, validation pipeline, and docs (LEV-006 thru LEV-009).

This backlog is intentionally detailed so each ticket can be handed to an engineer without ambiguity, while still leaving room for iteration as lighting/physics systems come online. Keep the document updated whenever schema or subsystem contracts change.***

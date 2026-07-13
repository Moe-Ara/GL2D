# Feelings and presentation transitions

The feelings system coordinates temporary presentation and gameplay modifiers
without baking cinematic state into individual renderers or controllers. A
`FeelingSnapshot` is authored data; `FeelingsManager` owns the current blend;
`Scene` advances that blend once per frame.

## Timing contract

Call `Scene::advance(frameDeltaSeconds)` once per rendered frame. Feelings use
the unscaled frame delta, then the blended `timeScale` is applied only to the
fixed-step simulation. This has three useful consequences:

- camera, lighting, and post-processing blends remain smooth during slow motion;
- a `timeScale` of zero pauses simulation without freezing its own transition;
- multiple fixed steps in one frame do not advance a presentation blend more
  than once.

`Scene::update` is an exact simulation-step API and intentionally does not
advance feelings. Prefer `advance` in game loops.

`Scene::updateWorld` applies the current snapshot to its camera before updating
and rendering it. A custom game loop using `Scene::advance` should do the same:

```cpp
scene.advance(frameDelta);
camera.applyFeeling(scene.feelings().getSnapshot());
camera.update(frameDelta);
RenderSystem::renderScene(scene, camera, renderer);
```

## Blending behavior

Numeric overrides blend through a semantic neutral value when either endpoint
is absent. Multipliers use `1`, additive values use `0`, color tints use white,
and offsets use zero. An effect therefore fades in from neutral and fades back
to neutral before the optional override is cleared; it does not snap on or off.

Post-processing feelings compose with authored scene settings:

- `colorGrade` multiplies the scene tint;
- `vignette` adds to the authored vignette and clamps to `[0, 1]`;
- `bloomStrength` adds to authored bloom;
- lighting multiplier/additive fields compose with scene and light values.

String identifiers such as music, palette, particle, and audio presets are
discrete. A defined target identifier becomes active when the transition
starts; an identifier omitted by the target remains until the transition ends.

## Definitions and timed effects

Load the JSON definitions, choose an explicit default, and optionally connect
external targets:

```cpp
FeelingsSystem::FeelingsController feelings{scene.feelings()};
feelings.setDefinitions(
    FeelingsSystem::FeelingsLoader::loadMap("assets/config/feelings.json"),
    "default");
feelings.setTargets(&camera, nullptr, &audio);

if (!feelings.setFeeling("heavy_landing", 350.0f)) {
    // Unknown id: report the authored event/configuration error.
}
```

Durations and blend values use milliseconds. A positive duration automatically
returns to the default using the active feeling's `blendOutMs`. A missing or
zero duration persists until another feeling is selected. `setDefinitions`
rejects a missing default and mismatches between map keys and snapshot ids.

Call `FeelingsController::update(realDeltaMs)` once per frame for duration timers
and external camera/particle/audio targets. It does not advance the manager;
`Scene::advance` owns that responsibility.

## Automatic consumers

The scene/render pipeline currently applies these fields automatically:

- camera: zoom, offset, follow speed, shake magnitude, shake roughness;
- simulation: time scale;
- ECS and legacy character motion: entity speed and acceleration speed;
- ECS and legacy animation: animation speed;
- rendering: color grade, vignette, bloom, light intensity/radius/color, and
  ambient light multipliers/addition;
- renderer-owned particles: ambient particle tint;
- audio connected through `FeelingsController`: music and SFX volumes and music
  track changes.

Other snapshot fields are validated and blended data for their named subsystem,
but are not silently given unrelated behavior. Combat code should consume
`damageMul`/`armorMul`; UI code should consume `uiTint`/`uiLerpSpeed`; fog,
palette, particle-preset, and advanced audio-FX fields require the corresponding
runtime integration.

## Validation

JSON and programmatic snapshots share the same validation contract. Blend
times and multipliers must be finite and non-negative, zoom must be positive,
unit-range fields stay in `[0, 1]`, and colors reject non-finite or negative
channels. Loader errors include the feeling id and exact offending field.

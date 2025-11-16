# Animation Metadata Format

Animations are described through JSON documents that live next to your art assets (for example `assets/character/animations.json`).  
Each file can declare an optional sprite atlas, a global default frame duration, and a set of named animations with their transitions.

```json
{
  "initialState": "SlimeIdle",
  "defaultFrameDuration": 0.12,
  "atlas": {
    "texture": "assets/character/slime_atlas.png",
    "rows": 4,
    "cols": 8
  },
  "animations": [
    {
      "name": "SlimeIdle",
      "loop": true,
      "playback": "forward",
      "frameDuration": 0.1,
      "transitions": [
        { "target": "SlimeBlink", "condition": "triggerBlink" }
      ],
      "frames": [
        { "row": 0, "col": 0, "duration": 0.08 },
        { "row": 0, "col": 1, "duration": 0.12 },
        { "uv": [0.25, 0.0, 0.5, 0.25], "event": "blink_peak" },
        { "texture": "assets/character/Blu_Slime_Idle_0004.png" }
      ]
    }
  ]
}
```

### Root fields

| Field | Type | Required | Description |
| --- | --- | --- | --- |
| `initialState` | string | optional | Name of the animation that should boot the state machine. Falls back to the first animation or one containing “Idle”. |
| `defaultFrameDuration` | number | optional (0.12s default) | Global fallback duration used whenever an animation/frame omits its own duration. |
| `atlas` | object | optional | Declares a shared sprite-sheet texture. Frames can reference rows/cols/UVs and inherit this texture. |
| `animations` | array | required | Collection of animation definitions. |

### Atlas object

| Field | Type | Description |
| --- | --- | --- |
| `texture` | string | Path to the shared texture. Leave empty to rely on per-frame textures. |
| `rows` / `cols` | integer | Number of grid rows and columns in the atlas. Used when frames specify `row`/`col`. |

### Animation entry

| Field | Type | Required | Description |
| --- | --- | --- | --- |
| `name` | string | yes | Unique identifier used when building animation states or transitions. |
| `loop` | bool | optional (true) | Whether the animation repeats when it reaches the end. |
| `playback` | string | optional (`forward`) | Playback mode: `forward`, `reverse`, or `pingpong`. |
| `frameDuration` | number | optional | Default duration for frames in this animation. |
| `transitions` | array | optional | Metadata for state-machine transitions. `condition` matches a signal name resolved in code. |
| `frames` | array | yes | Sequence of frames. |

### Frame entry

| Field | Type | Description |
| --- | --- | --- |
| `row` / `col` | integer | Address a grid cell in the atlas. Requires atlas metadata. |
| `uv` | array[4] | Explicit UV rectangle `[uMin, vMin, uMax, vMax]` for complex layouts. |
| `texture` | string | Path to a standalone texture for this frame. Takes precedence over atlas texture. |
| `duration` | number | Optional override for frame timing. |
| `event` | string | Optional token dispatched through `Animator::setFrameEventCallback`. |

### Transitions

Each transition entry declares a `target` animation name and a `condition` string. During bootstrapping the loader wires transitions into `AnimationState`s and asks the application to supply condition callbacks. The demo uses simple boolean “signals” that toggle on animation events or timers, but you are free to drive them from input, AI, or gameplay data.

### Adding new animations

1. Drop the new textures (or atlas) into `assets/`.
2. Append another animation entry with a unique `name`.
3. Describe its frames. Use `texture` for individual files or `row`/`col` + `uv` when sharing an atlas.
4. Declare the transitions you'd like the `AnimationStateMachine` to automatically create.
5. Update the application code to provide the transition condition callbacks (for example toggling a signal when a key is pressed).

See `assets/character/animations.json` for a concrete example in the repository.

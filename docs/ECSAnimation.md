# ECS Animation

GL2D's ECS animation path treats animation graphs as immutable shared resources and
all playback/presentation state as entity-local data. This prevents the legacy failure
mode where animating one shared `Sprite` changed UVs, textures, color, or facing for
every entity using that resource.

## Data and systems

- `AnimationGraph2D` validates states, clips, frame durations, transitions, and the
  initial state once when the resource is constructed. Transitions are compiled to
  state indices so fixed-step evaluation does not perform state-name lookup.
- `Animator2D` stores current state/frame timing, playback direction, speed, explicit
  state requests, and completion state.
- `AnimationParameters2D` supplies boolean and float transition parameters.
- `AnimationEventQueue2D` receives state entry/exit, named frame, loop, and completion
  events. `Scene::advance` clears it once per rendered frame, so events from every
  fixed substep remain observable.
- `CharacterAnimationParameterSystem2D` publishes speed, vertical velocity, grounding,
  rising, falling, and movement parameters after collision resolution.
- `AnimationSystem2D` evaluates transitions and advances looped, one-shot, reverse,
  and ping-pong clips after gameplay and physics.

`SpriteRender` owns per-entity tint, animation tint, UVs, facing, and texture overrides.
The renderer resolves those values into draw data without mutating the shared sprite.
Gameplay tint and animation tint are multiplied independently, allowing hit flashes,
feeling effects, or fades without being overwritten by animation frames.

## Data-driven loading

`AnimationGraphLoader2D` converts the established animation JSON schema into an ECS
graph. It supports atlas/custom UV frames, per-frame duration and events, texture and
normal-map resolution, playback modes, and simple transition expressions such as:

- `moving`
- `!grounded`
- `speed > 100`
- `verticalVelocity <= 0`
- `grounded == true`

Asset path policy stays with the caller-provided texture resolver, keeping graph
loading independent of a specific game directory or global texture manager.

The `ECSPlatformer` demo uses a shared graph to drive idle, run, rise, and fall states
from live motor/contact data. Its tint-based frames deliberately avoid external art so
the animation state pipeline remains runnable in automated OpenGL smoke tests.

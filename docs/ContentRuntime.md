# Content runtime and ownership

GL2D's content path separates reusable templates from runtime instances. Prefab
specifications and resource registries locate templates; entities and components
own the runtime objects they use.

## Resource registries

`SpriteManager`, `AnimatorManager`, and `AnimationStateMachineManager` are
non-owning caches. Registration accepts `std::shared_ptr`, the cache stores a
`std::weak_ptr`, and `get` returns shared ownership only while the asset is alive.
Expired entries are removed lazily and are never returned as raw pointers.

Asset owners must keep their template alive for as long as factories may instantiate
it. A component created from an animator or state-machine template holds shared
ownership. Sprite components receive a clone of the registered sprite template so
per-entity UV, color, flip, and texture changes do not mutate every instance.

Live duplicate IDs are rejected instead of silently replacing a resource that
existing content may still reference. Registries provide explicit unregister and
clear operations for content reload and tests. They are main-thread content APIs;
concurrent registration and lookup is not supported.

`SpriteComponent`, `AnimatorComponent`, and
`AnimationStateMachineComponent` accept shared ownership only. The former raw
pointer overloads and no-op deleters were removed because they disguised borrowed
lifetimes as ownership.

## Factory diagnostics

`ComponentFactory` treats a missing or expired sprite, animator, or animation-state
machine ID as invalid content and throws with the component type and referenced ID.
It does not create a component containing a null runtime dependency. Trigger specs
likewise require a non-empty event ID and a recognized activation name.

This makes prefab creation fail fast. Load/register required assets before
instantiating prefabs, and surface the exception to authoring tools instead of
continuing with a visually or behaviorally incomplete entity.

## Level validation

`LevelLoader` currently accepts schema version 1. Loading fails with a
`LevelException` for:

- unsupported schema versions;
- unknown prefab or tilemap references;
- duplicate or empty trigger IDs;
- empty trigger event IDs;
- non-finite trigger positions or parameters;
- invalid trigger dimensions; or
- unsupported trigger shapes and activation names.

`LevelManager::loadLevel` preserves the previous level when parsing fails and makes
the diagnostic available through `lastError()`. `takeCurrent()` transfers the
successfully loaded level when a caller wants to install its entities into a scene.

```cpp
LevelManager levels;
if (!levels.loadLevel("assets/levels/level1.json")) {
    throw std::runtime_error(levels.lastError());
}

auto level = levels.takeCurrent();
for (auto& entity : level->entities) {
    scene.addEntity(std::move(entity));
}
```

Loading and scene installation should occur outside an active scene update. Scene
mutation during callbacks is deferred, but replacing a whole level is a
frame-boundary operation.

## Authored triggers

Level trigger geometry is represented once, by `ColliderComponent`:

- `AABB`/`Box` uses a center position and positive size;
- `Circle` uses a center position and positive radius.

The collider is marked as a trigger and participates in BVH pair generation,
collision masks, and exact narrowphase. `TriggerComponent` contains only the event
ID, numeric parameters, activation mode, and callback. This avoids the previous
placeholder shape data drifting away from physics geometry.

Activation modes have distinct behavior:

- `OnEnter` fires once when each overlap begins.
- `WhileInside` fires every fixed physics step while the exact overlap remains.
- `OnExit` fires when an active exact overlap ends.
- `Manual` ignores collision transitions and fires only through `activateManual`.

Attach a callback after loading to route authored IDs into game-specific camera,
dialogue, scripting, or cinematic code:

```cpp
trigger.setCallback([](Entity& owner, Entity* other,
                       const TriggerComponent& event) {
    dispatchGameplayEvent(event.eventId(), event.parameters(), owner, other);
});
```

Callbacks run synchronously during the scene trigger phase. Scene entity creation
and destruction requested there is deferred to the safe mutation boundary.


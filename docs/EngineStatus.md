# Engine status and production roadmap

A living record of GL2D's validated state and the work remaining to reach the
"visually rich, responsive 2D platformer" target (Hollow Knight / Ori class).
Update it at the end of each stabilization pass.

## Validated state (last pass)

- **Tests:** 149 Boost.Test cases pass. Build with `-DGL2D_BUILD_TESTS=ON` and
  run `ctest --test-dir build --output-on-failure`.
- **Memory / UB:** the full suite runs clean under AddressSanitizer +
  UndefinedBehaviorSanitizer (no leaks-mode false positives suppressed):
  ```bash
  cmake -S . -B build-asan -DGL2D_BUILD_TESTS=ON -DGL2D_BUILD_DEMO=OFF \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
  cmake --build build-asan --target GL2D_TEST -j
  ASAN_OPTIONS=detect_leaks=0 ./build-asan/GL2D_TEST
  ```
- **Warnings:** the engine library compiles clean under `-Wall -Wextra`.
- **ECS throughput:** `GL2D_ECS_BENCHMARK 100000 200` iterates the ECS registry
  ~6.8x faster than the legacy `Entity` component path (46 ms vs 314 ms), i.e.
  ~0.23 ms/frame for 100k entities. This is the case for continuing the ECS
  migration.

## Recently landed

- Contact-material friction + restitution in the impulse solver (combined via
  geometric-mean friction / max restitution, Coulomb-clamped, resting-contact
  suppression). See [Physics.md](Physics.md).
- `Scene` owns its clear color; `RenderSystem` no longer hardcodes it.
- Engine-level parallax: `ParallaxLayer2D` + `ParallaxSystem2D`, run as a
  presentation pass by `RenderSystem`. See [ECSArchitecture.md](ECSArchitecture.md).
- The Lost Heroin demo migrated its backgrounds onto the parallax component and
  advances environmental-storytelling "chapters" through engine trigger volumes.

## Roadmap (prioritized)

This is continuous work; each item should ship as a vertical slice with tests,
sanitizer coverage, and a before/after measurement, per the ECS migration policy.

1. **Render interpolation.** `FixedStepClock` already exposes
   `interpolationAlpha`, but extraction draws raw simulation transforms. Store
   previous+current fixed-step transforms and interpolate at draw time to remove
   micro-stutter when display rate and sim rate diverge. Highest game-feel ROI.
2. **Multi-point contact manifolds + sleeping.** The solver resolves one contact
   point and never sleeps. Needed for stable stacking and many physics props.
3. **Finish the ECS migration.** Move the remaining legacy `Entity` gameplay
   (controllers, sensors, combat, health) onto the registry so there is one
   authoring model; the benchmark quantifies the payoff.
4. **Data-driven scenes.** Level/prefab loading + serialization so content is
   authored as data, not C++ (`SceneBuilder` is currently hand-written).
5. **Structural command buffer.** Replace the "add/remove components between
   phases" rule with a deferred command buffer so systems can mutate structure
   safely mid-query.
6. **Animation authoring depth.** Blend trees, root motion, and richer transition
   conditions for the state machine to reach Ori-class fluidity.
7. **Diagnostics.** Frame-time / system-time overlay and a headless perf harness
   wired into CI to catch regressions in the numbers above.

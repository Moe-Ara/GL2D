# THE LOST HEROIN — Pre-Production Package

**Working title:** THE LOST HEROIN *(project codename)*
**Proposed display title:** **LOWTIDE**

> Naming note for the team: the codename spells "Heroin", not "Heroine". If the
> drug connotation is unintended, do not ship the codename. "LOWTIDE" is the
> recommended display title used throughout this package; the internal folder
> name stays as-is.

**One-line pitch:** A girl crosses the bed of a vanished sea toward the one wave
that never fell — and learns the whole walk has been the held breath before it
lands.

**Genre:** Cinematic 2D side-scrolling adventure (no combat, no inventory).
**Length:** ~4 hours. **Players:** 1. **Engine:** GL2D.

---

## Package contents

| Doc | Contents |
|---|---|
| [01-Vision.md](01-Vision.md) | Concept, premise, mystery, themes, audience, reason to exist |
| [02-StoryBible.md](02-StoryBible.md) | Full narrative, world history, hidden truths, symbolism, reveals |
| [03-BeatBible.md](03-BeatBible.md) | The 4 hours, beat by beat, with pacing/emotion/mystery tracks |
| [04-EnvironmentBible.md](04-EnvironmentBible.md) | Every location as a designed space |
| [05-Cinematography.md](05-Cinematography.md) | Camera philosophy and shot design |
| [06-PuzzleBible.md](06-PuzzleBible.md) | Environmental puzzle languages and every major puzzle |
| [07-GameplayProgression.md](07-GameplayProgression.md) | Mechanic introduction/retirement, difficulty/tension/curiosity curves |
| [08-ArtDirection.md](08-ArtDirection.md) | Visual language: silhouette, color, light, motif |
| [09-AudioDirection.md](09-AudioDirection.md) | Per-chapter soundscapes, music policy, silence policy |
| [10-GreyboxGuide.md](10-GreyboxGuide.md) | Every area as playable grey geometry |
| [11-AssetRequirements.md](11-AssetRequirements.md) | Categories of assets eventually required |
| [12-PolishLibrary.md](12-PolishLibrary.md) | 150+ small polish opportunities, by chapter |
| [13-GL2DFeatureRequirements.md](13-GL2DFeatureRequirements.md) | Engine capabilities per chapter: have / partial / build |
| [TODO.md](TODO.md) | What is implemented today, and the ordered build plan for the rest |

## The five emotional states (the engine-facing spine)

The game's protagonist affects the world through emotional state, realized with
GL2D's FeelingsSystem. Every chapter, camera choice, palette, and puzzle sits on
one of five authored states. These are data (`assets/config/feelings.json` in
the demo) — not code.

| State | World response | Grade | Camera | Player feel |
|---|---|---|---|---|
| **Still** | Ghost Tide at rest; baseline | neutral, slightly cool | medium, steady | normal |
| **Wonder** | bioluminescence wakes, tide lifts gently | warm highlights, bloom up | wider, slower follow | slightly floaty |
| **Fear** | tide recedes, lights dim, world dries and sharpens | desaturated, vignette in | tight, fast follow, micro-shake | faster accel, harsher stops |
| **Grief** | Ghost Tide rises, sound muffles, weight | blue-grey, crushed highlights | low offset, heavy damping | slowed, heavier |
| **Resolve** | tide obeys, light strengthens | full contrast, clean | stable, confident | normal+, surefooted |

## Chapter map

| # | Chapter | Length | Dominant state | Palette anchor |
|---|---|---|---|---|
| 0 | The Held Breath (prologue) | 10 min | Still | grey dawn, one warm window |
| 1 | Brinehollow — the Village on Stilts | 30 min | Still → Wonder | bleached wood, brass |
| 2 | The Salt Meadows | 30 min | Wonder | white, mirror-sky blue |
| 3 | The Coral Forest | 35 min | Wonder → Still | bone grey → biolume teal/violet |
| 4 | The Trench | 40 min | Fear | black, single warm lure-light |
| 5 | The Silent Fleet — the liner *Aurelia* | 30 min | Grief | rust, green glass, lamplight |
| 6 | The Standing Wave — Lanternquay | 35 min | Grief → Resolve | storm slate, frozen silver |
| 7 | Lowtide (finale + epilogue) | 15 min | Resolve → Still | drowned gold, dawn |

Total ≈ 3 h 45 m of play + credits scene.

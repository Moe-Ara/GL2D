# 08 — Art Direction Guide

## The one-sentence look

**A hand-inked storybook drowned and dried again:** soft painterly depth bands,
hard readable silhouettes on the play plane, and light that behaves like a
character.

## Silhouette doctrine

- The play plane (MG) is the **contrast plane**: anything Mara can touch,
  climb, or release must read as near-black or near-light shape against its
  band. Interactivity is communicated by silhouette clarity, never outline
  glow or button prompts.
- Mara's silhouette: small round head, heavy coat with a broken triangular
  hem (reads at 16 px), boots slightly oversized (a child in someone's boots —
  the keeper's). Her Call posture (head lifted, chest open) must be readable
  at the widest zoom, because distant players still need to see her sing.
- Each chapter owns one **signature silhouette** (stilt-legs / mast-diagonals
  / antler-coral / wreck-ribs / chandelier-star / the wave-curl / the
  lighthouse) that appears in its poster shot and nowhere else in the game.

## Color progression (the game in seven palettes)

| Ch | Ground | Light | Accent | Forbidden |
|---|---|---|---|---|
| P | ash grey-brown | cold white | one warm window | saturation > 20% |
| 1 | bleached driftwood | overcast pearl | tarnished brass | green |
| 2 | white-salt, pale rose shadow | high-key white | mirror storm-blue | warm ground tones |
| 3 | bone grey-pink → deep sea dusk | biolume teal/violet | lure-fish ember | daylight blue |
| 4 | true black, cold silt brown | jelly warm-amber | anemone cyan | any full-frame light |
| 5 | rust, oxblood, green glass | dusty grey shafts | one warm cabin gold | biolume colors |
| 6 | storm slate, glass green | wave-filtered jade | release-white, ghost gold | warm sky |
| F | drowned gold, deep blue | lamplight god-rays | dawn rose | grey |

Rules: adjacent chapters never share an accent; each chapter's accent becomes
a *memory color* that reappears exactly once later (brass → the lamp's crank;
biolume teal → the rising dead; jelly amber → the lamp's flame). The game's
full arc in grade: **desaturated → chromatic → black → rusted → jade → gold.**

## Light

- Light sources are earned: every lamp in the world is dead until Mara's
  touch/Call wakes it. Waking a light is always staged (bloom-up over 0.5 s,
  never instant).
- HDR discipline: only three things may bloom — living biolume, the Ghost
  Tide's surface glitter, and flame. Sky never blooms.
- Shadows are soft everywhere except Ch6, where the wave-filtered light casts
  the game's only hard-edged shadows (frozen time = hard light).
- Normal-mapped sprite lighting on the play plane only; parallax bands are
  flat-lit paintings (depth via value, not shading).

## Fog & atmosphere

Per-band fog planes (not full-screen): mist ribbons at mud level (Ch1), heat
shimmer distortion band (Ch2), spore haze (Ch3), silt gradients that eat the
BG entirely (Ch4), dust shafts (Ch5), pressure-green depth fog (Ch6).
Fog color is always the palette's light color, never grey by default.

## Particles (the living air)

Every chapter has exactly two ambient particle identities (more reads busy):
P: dust motes / none. 1: salt-mist flecks / rope fibers. 2: wind-borne salt
grains / pool ripple rings. 3: drifting spores / polyp sparks. 4: falling
silt / jelly plankton glow. 5: rust flakes / dust in shafts. 6: static
rain-beads (frozen; particles that do NOT move — negative-space particles) /
release-bursts. F: rising light-motes / bubbles.

## Animation style

- Frame counts low and weighty (8–12 frame walks), easing hand-authored;
  follow-through on the coat hem and hood is where the life lives.
- Mara has **contextual idles** per feeling-state: Still (hands in sleeves),
  Wonder (head tracking points of light), Fear (hood up, arms close), Grief
  (shoulders down, slower blink), Resolve (weight forward). State-driven idle
  swap is the cheapest characterization in the game.
- The world animates on touch: grass-kelp parts, boards flex, dust puffs —
  reaction animation > ambient animation in the budget.

## Weather as staging

Weather never changes randomly; it is authored per scene and always *means*:
wind = the world urging her on (Ch2), stillness = held breath (P, 6), frozen
rain = the truth (6), real falling rain = release (F only).

## Visual motifs (recurring, never explained)

1. **Lines going down** — ladders, anchor chains, light shafts, rain: the
   whole game's composition points toward the seabed until Ch6 reverses it
   (lighthouse, rising weights, rising dead: lines going up).
2. **Circles of light in the dark** — jelly halo, bell-tide rings, the lamp's
   sweep: safety is round.
3. **Threes** — three meals, three figures in the drawing, three bells, three
   weights: the family, encoded everywhere.
4. **The low hook** — introduced in the prologue (child-height coat hook),
   paid off in the final shot. The player's subconscious knows this house.

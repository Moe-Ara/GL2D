# 01 — Vision Document

## Core concept

LOWTIDE is a side-scrolling cinematic adventure about walking the floor of a
vanished sea. The player is Mara, a lighthouse keeper's daughter, maybe ten
years old, who wakes to find the ocean gone — boats stranded in mud, kelp
hanging in the air like winter trees, and on the far horizon a single wave that
never fell, standing like a mountain of glass.

She walks toward it. That is the whole game.

What makes the walk a game is the **Ghost Tide**: the sea is gone, but its
*memory* remains, and that memory answers to Mara's emotional state. Where she
feels wonder, dead coral lights up and remembered water gathers enough to carry
her. Where she is afraid, even the memory drains away and the world turns
brittle and sharp. The engine's feelings system is not a backend detail — it is
the protagonist's inner life made into level design. The player never sees a
meter. They see the world change.

## High-level premise

The night of the Great Storm, the light of Lanternquay went out. The fishing
fleet and the liner *Aurelia* were lost in the dark, and a wave came for the
town. Mara wakes some unmeasured time later on the dry seabed, far from home,
with no memory of the night. Between her and the Standing Wave lie the places
the sea used to own: a stilt village, salt plains, a coral forest, a black
trench, and the wrecks of the ships that never came home. Every one of them
remembers the storm. None of them will say it aloud.

## Central mystery

Three questions, opened early and answered in strict order, each larger than
the last:

1. **Where did the sea go?** *(opened in the first minute; answered at the
   midpoint)* — It didn't go anywhere. It is standing at the horizon, mid-fall.
2. **Where did all the people go?** *(opened in Chapter 1; answered aboard the
   Aurelia)* — They were on the water the night the light failed, or under the
   wave the moment it broke.
3. **Why does the sea's memory answer this one girl?** *(opened whenever the
   player first notices; answered at the top of the lighthouse)* — Because the
   whole world of the game is the held instant of the wave's fall, and Mara is
   the one who was supposed to be keeping the light. The walk across the seabed
   is her crossing of that single frozen second. The sea is waiting for her to
   let it finish.

The game never states any of this in dialogue. There is no dialogue.

## Themes

- **The moment you couldn't act.** Guilt fixed on a single instant — and the
  impossibility of living inside that instant forever.
- **Grief as weather.** Emotion rendered as tide, fog, light, and pressure
  rather than words.
- **Keeping the light.** Tending something for others even after failing once;
  duty as a form of love that survives loss.
- **The sea as memory.** Water that remembers shapes, routes, songs, and the
  drowned; a landscape that is itself an archive.

## Emotional goals

- The first ten minutes should feel like *held breath* — hushed, grey, wrong.
- At least once per chapter the player should stop walking just to look.
- The Trench should produce dread without a single enemy sprite in close-up.
- The Aurelia should make the player feel like an intruder in someone's grief.
- The relighting of the lamp should feel earned physically (a hard climb) and
  emotionally (the player has, by then, assembled the truth themselves).
- The ending should land as release, not tragedy: the wave falling is the world
  finally allowed to grieve, and the after-image is warm.

## Target audience

Players of Limbo, Inside, Gris, Journey, Planet of Lana, Little Nightmares:
people who buy 3–5 hour authored experiences, finish them in one or two
sittings, and talk about single images afterwards. Age 12+. Playable by
non-gamers: two axes, one jump, one contextual verb, one Call.

## Why this game should exist

- **It is the game GL2D was built for.** The feelings system, parallax
  pipeline, HDR lighting, particles, and camera game-feel exist and have no
  flagship. LOWTIDE gives every subsystem a reason and drives the engine's
  roadmap with concrete needs (see doc 13).
- **The hook is genuinely new.** Emotion-as-tide is not the mechanic of any of
  the inspirations; "the entire game takes place inside one frozen second" is a
  reveal none of them attempt, and it recontextualizes every environment
  retroactively — strong replay and word-of-mouth value.
- **It is solo-shippable.** One character with a readable silhouette, no
  combat AI, no systemic simulation, environments built from layered stills and
  particles — scope that one developer plus contracted art/music can carry.

## Pillars (tie-breakers for every future decision)

1. **The world answers her.** If a feature doesn't visibly respond to Mara's
   state, cut it or move that budget to something that does.
2. **Mystery over exposition.** When in doubt, remove the explanation.
3. **Forward is the reward.** No collectibles, no backtracking for progress;
   curiosity is the only currency.
4. **Every screen is a poster.** If a screenshot of a scene wouldn't be worth
   posting, the scene isn't done.
5. **Silence is content.** Music is rare; quiet is designed, not empty.

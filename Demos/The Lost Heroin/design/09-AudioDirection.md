# 09 — Audio Direction

## Philosophy

The mix is built around **absence**: a seaside world with no sea in it. Every
chapter's soundscape is defined first by what is *missing*, then by what
remains. Music is an event, not a bed — the score appears five times in the
whole game. Silence is authored with room tone, never digital zero.

The FeelingsSystem drives the audio the way it drives color: each state maps
to a mix preset (low-pass, reverb send, ambient gain) so emotion moves the
whole soundworld at once. Grief muffles (LPF ~2.4 kHz, long reverb), Fear
dries and sharpens (short pre-delay, +footstep gain, audible breath), Wonder
opens (full-band, chorus-wide ambience), Still is the neutral reference,
Resolve adds low warmth (+80–160 Hz shelf).

## Per-chapter soundscapes

| Ch | Bed (always) | Punctuation (sparse, spatial) | Missing on purpose | Reverb |
|---|---|---|---|---|
| P | interior room tone; wood creak | clock *not* ticking (felt); her steps | any exterior sea/wind until the door | dry, small |
| 1 | wind through pilings (bottle-tone); rope creak | shutter knock; single bell tolls; tide-rise wash | voices, gulls, water | open air, short |
| 2 | wind with grain-hiss; heat-shimmer whine (barely) | pool ripples; *underwater thunder* near mirrors; sail-cloth luff | footstep echo (salt eats it) | none (anechoic dread of open space) |
| 3 | coral ticks/clicks; canopy wind above only | polyp chimes (pentatonic, chain-melodies); lure-fish flutter; the Warden's one horn note | wind at ground level | nave-like, long tail, quiet |
| 4 | her breath; silt hiss; deep metal groans | anemone shimmer; bell + hull-lift; **the Undertow = a hole in the mix** (ambience ducks −12 dB around its position + sub-bass swell) | everything else — darkest mix of the game | tight slap, close walls |
| 5 | hull whale-tones; papers settling; rust-fall patter | chandelier chimes on tilt; piano string when it slides; the warm cabin = sudden dry intimacy | wind inside; music where you'd expect it (ballroom is silent) | long steel corridors vs dry carpet cabins |
| 6 | pressure LFE bed; near-silence | steps echoing off the wave (long pre-delay slap); Time-Stub releases = 1 s of full storm audio, hard-gated; floor-by-floor thaw: wind → rain → clockwork → "Mara" | continuous weather (all frozen) | wave-slap: the signature reverb of the game |
| F | underwater muffle + lamp hum; then ordinary morning sea | soft bells swaying in current; whale song (the Warden, finally in water); gulls at the surface | tension of any kind | warm, enveloping → open dawn air |

## Wildlife policy

No living wildlife sounds until: lure-fish flutters (Ch3, sea-life in air),
whale song (F), gulls (F, surface only). The first ordinary gull cry of the
finale should land like a sunrise — it has been withheld for four hours.

## Music (five cues total)

1. **The Cistern (2.4)** — solo cello over the wind, 90 s. First music of the
   game; establishes the score's instrument = a voice-like cello (rhymes with
   the Call).
2. **The Warden (3.3)** — one sustained low horn note as the eye passes; not
   melody, presence.
3. **The Cabin (5.3)** — the cello again, muted, 40 s, stops mid-phrase when
   she touches the drawing (the score itself is interrupted, like the lives).
4. **The Lamp (6.4)** — the only full piece: cello + the Call's hum-tone +
  the pentatonic polyp-scale resolving into one theme as the ghost-lights
  rise; crescendos into the ignition white-out.
5. **Credits/Below (F)** — the theme restated at half tempo underwater, then
   dissolving into ordinary morning ambience for the final door.

The Call itself is quasi-musical: Mara hums a fixed minor-third motif; all
bells, polyps and the finale theme are tuned around it, so the entire world is
in the key of her voice.

## Environmental sound cues (gameplay-critical)

- Ghost Tide rise/fall: white-noise wash pitched by height; the decay *is* the
  puzzle timer (players learn to hear remaining time).
- Jelly brightness: amber hum whose level maps to radius — light is audible in
  the dark chapter.
- Undertow proximity: mix-hole radius + sub swell; never a "monster roar".
- Time-Stub target in range: a faint single-tine ring (the frozen object
  "waiting"); release: storm-second then re-freeze cut to pressure bed.
- Touched objects chord: every reach-touch gives one soft chord in the world
  key — touch is the game's dialogue, and it is scored.

## Silence placements (authored)

Door-opening (P1, 3 s of nothing before the wind), post-bell decay (1.4),
the eye of the Warden, the second after the cabin light goes out, the kneel
(total silence, first digital-adjacent zero of the game, 2 s), and the moment
between lamp ignition and the wave's first movement.

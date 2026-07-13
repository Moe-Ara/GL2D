# 05 — Cinematography Guide

## Philosophy

The camera is a **witness with taste**, not a leash. It follows Mara the way a
patient documentary operator would: slightly late, slightly ahead of her
intent, and always willing to abandon her scale in favor of the world's. The
player should never *think about* the camera, but every held frame should be
composed.

Three laws:

1. **The camera never cuts while the player has control** (exception: the
   Undertow's checkpoint smash-cut, whose violence is the point). Movement,
   zoom, and framing changes are continuous.
2. **Scale is the storyteller.** Emotional beats are delivered by zoom and
   headroom before anything else: intimacy = tight and low; awe = wide and
   high-headroom; dread = tight and *high* (ceiling pressure).
3. **One sacred frame per chapter.** Each chapter contains exactly one locked,
   composed, promotional-grade shot the player walks through (the "poster"
   shots in doc 03). Scarcity keeps them potent.

## Camera states (maps to GL2D camera + feelings)

| State | Zoom | Damping | Look-ahead | Dead zone | Notes |
|---|---|---|---|---|---|
| Intimate (interiors, prologue) | 1.25–1.4× | slow (4) | none | small | low framing, Mara ~1/3 height |
| Walk (default) | 1.0× | medium (6) | 0.12× vel | medium | the game's "prose" |
| Vista (meadows, bow) | 0.6–0.8× | very slow (3) | 0.2× vel | wide | engage on volume triggers |
| Dread (trench) | 1.3–1.4× | high (8) | reversed ↓ | tight | micro-shake by Undertow proximity |
| Ceremony (sacred frames) | fixed | locked | none | none | player walks within a locked frame |

All states are **camera-feeling presets** blended by the FeelingsSystem
(zoomMul, followSpeedMul, offset, shake) — the same data channel as color and
audio, so mood changes move *everything* in sync.

## Zoom behavior

- Zoom is always motivated: discovery zooms out (world grows), danger zooms in
  (walls close), touch zooms in briefly (attention).
- Speed limit: no more than 0.15×/s except the two authored smash moments
  (trench daylight cut; lamp ignition).
- The Call breathes the camera: +0.08× swell over the hum, settle on release.
  The player's own voice literally moves the lens — subliminal ownership.

## Transitions between areas

- **Continuous by default:** areas connect by walkable seams; parallax sets
  swap during occlusion moments (doorways, crawl-spaces, FG wipes by a
  foreground pillar crossing the frame).
- **The FG wipe** is the signature transition: a near-layer object (piling,
  coral fan, wreck rib) sweeps the frame as Mara passes; when it clears, the
  new chapter's palette is live and its feeling-state has blended in.
- Hard cuts exist only as: Undertow death → checkpoint; lamp ignition → white;
  final door → black.

## Reveal shots (the money moments)

Staging grammar for all reveals: **occlude → release → hold → return.**

1. *The missing sea* (P1): door opens as FG wipe; camera pulls from 1.3× to
   0.8× over 4 s; hold 4 s; control never taken.
2. *First Ghost Tide* (1.2): camera holds low as translucent water rises past
   the lens's bottom edge — the tide enters the *frame* before the basin.
3. *The Warden* (3.3): camera locks; the whale's body enters from below-frame
   and exits above-frame — never fully in frame at once; scale by denial.
4. *The Standing Wave* (5.4): the game's longest zoom-out (12 s, 1.0×→0.55×)
   as Mara walks the bowsprit; wave parallax factor so low it barely moves —
   planetary stillness while the ship slides past.
5. *The shadow under the table* (6.4): the reverse — the game's slowest zoom
   *in*, ending in the locked sacred frame before the kneel.

## Tracking shots

- Boardwalk and mole use rail-like lateral tracks with fixed Y (composition
  beats fidelity: Mara may drift within the frame vertically).
- The Procession Road tracks *ahead* of Mara by up to 30% frame width —
  the camera is more curious than she is; dread by anticipation.
- The chain-climb chase reverses look-ahead downward toward the pursuer for
  the first half (fear looks back), then flips upward at the daylight break
  (hope looks up) — one parameter, whole arc.

## Static shots

Used for: the cistern rest, cabin doorway, the kneel, the post-credits cup.
Statics are earned by rarity; each is letterboxed except the cistern (rest
should feel unframed, not composed).

## Slow cinematic moments (player retains control)

- Forced slow-walk zones (bowsprit, wave-shadow entry, gallery finale) cap
  speed via the feelings channel (entitySpeedMul), never by input lockout.
- During these, FG layers get +10% parallax factor — the world sweeps past
  slightly faster than life, the "dolly feel".

## Foreground occlusion

- Every chapter carries a dedicated FG occluder set (pilings, salt streamers,
  fan corals, wreck ribs, mullions, bead-rain). Rules: occluders may cover
  Mara ≤ 40% of any 5-second window; never during puzzle manipulation; always
  during transitions (that's the wipe).
- Occluders are desaturated 20% and darkened toward the palette's shadow
  anchor so they read as *frame*, not content.

## Framing & composition rules

- Rule of thirds for walking; dead-center only for ceremony (the kneel, the
  lamp).
- Horizon never at frame-middle: low (⅓) in wonder chapters (sky owns the
  world), high (⅔) in dread chapters (ceiling owns it).
- Mara faces open frame-space except when the design wants resistance
  (walking left-to-right is the game's "forward"; the *only* sustained
  right-to-left movement is the lighthouse spiral — going home runs against
  the grain).
- The Standing Wave always appears on the same world-side (right/east) until
  Ch6, when it is overhead — the geography must never confuse the compass.

## Letterboxing

Engages: under the wave (Ch6), sacred frames, the finale. 2.35:1 by bars that
slide, never pop. It is the visual signal for "the world is speaking now".

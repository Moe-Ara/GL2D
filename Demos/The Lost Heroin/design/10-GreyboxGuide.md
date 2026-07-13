# 10 — Greybox Guide

Every area speced as grey geometry. Unit: 100 px = 1 m (PhysicsUnits). Mara:
capsule ~0.8 × 2.0 m; run 3 m/s; jump apex ~1.3 m; long-drop safe to 6 m.
"Screen" = 19.2 m wide at default zoom.

Conventions: `CP` checkpoint · `CT` camera trigger (state/zoom/rail) · `SE`
scripted event · `FZ` feeling zone (state blend volume). The greybox must be
**fun silent and grey**: spacing, drops, and reveals are the product; art is
paint.

Greybox materials: play plane #444, climbable #666 with 45° hatch, Ghost-Tide
volumes translucent #4aa at 30%, frozen objects #99b, releasables #bb6,
triggers wireframe.

---

## P — The Held Breath (world length ~260 m)

- **P1 room + porch (30 m, flat):** door push teach; `CT` intimate 1.3×;
  `SE` door-open reveal (zoom to 0.8× over 4 s); `FZ` STILL from boot.
- **P2 ladder shaft (18 m down):** 3 ladder flights broken by 2 platforms;
  one 4-m teach-drop with landing kick; `CP` at seabed touch.
- **P3 flats (210 m, flat with 3 gentle dips):** run unlock at 60 m (`SE`
  wind push gust); footprint decals converge from 5 lines to 1; `CT` walk
  state + look-ahead 0.12; village silhouette fades in at 150 m.
- Pacing check: first input→first reveal < 90 s; nothing hostile for the
  entire prologue.

## 1 — Brinehollow (world length ~700 m; play band ±14 m vertical)

- **1.1 porch lanes (180 m, +8 m elevated):** boardwalk gaps 1.5–2.5 m
  (jumpable), one collapsed span forcing a 8-m descent + re-climb; 4
  reach-touch props (`SE` chord each); `CP` ×2.
- **1.2 harbor basin (90 m Ø bowl, −6 m):** bell gantry at rim; Ghost-Tide
  volume fills to −1 m line for 22 s on ring; skiff = floating platform
  (buoyancy); fail = tide-out, walk back 10 s. `CT` wide on basin entry;
  `SE` first-tide cinematic; `FZ` WONDER during tide. `CP` before + after.
- **1.3 mole (150 m, flat, +2 m):** one crate push (2 m); name-board prop at
  110 m; `CT` static-wide poster shot spanning 90–140 m (rail-locked Y).
- **1.4 terraces (3 basins: 40/50/60 m Ø, steps of +5 m):** bells B1(22 s)
  B2(18 s) B3(cracked — counterweight swing arc 4 m); solution path:
  B1-ride → B2-ring from skiff → weight-swing → B3-ride to exit. `CP` per
  terrace. Exit `SE`: all-tides-at-once hold 10 s, then drain (`FZ` WONDER
  → STILL).

## 2 — Salt Meadows (world length ~950 m; near-flat, dune band ±5 m)

- **2.1 white walk (260 m):** wind force +0.4 m/s tailwind zones (visible
  streamers); `CT` vista 0.65×; frozen gull decal high in BG at 190 m
  (never pointed at). `CP` at wreck stake 240 m.
- **2.2 mirror pools (240 m dune maze, 3 branches):** 5 pools; reflected
  mast-shadow decals rotate to point next branch; ripple = shadow blind 6 s;
  wrong branch loops back in 25 s. `CT` locked-Y for reflection composition.
  `SE` reflected-lightning flash once. `CP` at maze entry + exit.
- **2.3 fleet on the salt (280 m):** 3 hull climbs (6–9 m), yard-walks with
  2-m gaps, sail-slide 12 m; capsize puzzle: boom re-hang (push 3 m + reach)
  → sail force rolls hull 90° over 6 s (`SE`). `FZ` GRIEF-tint 30-s zone at
  graveyard center. `CP` ×2.
- **2.4 cistern (25 m interior):** sit-verb shrine; music cue #1; `CP`
  (session-break point). Exit `CT` back to walk state.

## 3 — Coral Forest (world length ~650 m; vertical band +25 m)

- **3.1 bone orchard (200 m, climbs to +12 m):** 3 climb-trees (branch
  hitboxes: climbable = thin, dead-end = bulb), 2 kelp-ropes (swing period
  1.8 s), one 5-m drop-through canopy hole; `CP` ×2. First polyp at 185 m
  (`SE` glow on Call, auto-zoom insert).
- **3.2 lit path (250 m, rolling +8/−10 m):** chain nodes every 6–8 m;
  chain speed 4 m/s, decay 8 s; gate G1 (single chain), then G2 requiring
  lure-fish routing: light order defines fish path (3 nodes), fish enters
  hollow, hollow lights = platform visible inside. Failure: decay → re-Call
  at last node. `CP` before each gate. `SE` full-cascade cinematic at G1.
- **3.3 Warden cliff (60 m, slow-zone):** forced walk 1.2 m/s; `CT` locked
  frame; `SE` Warden rise (12 s, screen-height silhouette on BG band 0.3
  moving bottom→top). No mechanics.
- **3.4 descent + procession road (140 m, −20 m switchbacks):** belongings
  props density ramps 1-per-10 m → 1-per-2 m; Call light-radius visibly
  shrinks via `FZ` (STILL → FEAR blend across final 60 m); `CT` tighten to
  1.2×; `SE` turn-back moment: wave-glint decal noticeably larger. `CP` at
  canyon mouth (session break).

## 4 — The Trench (world length ~800 m; ceiling closes from 20 m → 4 m)

- **4.1 jelly colony (120 m):** pickup at 60 m (`SE`); light radius: bright
  5 m walk / 2.5 m run; teach-corridor with radius-gates (stalactite slaloms
  spaced 4 m). `CP` at pickup.
- **4.2 undertow galleries (280 m, 3 setpieces):**
  S1 patrol crossing (period 9 s, corridor 30 m): wait-and-walk.
  S2 skylight room (`SE` darkening pass overhead, no threat — pure staging).
  S3 anemone relay: gallery 46 m, dark gap 18 m, stands at 0/12/24/36 m;
  calm-walk bridge exactly covers gaps; run = dim = fail-soft (grab-back
  event, re-light at stand). First contact = light-steal (`SE`), second =
  smash-cut `CP` reset (checkpoints every ≤ 25 s of progress).
- **4.3 fleet floor (240 m):** 4 wrecks; bell-lift W2 (hull +2.5 m for 12 s);
  decoy bell at 30 m offset; Undertow re-route time 8 s = passage window;
  father's name-board on W3 (reach + jelly-raise to read — optional). `FZ`
  GRIEF pockets 8-m radius per wreck inside global FEAR. `CP` ×3.
- **4.4 chain stair (60 m vertical):** climb 3 chain segments; `SE` surge
  chase at segment 2 (climb-input QTE-free: just climb; speed check 2.2 m/s
  vs surge 2.0 m/s); daylight break at top (`SE` hard lighting cut, FEAR →
  GRIEF blend); jelly release pool on the anchor ledge (`SE`). `CP` top
  (session break).

## 5 — The Aurelia (world length ~600 m incl. verticals; list states L-port
15° / L-star 12°)

- **5.1 hull garden (80 m exterior climb, +18 m):** rivet-seam ladder holds
  every 2 m, 2 porthole rests; rust-flake particles on grab. `CT` silhouette
  wide (BG = hull only). `CP` at window entry.
- **5.2 ballroom (70 × 14 m, two list states):** flood-station port (Call,
  8 s fill) → tilt swap 5 s (`SE` slide choreography: 12 furniture bodies,
  chandelier swing 20°); furniture stack = stair to +8 m mezzanine. `CP`
  before/after tilt.
- **5.3 cabins (120 m corridor at list):** uphill walk vs downhill slide
  rhythm; 1 crawl (broken bulkhead); warm-cabin diorama (`SE` touch → light
  release, music cue #3, `CP`).
- **5.4 bow (90 m open deck, slow-zone last 30 m):** `CT` 12-s zoom-out rail
  1.0×→0.55×; `SE` wave reveal hold; mast-bridge exit (25 m beam at 20°,
  walk-only). `CP` at mast base (session break).

## 6 — Lanternquay (world length ~700 m + lighthouse 30 m vertical)

- **6.1 outskirts (180 m):** frozen props (push-fail thunk teach ×2);
  bead-rain curtains (walk-through FG); Time-Stub teach: cart release
  (roll 6 m → step). Letterbox `CT` on entry; `FZ` GRIEF. `CP` ×2.
- **6.2 street of bells (240 m, +10 m to gate):** releasables: awning
  (debris fill, 2 s), mast (fall spans 8-m gap OR blocks stair — depends on
  awning state), town bell (swing knocks shutter platform). Reset shrine at
  mid-street (re-freezes all three, 10-s walk-back cost). `SE` true-toll on
  bell release (town-wide flicker). `CP` before each releasable.
- **6.3 lighthouse climb (30 m vertical, 4 landings):** crank-hold stations
  (3 × 4 s hold) each +1 storey of ghost-lamps and one audio-thaw layer;
  weight-3 puzzle: exterior beam release through window (timing: beam falls
  3 s, weight platform catch window 1 s — retry loops beam re-freeze). `CP`
  per landing.
- **6.4 lamp room (12 m):** kneel-verb at table (locked static `CT`); lamp
  sequence: wick (reach), crank (hold), Call (hold 6 s, `SE` ghost-lights
  rise town-wide, music cue #4) → white-out. `FZ` GRIEF → RESOLVE at kneel.

## F — Lowtide (world length ~400 m, mostly non-failable)

- **F1 gallery walk (40 m, slow-zone):** wave-fall `SE` in BG bands (6 s,
  slow-motion timeScale 0.4 via feeling); flood-to-white.
- **F2 below (200 m gentle ascent):** swim-up input; rising light spawners;
  Warden pass `SE`; zero fail states; `FZ` RESOLVE → STILL.
- **F3 surface + gallery + door (160 m):** dawn walk; coat-hook `SE`; cut to
  black. Credits scene: 60-s auto-drift (no input) down lamplit water.

## Global greybox budgets

- World total ≈ 5.1 km walkable + 3 interiors. At 1.4 m/s average travel with
  puzzle/scene dwell, ≈ 3 h 45 m.
- Checkpoint spacing: never > 90 s of loss; Trench: never > 25 s.
- Camera triggers: 41 total (listed above); feeling zones: 19; scripted
  events: 33.
- Greybox milestone test: full run-through in flat grey with placeholder
  audio ticks must already produce the tension curve of doc 07 in playtest.

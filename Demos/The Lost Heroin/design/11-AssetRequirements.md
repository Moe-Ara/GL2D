# 11 — Asset Requirements

Implementation-agnostic categories. Counts are targets for a solo-plus-
contractor pipeline; every category lists reuse leverage. No pack names, no
purchased-asset assumptions.

## Character

- Mara sprite set: walk/run/jump/land/climb/hang/push/reach/kneel/Call/swim +
  5 feeling-state idles. (~20 clips, 8–12 frames each; the single largest
  animation investment — everything else animates far less.)
- The child-shadow (Ch6): Mara's silhouette re-timed, 3 poses. Near-free.
- The Warden: one enormous multi-part silhouette (body/fin/eye layers) with
  slow sine articulation — rigged sprite, not frame animation. 1 build, used
  4 times.
- The Undertow: shader/particle entity (flow-field of dark + debris
  glitter), no character frames at all.

## Environmental structure sets (tiling + landmark pieces)

- Weathered timber kit: stilt pilings, boardwalks, ladders, shutters, roofs
  (P + Ch1; re-dressed for Lanternquay streets in Ch6 — same kit, storm
  palette).
- Stone kit: mole blocks, cistern, churchyard walls, lighthouse exterior
  ring (Ch1/2/6).
- Salt terrain bands: flats, dune crests, crystalline crust edges (Ch2).
- Coral kit: trunk/branch modules (climbable + bulb variants), fan-coral FG
  blades, polyp clusters (unlit/lit), gate anemones (Ch3).
- Trench rock kit: overhangs, columns, silt beds, anemone stands (Ch4).
- Shipwright kit: small-hull parts (fishing fleet — Ch2 salt + Ch4 floor,
  one kit two palettes), liner parts (hull plates, rivet seams, railings,
  ballroom architecture, corridors, cabins) (Ch5).
- Lighthouse interior: spiral stair, clockwork (weights, gears, crank),
  lamp assembly (Ch6/F). The clockwork is a hero asset — build once, love it.

## Hero props (one-off, story-bearing)

Stopped clock · cold cup (+ steaming variant) · low coat hook · child boots ·
tide-bells (3 sizes + cracked) · skiff · trawler keel · boom/sail rig ·
jelly-lantern · anemone stand · wreck name-boards (stencil system) · luggage
+ LANTERNQUAY tags · route chalkboard · chandelier · piano · child's drawing ·
keychain · practice crank · town bell · leaning mast · cart · shrine niches ·
the lamp.

## Effects & overlays

- Ghost Tide: translucent water volume with surface glitter + rise/fall
  meshes; buoyancy ripple rings.
- Water memory FX: bead-rain curtains (static particles), release-bursts,
  underwater god-rays, bubbles, rising light-motes.
- Weather: salt-dust streamers, wind gusts, heat shimmer distortion band,
  silt-fall, rust-flake fall, spore drift, marine snow (up + down variants).
- Fog planes per palette; vignette/grade LUT ramps per feeling state.
- Biolume: polyp glow states, chain-pulse traveling light, lure-fish comet
  trail.
- The frozen-world set: hard-shadow decals, frozen spray sheets, glass-smoke
  columns.

## Audio (categories)

- Foley: footsteps (wood/mud/salt/stone/steel/parquet/underwater), climbs,
  pushes, cloth.
- World beds per chapter (7) + missing-element variants.
- Bells (3 pitches + cracked + town + underwater sway), polyp chimes
  (pentatonic set), clockwork loop (healthy/slowing), hull groans, chandelier
  chimes, storm-second stingers (gated release audio), the name call
  ("Mara", one recording).
- The Call: hum motif set (5 intensities); score: 5 cello-led cues.

## UI (minimal by mandate)

Title card, pause overlay (already engine-supported), save-icon glyph, one
contextual input glyph style, credits. No HUD elements exist.

## Reuse ledger (why this fits solo scope)

- Timber kit ships 3 chapters; ship kit ships 2; fleet hulls ship 2.
- The Warden, Undertow, and all water are procedural/rigged — zero frame
  animation outside Mara.
- Palette swaps are feeling-state grades, not re-paints (engine does the
  work).
- Total unique landmark scenes needing bespoke paint: 8 (the posters).

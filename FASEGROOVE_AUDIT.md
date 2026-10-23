# Fase Groove Guitar — technical and musical audit (v0.9.0)

Scope: `Source/engine` (pure C++ groove engine), `Source/PluginProcessor.*` (audio thread, state,
LINK/KEY/LEAD), the editor's timer, and the test suites. Build: JUCE 8, Release, Ninja.

## 1. Confirmed problems and fixes

| # | Problem (confirmed) | Fix |
|---|---|---|
| 1 | **Heap allocation on the audio thread every block.** `raw (const juce::String&)` built a `juce::String` from every `const char*` ID, and the numbered IDs (`level(l)`, `octave(l)`, `mute(l)`, `push(l)`, `laneSwing(l)`, `volume(l)`, `chordRoot/Type(i)`, `keyswitch(a)`, `segment(i)`) were concatenated, i.e. well over 100 alloc/free pairs and map lookups per `processBlock` (in `currentSettings()` and the guide-sound gains). | `raw` now takes `juce::StringRef`, so the APVTS map lookup allocates nothing. The numbered parameters are looked up once in the constructor (`laneParams`, `chordRootParams`, `chordTypeParams`, `keyswitchParams`, `segmentParams`) and read through those pointers. |
| 2 | **Correlated seeds in the generator RNG.** `Random(seed)` started at `seed * step + c` and then advanced by the same step, so seed *n+1* replayed seed *n*'s draws shifted by one. Neighbouring seeds gave near copies (5–8 duplicate grooves out of 40 consecutive seeds in Soulful, Deep and Chill), and the lane dice (`seed*31 + lane`) were correlated across lanes. | The seed is hashed (a 64-bit finaliser) before use. Same seed still gives the same result. Measured: 40/40 distinct grooves per genre, 0 duplicates between seeds *n* and *n+1*. |
| 3 | **Lane dice fallback broke the genre and repeated itself.** When a genre rarely uses a lane (for example a stab in Gospel, Afro, Funky or Jackin'), `makeRandomLane` returned 1 of 3 fixed cells with bar 2 the same as bar 1. Gospel could get 16th stabs. | Bar 2 may take the neighbouring cell and then answers bar 1, and a plain hit of a busy bar may rest. Gospel and Dance are moved onto the 8th grid. Measured: 3 → 15 distinct lines out of 20 throws. |
| 4 | **Unreachable genre material.** Funky & Disco never generated disco octaves (role weight 0, although the preset bank has them). The gospel lick contour existed but Gospel's lick chance was 0. | Funky & Disco weights are now funk-16ths 7 / chank 6 / octaves 3, and Jackin' gets octaves 2. Gospel gets 8th-only answer licks (chance 0.3). |
| 5 | Editor timer (30 Hz) rebuilt the whole displayed groove (`displayedPattern()`, several string allocations) on every tick, but used it only every third tick. | It is now built only on the slow tick. (Message thread, CPU only.) |

## 2. Genres analysed (13, the same list as `grooveCategories()`)

Piano House, Soulful, Deep, Gospel, Jackin', Jackin' Piano, Garage & UK, Afro & Latin,
Funky & Disco, '90s Piano House, Neo Soul & Broken, Chill & Pads, Dance Stabs. There are 41 preset
grooves (3–4 per genre). Disco lives inside *Funky & Disco*. There is no separate Jazz genre: jazz
exists only as the "Neo soul & jazz" progression category.

What was kept, because it is musically valid:
- Funk and Jackin': down-strums on even 16ths, up-strums on odd 16ths, a continuous ghost scratch, a funk-grip 3-string voicing (Nile-style chank) and a 16th chank.
- House piano styles: the stab on the "&". Garage: 2-step skips. Afro: tumbao / partido-alto picking. Neo-soul and Chill: space, laid-back 8ths, licks, shell voicings.
- Gospel: 8th strums with shell voicings. Dance: short 8th stabs with no scratch and no held power chord.
- Accents: the offbeat 8th of the up-strum, chank and scratch is loudest. Ghosts are dead articulations, and a ghost down-strum brushes only the top strings.
- Push chord changes on the last 8th, voice-led shapes, and genre-weighted fills.

Improved (weak or missing rules):
- **Piano House / Jackin' Piano / '90s** used the same generator and were indistinguishable. Now Jackin' Piano adds a 16th push before a beat in about 60 % of bars. '90s replaces the low root with Italo-style octaves on the beats in about 45 % of grooves (some beats rest). Piano House is unchanged.
- **Funky & Disco**: Nile-style 16th strumming is the most likely role, and disco octaves now appear.
- **Dance**: in about 40 % of grooves, octaves on the beats that the stabs leave free (8ths only).
- **Gospel**: 8th-note answer licks, using its own lick contour.
- **Soulful / Deep / Neo-soul / Chill**: in half the grooves, one chord on beat 2, 3 or 4 is anticipated by a 16th, so the space is not always on the quarter.
- **Busy (density > 0.6) scratch** in the house styles: often drawn from scratch cells with holes, instead of always filling all 16 steps (an all-16ths scratch had made 25 % of busy Piano House and Jackin' Piano throws rhythmically identical).

## 3. Dice / random tests (`tests/EngineTests.cpp` → `testDiceDiversity`)

Run per genre × density (0.25 / 0.5 / 0.8) × 40 spread seeds. Humanize is off, so the groove is
measured and not the feel. Everything is measured on the rendered MIDI:
- distinct chord-onset rhythms ≥ 32/40 (measured 35–40); distinct step:pitch sequences ≥ 34/40 (37–40); distinct note-length profiles ≥ 20/40 (23–40); the most frequent rhythm ≤ 3/40 (measured 1–3); ≥ 2 lane mixes per genre (2–18);
- determinism: the same seed renders identical events, and `makeRandomLane`, `makeVariation` and `makeSectionVariation` are repeatable; seeds *n* and *n+1* differ; every MIDI stream is well formed (each note-on closed, time-ordered);
- style identity over the whole population: the house piano styles have ≥ 75 % of stabs on the "&"; Jackin' Piano has a 16th push in ≥ 40 % of grooves; disco octaves appear in Funky & Disco; Gospel has licks; Gospel and Dance are 100 % on 8ths; and the multi-role genres use ≥ 5 lane mixes.
- Set `FGG_DICE_REPORT=1` to print the table.

Limits: these numbers are a floor against repetition and do not prove taste. Taste is guarded by the
existing identity tests (`testGenreIdentity`, `testStyleCoherence`) and the population ratios above.
Pitch variety comes from the lanes and the voicings (the pattern format has no per-hit pitch), so a
one-lane genre (Dance stabs) varies mostly in rhythm.

New plug-in test `testNumberedParameters` checks that every cached numbered parameter reaches the
engine settings.

## 4. Residual issues (not fixed)

- `GrooveEngine::patternFor` rebuilds the pattern (std::string allocations) **on the audio thread** when the pattern, seed, genre, density, section, part order or grid changes. This happens on user or automation changes, not every block, and is short. A clean fix would build the pattern on the message thread and swap it in lock-free, which would change the engine API.
- `pending` is reserved for 1024 hits. Very large offline blocks (more than about 40 steps) may grow it. This only affects offline export.
- Swing acts on 16ths only. Genres written on 8ths (Gospel, Dance) cannot shuffle their 8ths.
- **Compatibility:** projects saved on *Random groove* store only the seed. With the RNG fix and the new genre rules, a seed now plays a different groove than in ≤ 0.9.0. Presets, user patterns and grids are unchanged.
- The grid view repaints at 30 Hz even when nothing has changed (GUI CPU only).

## 5. Notes for Fase Groove Arranger (`ThirdParty/fasegrooveguitar`)

- The vendored engine is already behind this repository's engine. To update it, copy `Source/engine/*` (no API or signature changes: `makeRandomPattern`, `makeRandomLane`, `makeVariation`, `makeSectionVariation`, `GrooveEngine` and `GuitarSettings` are unchanged).
- The Arranger saves `riffSeed` in `ProjectIO`. After the update, guitar tracks on a random groove will regenerate differently (see compatibility above). If old projects must sound the same, store the resolved grid (or keep the old engine for older project versions).
- The engine itself does not allocate per block except in `patternFor`'s rebuild. Hosts that call `render` from a realtime thread should change settings that trigger a rebuild only between blocks, or pre-warm by calling `resolvePattern` off-thread.

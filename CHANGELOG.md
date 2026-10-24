# Changelog

*🇮🇹 [Versione italiana](CHANGELOG.it.md)*

All notable changes to **Fase Groove Guitar**.

## [0.10.2] - 2026-10-24

- Every random groove has an audible chord part (a few were almost silent).
- The grid lights follow the sound again (they lagged since 0.10.1).
- Shared FaseGroove code now comes from the common module; no change to saved projects.

## [0.10.1] - 2026-10-24

- The groove is built off the audio thread and swapped in: changing a setting no longer allocates memory on the audio thread.
- Lighter editor: redraws and rebuilds only when something changes, presets indexed by genre, unused code removed. No change to the sound.

## [0.10.0] - 2026-10-23

- Random grooves more varied: nearby seeds no longer give near copies, single-lane dice vary the second bar, Jackin' Piano adds 16th pushes, '90s octaves on the beats, Funky & Disco Nile-style 16ths and disco octaves, Gospel 8th-note licks, Dance octaves; busy scratches leave holes.
- Fixed: parameters read without allocating on the audio thread (over 100 allocations per block before); the editor no longer rebuilds the groove 30 times a second.

## [0.9.0] - 2026-10-22

- Random grooves stay in the selected genre: funk and jackin' on 16th chank and scratch, house and '90s on offbeat stabs, garage on 2-step skips, deep and soulful on sparse extended chords, afro on picked clave, neo-soul and chill on laid-back voicings with a lick, dance on short stabs, gospel on 8th strums.
- A new seed changes the groove, and bar 2 is its own bar rather than a copy of bar 1.

## [0.8.0] - 2026-10-22

- Fixed: some generated grooves were silent. A pattern of only weak 16ths on an up-strum, chank or scratch fell to about MIDI 60, and the guide guitar (velocity squared) made that nothing. Every generated bar now keeps a speaking accent. Ghosts stay quieter than the beat, and the offbeat stays the loudest 16th.
- A funk, jackin' or jackin' piano intro that was only a ghost scratch now has one chank on the "&".

## [0.7.0] - 2026-10-22

- Random grooves follow the genre: funk stays on alternate 16ths, gospel on 8ths, afro on tumbao and partido alto, neo-soul and pads stay sparse, dance power sits on the downbeats. Bar 2 answers bar 1.
- Turnarounds and section variations keep that grid, and do not add a lick or a scratch to a genre that does not play one.
- Funky chord dice stay in the funk progressions. Dance stabs have their own minor power vamps.
- A random groove is voiced in the style (shells and drop-2 for gospel and neo-soul, triads for house and dance, small grips for funk). The lick phrase follows the style. Preset grooves keep their voicing.
- Intros, build-ups and mix fills follow the genre: funk keeps the scratch, gospel a soft strum, dance the power chords, pads the swell.

## [0.6.0] - 2026-10-22

- Fixed: "Always" in the grid's Chance menu was read as a roll, so the chance stayed put and the roll was cleared.
- Fixed: "The same chord as..." did nothing for bars 5–8.
- Fixed: an edit on a 2-bar pattern changed only the first half; switching to 4 bars played a stale second half (symbol, velocity, chance and roll).
- Fixed: a slide on beat 1 put its grace note before the start, so it never sounded.
- Fixed: humanize revoiced every stab, slide and chank on the last bar of a phrase. It now does so only now and then, and more often as humanize rises.

## [0.5.0] - 2026-10-20

- Thirteen additional `Arranger:` presets coordinate palettes, articulation and balance with Fase Groove Arranger 0.4.7, including the updated Jackin in the funky/disco direction.
- Previous preset indexes and values remain available; saved projects are not rewritten and the theme and control layout stay unchanged.

## [0.4.0] - 2026-10-19

- Real guitar shapes: every chord type on every root is voiced as a playable shape (one note per string, within four frets, a barre and three fingers), the 3rd and 7th always there. The shapes move with the progression: each chord stays close to the one before.
- Strums: up strums quicker and on the top strings, accents quicker, the middle strings of a down strum loudest, ghost strums on the top strings only, a tiny per-string humanize; a plain strum never takes longer than most of a 16th.
- Chank on the top two strings of the stab (funk style); the muted scratch and ghost strums are dead notes for sampled guitars.
- New PLAYING... menu: Let ring (strums sound until the next chord), Room for bass & piano (no low strings, stabs higher), Push chord changes (the last 8th plays the next chord), Keyswitches for sampled guitars (sustain, palm mute, dead note, slide; notes C-2..D#1) with CC 11 volume swells, also in exported per-lane tracks.
- Slides overlap their target (legato on sampled guitars); a new chord no longer cuts the grace note.
- Fixed: chord tones dropped in some voicings (C# maj9/9/13/6/9 lost the 3rd); the power chord takes the chord's own 5th on diminished and augmented chords; the lick scale follows the chord (dominant, half-diminished, diminished, augmented, sus).
- Fixed: a chord held as an inversion (C/E) was read with the wrong root.
- Fixed: two chord hits on one step (or a big strum over a scratch) cut strings to nothing: they are one strum now, and a string always sounds at least 30 ms.
- Fixed: choosing the relative key (C major to A minor) on the circle or over KEY link moved the chords; a key changed by the host now moves them too.
- Fixed: no memory allocated on the audio thread for chords and notes; octaves stay within the guitar's range; the turnaround bar follows a new random seed; "Lydian Jack" named bVII+.

## [0.3.4] - 2026-10-19

- Preserve early template/push hits at high tempos; regression checks compare different block sizes. README and privacy now describe the existing optional update check.
- Shared Piano/Drums/Guitar control theme: matching buttons, menus, embedded Inter fonts, focus and active states; existing layouts are preserved.
- Note-offs keep the channel used by their note-on, including a channel change while playing.
- Old presets restore defaults for missing parameters instead of inheriting the previous preset.
- MIDI Start and Song Position Pointer replace the old clock position in the current block.
- Equal-time event ordering no longer needs the temporary allocation used by stable_sort.
- Empty audio blocks are handled without invalid sample positions.
- Slow rakes retain all delayed strings with small audio blocks and high tempos. Manual auditions preserve the strum across blocks; All Notes Off also stops guide voices.

## [0.3.3] - 2026-10-19

- KEY shows the key itself ("KEY  A Dorian"); the groove's display is a little narrower to make room. KEY can be switched on before LINK: it works once linked.

## [0.3.2] - 2026-10-19

- Safer exports: every .mid (and .wav) is written to a temporary file first; the one already there is replaced only once the new one is complete (a full disk or no permission keeps the old file).
- Room reserved for very large audio blocks (offline rendering): nothing grows on the audio thread.
- MIDI chord input: the same note struck twice stays held until both are released; All Notes Off lets go of all.

## [0.3.1] - 2026-10-19

- **KEY** beside Sequencer (with LINK on): the same key and scale as Fase Groove Piano; the chords move the shortest way. The panel's title is now just TEMPO.
- Dragging a clip: the window shrinks at once to the small strip of Fase Groove Piano (it waited for the drop before, so it often didn't), and comes back by itself.

## [0.3.0] - 2026-10-19

- **LEAD** (in the guitar part's heading, with LINK on): the guitar plays the main riff and a linked Fase Groove Piano only fills in a chord now and then. Off, or without LINK, nothing changes.
- MIDI export rebuilt: every note paired with its end, a string struck again ends the note before, two lanes on the same note at once give one note, nothing hanging or zero-length.
- MIDI chord input: no memory allocated on the audio thread.
- New tests on every preset groove: clean notes, one track and the tracks per lane alike, the same settings the same clip.

## [0.2.2] - 2026-10-19

- The interface always in the chosen language (some texts stayed in English when the plug-in opened).
- Song window identical to Fase Groove Piano's: orange drag tiles, total on two lines, fills as wide as the row, "Main groove" on the timeline, RESET beside the title.
- Bigger PUSH and SWING knobs in the mixer.

## [0.2.1] - 2026-10-19

- Much lighter interface: the undo history is saved only after an edit (no more snapshots of the whole state twice a second) and the panels refresh 10 times a second, the lights 30.
- While a clip is dragged the window shrinks to a small strip, as in Fase Groove Piano, so the DAW's tracks are in view.
- The song window laid out as Fase Groove Piano's: the three drag tiles, SAVE and CLOSE on one row.
- MIDI export cleaned up: note-offs before note-ons on the same tick, a chord note struck again is ended first, no zero-length or hanging notes. Chords now sound in any instrument as they do in the plug-in.
- Audio thread: no more sorting with allocations in every block.

## [0.2.0] - 2026-10-19

- **174 chord progressions in a browser**, as the grooves': ten categories, every progression with
  its chords in the key. Fase Groove Piano's progressions (built on 13 scales as the piano builds
  them: triads, 7ths or 9ths, chords from outside the scale) and more for the guitar (disco &
  '90s, garage & UK, afro & latin, neo soul & jazz, gospel, circle of fifths).
- **The circle of fifths** of Fase Groove Piano in the browser: change the key, drag a chord onto
  another, see how well the chords follow each other; **NEW ROUND THE CIRCLE OF 5THS**.
- **1 to 8 chords**, one per bar (two rows of slots from five on); chord types m7b5, dim and aug.
- **A smaller window and a big editor.** The main window shows the groove at a glance (click it to
  edit) and, in a second tab, the volume faders; the dice, MY GROOVES, IMPORT, TEMPLATE and CLEAR
  stay there. **EDITOR...** opens the editor over the whole window, as Fase Groove Piano's piano
  roll: transport, a brush (accent, hit, ghost, rake), step grid, velocity, mixer and fill. The
  right-click menu of a step can delete it or move it one 16th left or right.
- **LINK and PLAY: no more stutters** (the beat goes on block by block, following the shared clock gently).

## [0.1.0] - 2026-10-19

The first version: the guitar of the Fase Groove family, with the same interface as
Fase Groove Piano and Fase Groove Drums.

- **43 guitar grooves in 13 genres** (the same as the piano and the drums): offbeat chords, house
  strummers, the Nile Rodgers chicken scratch, disco octaves, highlife picking, montunos, two-step
  chops, gospel strums, neo soul licks, chill swells, dance power chords...
- **12 ways to play a chord**, one per row of the grid: down strum, up strum, stab, chank, muted
  scratch, slide, swell, arpeggio, lick, octaves, power chord, root. Each voiced as a guitarist
  plays it in standard tuning; a new chord stops the one still ringing (one guitar, six strings);
  the muted scratch fills the 16ths between the other hits by itself.
- **Chords:** the key, 18 progressions by genre, four chord slots (one per bar), a die for new
  chords. **MIDI IN**: a chord held on the MIDI input (your keyboard, or Fase Groove Piano's MIDI
  out) is what the guitar plays.
- **The strum:** the time between two strings (0–40 ms), down strums low to high, up strums high to
  low, the rake (`f`) slower; in the guide sound and in the MIDI.
- **Guide guitar:** synthesized plucked strings in four sounds (Clean Funk, Warm Jazz, Disco Chorus,
  Nylon), with tone, reverb and volume.
- **NEW GROOVE** (random grooves by genre, from a few chords to busy 16ths), dice per row and per
  part A-D with locks, **TOTAL** random, **STYLE** setups by genre, **RESET**.
- **Fills** (strum build, octave run, scratch build, chank lift, slide up, stab stutter, lick run,
  arpeggio run, bass walk, swell, drop out, mix, custom), a **big strum** after every fill,
  **variations**, **song sections** and the **song window** with the whole song as one clip.
- **VELOCITY**, **MIXER** (with push / pull and swing per row, **TIMING** feels) and **FILL** views,
  groove **templates**, undo / redo, presets, **MY GROOVES**.
- **MIDI import**: a MIDI file's rhythm into the grid and its chords into the chord slots.
- **Export**: drag the MIDI clip into the DAW (one track or one per row), save `.mid`, WAV export
  (loop, song, stems).
- **LINK** with Fase Groove Piano and Drums: the same genre, swing, tempo and play button, on the
  same beat.
- Interface in 6 languages, automatic updates, Windows installer.

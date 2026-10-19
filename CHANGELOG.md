# Changelog

*🇮🇹 [Versione italiana](CHANGELOG.it.md)*

All notable changes to **Fase Groove Guitar**.

## [0.3.4] - 2026-10-19

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

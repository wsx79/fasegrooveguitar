# Fase Groove Guitar – User guide

*🇮🇹 [Guida in italiano](GUIDA.md)*

This guide covers [v0.5.0](https://github.com/wsx79/fasegrooveguitar/releases/tag/v0.5.0). The existing controls and their positions are retained; shared buttons, menus and colours have a common style.

**Arranger presets:** the style menu now includes thirteen `Arranger: …` entries coordinated with Fase Groove Arranger 0.4.7. Choose the same genre in Piano, Drums and Guitar and use LINK for key, tempo and transport. Jackin follows the requested funky/disco direction, with chopped hooks, articulated bass and percussive guitar. Earlier presets remain available and saved projects retain their settings; DAW levels and effects still contribute to the final mix.

## 1. Installation

**Windows:** run `FaseGrooveGuitar-Windows-Setup.exe` from the
[Releases](https://github.com/wsx79/fasegrooveguitar/releases/latest) page (or unzip the zip and copy
the files as below). **macOS / Linux:** download the zip for your system and unzip it.

| System | Copy | To |
|---|---|---|
| Windows | `Fase Groove Guitar.vst3` | `C:\Program Files\Common Files\VST3\` |
| macOS | `Fase Groove Guitar.vst3` / `Fase Groove Guitar.component` | `~/Library/Audio/Plug-Ins/VST3/` / `~/Library/Audio/Plug-Ins/Components/` |
| Linux | `Fase Groove Guitar.vst3` | `~/.vst3/` |

Then rescan the plug-ins in your DAW. On macOS, if the plug-in is blocked, run once in Terminal:
`xattr -cr ~/Library/Audio/Plug-Ins/VST3/"Fase Groove Guitar.vst3"` (and the same for the `.component`).
The **Standalone** app works without a DAW.

Load it as an **instrument** on a MIDI track. Press play in the DAW (or **PLAY** in the plug-in)
and the groove plays with the guide guitar.

## 2. The window

The same layout as Fase Groove Piano and Fase Groove Drums:

- **Header:** PRESETS, TOTAL (everything random), undo / redo, full screen, STYLE, RESET, OPTIONS;
  TRANSPORT & TEMPO (⏮ ▶ ■, SYNC DAW, the tempo, LINK, Sequencer); the display (groove, genre,
  the chord now, the section, where the playhead is).
- **GROOVE:** the groove, the genre of the dice, density, part length and the parts A-D.
- **GUITAR PART:** the row picked on the grid. **CHORDS:** key, progression, the four chords.
- **GROOVE FEEL & SONG**, **GUIDE SOUND & MIDI OUT**.
- The big **grid** (or VELOCITY, MIXER, FILL) and **EXPORT MIDI & AUDIO**.

## 3. The groove

- **GROOVE:** 43 grooves in 13 genres (the same as Fase Groove Piano and Drums); click its name for
  the browser (genres on the left, each groove with a picture of its rhythm). **Random groove** is
  the one of NEW GROOVE, **User pattern** the one edited on the grid.
- **RANDOM / GROOVE (dice):** a new random groove in the genre chosen in **GROOVE GENRE**.
  **DENSITY** goes from a few chords to busy 16th strumming with scratches.
- **Parts A-D:** the groove in four parts (half a bar each, or one bar with **Part length: 1 bar**);
  play them in any order (A A A B...), roll one with its die, lock it so the dice leave it alone.
- **STYLE** (top): a genre's groove, chords, swing, feel, fills and sound in one click. **RESET**
  puts everything back to the default.

## 4. Chords

- **Key** (the menu on the left): the chords move with it.
- **PROGRESSION...:** the progression browser: 174 progressions in ten categories, each with its
  chords in the key; a click loads one. On the right the **circle of fifths** (as in Fase Groove
  Piano): click a key to change it; the dots are your chords: drag one onto another chord to change
  it. Under it: key, **scale** (13, as the piano's), **colour** (triads, 7ths, 9ths) and how many
  **chords** play (1 to 8, one per bar), and **NEW ROUND THE CIRCLE OF 5THS**: each chord a fifth
  below the one before, sometimes with a secondary dominant. The **die** next to PROGRESSION... picks
  a progression of the groove genre (or a new circle).
- **The four slots:** one chord per bar, round and round (four bars). Click one to change its root or
  its type (maj7, m7, 7, m9, maj9, 9, m11, 13, 7sus4, 6/9, major, minor), copy another bar's chord,
  or put it on every bar. The slot playing now is lit.
- **MIDI IN** (on by default): when a chord is held on the plug-in's **MIDI input**, the guitar plays
  that chord (with its own rhythm) instead of the slots, and follows it as it changes. Play your
  keyboard into the guitar's track, or route **Fase Groove Piano's MIDI out** into it: the guitar
  then plays the piano's chords. The lowest note held is the root. One note alone plays as a power
  chord. Off: the notes of the MIDI input pass through (and play the guide guitar).

## 5. The grid

12 rows, the ways a guitarist plays a chord, × 32 steps (two bars of 16ths, or 64 with parts of one bar):

| Row | What it plays |
|---|---|
| **DOWN** (down strum) | the full chord on six strings, low to high |
| **UP** (up strum) | the top four strings, high to low |
| **STAB** | three high strings, short |
| **CHANK** | one high string, very short (the funk single-note chop) |
| **SCRATCH** (muted scratch) | the strings muted by the hand: a click with no pitch. It fills the 16ths no other chord hit plays |
| **SLIDE** | three high strings slid up from a semitone below |
| **SWELL** | the full chord fading in and held (the volume knob trick) |
| **ARP** (arpeggio) | one string at a time, up and down the chord |
| **LICK** | a phrase on the chord's pentatonic (minor over a minor chord) |
| **OCTAVE** (octaves) | the root and its octave (disco!) |
| **POWER** (power chord) | root, 5th and octave on the low strings |
| **ROOT** | the root on the low strings |

A new chord hit stops the chord still ringing: one guitar, six strings.

| Action | Result |
|---|---|
| Click | hit (click again: rest) |
| Shift + click | accent |
| Alt + click | ghost (very soft) |
| Ctrl / Cmd + click | **rake**: a slower strum, every string heard |
| Drag | paints |
| Right-click a step | the step's chance (it plays now and then) and a roll (2-4 quick strums) |
| Click a row's name | hear it over the chord of the bar, and pick it in GUITAR PART |
| Double-click a row's name | its velocities |
| Right-click a row's name | mute, a new line, clear, octave, level |
| A row's die | a new line for that row only |

The first edit of a preset turns it into the **User pattern** (the preset stays as it was).

## 6. Guitar part

The row picked: **MUTE**, its die, **OCTAVE** (up or down by octaves, the voicing keeps its shape),
**HEAR IT**, **LEVEL** (its MIDI velocities) and **VOLUME** (in the guide sound only). **PLAYS**
shows the notes it plays over the chord now.

## 7. Feel

- **SWING:** pushes every other 16th later, shown as drum machines show it: 50% straight,
  67% triplet shuffle, 75% the heaviest. Each genre brings its own swing (the same as the drums).
- **HUMANIZE:** small timing and velocity changes, and the soft notes come and go over a four-bar
  phrase. **NEW FEEL** makes another one (saved with the project).
- **VELOCITY:** overall level. **ACCENTS:** up-strums, scratches, chanks and picking louder on the
  offbeat and softer on the other 16ths, as funk guitarists play.
- **MIXER:** each row's **push / pull** (ms ahead of or behind the beat) and share of the swing;
  **TIMING...** has ready-made feels (laid-back strums, pushed scratches, jackin', lazy neo soul...).
- **TEMPLATE...:** the timing and accents of an audio loop or a MIDI file on any groove (drop an
  audio loop on the window to use it as a template).

## 8. Song

- **Fills** every 1, 2, 4 or 8 bars, in a **fill style**: strum build, octave run, scratch build,
  chank lift, slide up, stab stutter, lick run, arpeggio run, bass walk, swell, drop out, mix (a
  different one now and then) or **custom** (drawn in the FILL view).
- **BIG STRUM:** a big down strum on the bar after every fill (and every 8 bars of the drop).
- **Variation:** every 4 or 8 bars the last bar turns round (a slide, a pick-up, a lick, a stop).
- **Song section:** Intro / Outro (chanks and scratches), Break (a held chord), Build-up (a
  scratch building up), Drop, Main.
- **FULL SONG...:** the song window: the bars, own variation and closing fill of each section;
  drag or save the whole song (with markers), one section, or every section as its own clip.

## 9. Sound & MIDI

- **Guide sound:** the guide guitar, synthesized plucked strings: **Clean Funk**, **Warm Jazz**,
  **Disco Chorus**, **Nylon**, with **TONE**, **REVERB** and **VOLUME**. Switch it off to play the
  MIDI with your own guitar instrument.
- **STRUM:** the time between two strings of a strum (0 ms: every string at once). It is in the MIDI
  too, so your guitar instrument strums the same way.
- **MIDI CH.:** the channel of the notes (1 by default).
- **Sequencer:** off, the groove doesn't play with the DAW (e.g. while the dragged clip plays on the
  same track); ▶ still previews it.

## 10. Export and import

- **DRAG MIDI TO DAW:** drag the orange tile onto a guitar (or keys) track. **Notes:** all rows on one
  track, or one track per row. **Clip length:** the full loop (until groove, chords, fills and
  variations come round), 1–16 bars, or the whole song.
- **SAVE .MID...**, **SAVE .WAV...** (the guide sound: a seamless loop, the whole song, or stems).
- **IMPORT...** (or drop a `.mid` on the window): a MIDI file (a guitar part, chords, a piano riff):
  its rhythm into the grid, each hit in the row it sounds like (a strum, a stab, octaves, one
  note...), and the chords of its first four bars into the chord slots.
- **MY GROOVES:** save the grid as your own groove, load it in any project. **PRESETS:** everything.

## 11. LINK

**LINK** (in TRANSPORT & TEMPO) links Fase Groove Guitar with **Fase Groove Piano** and **Fase Groove
Drums** (and other instances): the same genre, swing, tempo, DAW sync and play button in all of them,
on the same beat. Change it in any one. To have the guitar play the piano's chords, route the
piano's MIDI out into the guitar's track (MIDI IN, section 4).

## 12. Shortcuts and options

- **Space:** play / pause. **Ctrl+Z / Ctrl+Shift+Z (Ctrl+Y):** undo / redo.
- **OPTIONS** (or a click on the logo): language (English, Italiano, Español, Français, Deutsch,
  Português), interface size, tips, automatic updates; **ABOUT**: credits and links.

## 13. Playing with the piano and new in 0.4

- **KEY** (beside Sequencer) shows the key and scale ("KEY  A Dorian"). On in the guitar and in
  Fase Groove Piano, with LINK on, they keep the same key and scale; the chords move the shortest way.
  Choosing the relative key (C major to A minor) keeps the chords where they are.
- **LEAD** (in the GUITAR PART heading, with LINK on): the guitar plays the main riff and a linked
  Fase Groove Piano only answers now and then, in its grid, piano roll and exported MIDI too.
- **Real guitar shapes:** every chord is a playable shape (one note per string, within four frets,
  3rd and 7th always there) and stays close to the chord before.
- **Strums:** up strums quicker and on the top strings, the middle strings of a down strum loudest,
  ghost strums on the top strings, a tiny per-string humanize. The chank plays the top two strings.
- **PLAYING...** (in GUIDE SOUND & MIDI OUT):
  - *Let ring*: strums sound until the next chord.
  - *Room for bass & piano*: no low strings, stabs higher.
  - *Push chord changes*: the last 8th of a bar plays the next chord.
  - *Keyswitches* for sampled guitars (sustain, palm mute, dead note, slide; notes C-2 to D#1) and
    CC 11 volume swells, also in the exported per-lane tracks.
- Slides overlap their target (legato on sampled guitars). Exports are written to a temporary file
  first: a failed save never loses the old clip.

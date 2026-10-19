// Fase Groove Guitar - Copyright (C) 2026 Fabio 'Pitch' Serra
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

// The guitar: the lanes of the grid (the ways a house guitarist plays a chord: strums,
// stabs, chanks, muted scratches, arpeggios, licks, octaves...), the chords and how each lane
// voices them on the neck, and the groove format shared by the presets, the random grooves
// and the user pattern. Pure C++ (no JUCE).

#include <array>
#include <string>
#include <vector>

namespace fgg
{

constexpr double stepLength = 0.25;   // a 16th note, in quarter notes

//==============================================================================
/** One row of the grid: one way of playing the chord of the bar. */
enum class Lane { strumDown = 0, strumUp, stab, chank, scratch, slide, swell, arp, lick, octave, power, root };
constexpr int numLanes = 12;

const char* laneName (int lane) noexcept;        // "Down strum", "Muted scratch" ...
const char* laneShortName (int lane) noexcept;   // "DOWN", "SCRATCH" ... (the grid's row labels)

/** The lanes that play the whole chord (or most of it) at once: a strum, a stab... A guitar has
    six strings: a new chord hit stops the chord still ringing. */
bool isChordLane (int lane) noexcept;

//==============================================================================
/** Chord qualities, as the chord slots show them. */
enum class ChordType { maj7 = 0, m7, dom7, m9, maj9, dom9, m11, dom13, sus4, sixNine, major, minor };
constexpr int numChordTypes = 12;
const char* chordTypeName (int type) noexcept;   // "maj7", "m7", "7" ... ("" for a major triad)
/** The intervals of a chord type, as a bit mask over the 12 semitones above the root. */
unsigned chordTypeMask (int type) noexcept;

const char* pitchClassName (int pitchClass) noexcept;   // C, C#, D, Eb ...

struct Chord
{
    int root = 9;                          // pitch class: 0 = C ... 9 = A
    int type = (int) ChordType::m7;
    bool operator== (const Chord& o) const noexcept { return root == o.root && type == o.type; }
    bool operator!= (const Chord& o) const noexcept { return ! (*this == o); }
};
std::string chordName (const Chord&);           // "Am7", "Fmaj9", "C"

/** Chord tones: a root and the intervals above it (a bit mask, bit 0 = the root). A chord from
    the slots, or the notes held on the MIDI input. */
struct ChordTones
{
    int root = 9;
    unsigned mask = 0x489;                 // root, minor 3rd, 5th, minor 7th
    bool operator== (const ChordTones& o) const noexcept { return root == o.root && mask == o.mask; }
};
ChordTones tonesOf (const Chord&) noexcept;
/** The chord a set of held notes makes: the lowest note is the root. mask == 0: nothing held. */
ChordTones tonesOfNotes (const std::vector<int>& notes) noexcept;

/** The MIDI notes a lane plays over a chord, lowest first, as a guitarist voices them in
    standard tuning (between E2 = 40 and about A5): a down strum the full six-string chord, a stab
    three high strings, a chank one, octaves two notes an octave apart... An arpeggio and a lick
    play one note, chosen by the step (0..15) in the bar. `octave` moves it all up or down. */
std::vector<int> laneNotes (Lane lane, const ChordTones& chord, int stepInBar, int octave = 0);

//==============================================================================
/** Chord progressions (four bars, one chord each), by genre, as intervals above the key. */
struct Progression
{
    const char* name;
    const char* category;
    std::array<int, 4> degree;             // semitones above the key
    std::array<ChordType, 4> type;
};
const std::vector<Progression>& progressions();
/** The four chords of a progression in a key (0 = C ... 11 = B). */
std::array<Chord, 4> progressionChords (int progression, int key);

//==============================================================================
/** A guitar groove: one line per lane, one character per 16th step:
      'X' accent   'x' hit   'g' ghost (very soft)   'f' rake (a slower strum, every string heard)   '.' rest
    All lines have the same length (16 = one bar, 32 = two bars). */
struct GuitarPattern
{
    std::string name;
    std::string category {};   // genre heading in the groove menu
    std::array<std::string, numLanes> lanes {};

    int length() const noexcept { return (int) lanes[0].size(); }
    std::string& operator[] (Lane l) noexcept             { return lanes[(size_t) l]; }
    const std::string& operator[] (Lane l) const noexcept { return lanes[(size_t) l]; }
};

inline bool isHit (char c) noexcept { return c == 'X' || c == 'x' || c == 'g' || c == 'f'; }

/** Every lane the same length (the longest one, shorter lines repeated), unknown symbols
    as rests: what every pattern goes through before it plays. */
void normalisePattern (GuitarPattern&, int minimumLength = 16);

//==============================================================================
/** The preset grooves, grouped by genre (the same 13 genres as Fase Groove Piano and Drums). */
const std::vector<GuitarPattern>& guitarPatterns();

/** Genre headings of the groove menu, in display order. */
const std::vector<const char*>& grooveCategories();
int categoryIndex (const std::string& category) noexcept;   // -1 when unknown

/** Swing that suits a genre (0 = straight, 1 = full triplet shuffle). */
float genreSwing (int category) noexcept;

/** The progression that suits a genre (an index into progressions()). */
int genreProgression (int category) noexcept;

/** Index of the "Random groove" entry, right after the preset grooves. */
int randomPatternIndex() noexcept;

/** Index of the "User pattern" entry (editable grid), after "Random groove". */
int userPatternIndex() noexcept;

constexpr int userPatternLength = 64;   // the user pattern: up to four bars of 16ths
constexpr int randomBeatLength = 32;    // a random groove: two bars

/** Every lane exactly `length` steps: longer lines are cut, shorter ones repeated. */
void fitPattern (GuitarPattern&, int length);

} // namespace fgg

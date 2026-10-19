// Fase Groove Guitar - Copyright (C) 2026 Fabio 'Pitch' Serra
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

// Drum sequencer. Turns a musical-time window (in quarter notes, "ppq") into
// note on/off events. Pure C++ (no JUCE).

#include "Guitar.h"
#include "Generators.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace fgg
{

struct GuitarSettings
{
    GuitarSettings() noexcept;

    int pattern = 0;
    int grooveBars = 2;                    // 2 (parts A-D of half a bar) or 4 (parts of one bar)
    float swing = 0.15f;                   // 0 = straight, 1 = triplet shuffle, up to 1.5 (a dotted 16th: the heaviest)
    int velocity = 100;
    float humanize = 0.2f;
    float accentGroove = 0.85f;             // up-strums, scratches, chanks, picking: louder on the offbeat, softer 16ths (0 = flat)
    std::uint32_t humanizeSeed = 0;        // NEW FEEL: another, repeatable humanization

    int fillEveryBars = 0;                 // 0 = no fills
    FillStyle fillStyle = FillStyle::strumBuild;
    bool strumAfterFill = true;            // a big strum on the bar after a fill (and every 8 bars of the drop)
    int variationEvery = 0;                // 0 = off, else every 4 or 8 bars the last bar varies

    Section section = Section::main;
    long long sectionStartBar = 0;         // where the section began (the drop's crash counts from there)

    std::uint32_t riffSeed = 1;            // used when pattern == randomPatternIndex()
    float density = 0.5f;
    int randomGenre = 0;                   // the genre NEW GROOVE writes in (grooveCategories() index)

    // The chords: one per bar, four bars round. When liveChord is set (notes held on the MIDI
    // input, e.g. from Fase Groove Piano), every bar plays that chord instead.
    std::array<Chord, 4> chords {};
    bool useLiveChord = false;
    ChordTones liveChord {};
    float strumMs = 12.0f;                 // the time between two strings of a strum

    std::array<int, numLanes> octaves {};  // each lane up or down by octaves (0 = as voiced)
    std::array<float, numLanes> levels {}; // velocity scale of each lane (1 = as written)
    std::array<bool, numLanes> muted {};

    // The feel of each lane: how much of the swing it takes (1 = as the Swing control) and how
    // far it sits ahead (< 0) or behind (> 0) the beat, in milliseconds.
    std::array<float, numLanes> laneSwing {};
    std::array<float, numLanes> laneShift {};
    double tempo = 120.0;                  // turns the milliseconds into beats

    // A groove template (e.g. taken from an audio loop): when active, the timing and the level of
    // every 16th of the bar come from it instead of the swing and the accent groove.
    struct Template
    {
        bool active = false;
        std::array<float, 16> timing {};   // in 16ths, < 0 early, > 0 late
        std::array<float, 16> level {};    // 1 = as written
    };
    Template grooveTemplate;

    // Used when pattern == userPatternIndex(): the hand-edited grid, and the velocity set by
    // hand on each of its steps (0 = automatic, from the symbol).
    std::array<std::array<char, userPatternLength>, numLanes> user {};
    std::array<std::array<unsigned char, userPatternLength>, numLanes> userVelocity {};
    // ...and the chance each of those steps plays (0 or 100 = always) and its roll (0 or 1 = one
    // hit, 2-4 = that many quick strums in the 16th).
    std::array<std::array<unsigned char, userPatternLength>, numLanes> userChance {};
    std::array<std::array<unsigned char, userPatternLength>, numLanes> userRoll {};

    // The fill drawn by hand (Fill style: Custom): one bar. It takes over from its first written
    // step; the lanes it doesn't use keep playing the groove.
    std::array<std::array<char, 16>, numLanes> customFill {};

    std::array<int, numSegments> segmentOrder { 0, 1, 2, 3 };   // A B C D
    std::uint32_t sectionVariation = 0;    // a song section's own version of the groove (0: as it is)

    // A song section with its own fill: when forcedFillBar >= 0 that is the only bar with a fill,
    // in forcedFillStyle (< 0: no fill at all).
    long long forcedFillBar = -1;
    int forcedFillStyle = -1;
};

struct NoteEvent
{
    double ppq;
    int note;
    int velocity;   // 0 for note-off
    int lane = 0;
    bool isNoteOn() const noexcept { return velocity > 0; }
};

//==============================================================================
class GrooveEngine
{
public:
    GrooveEngine();

    /** Appends (time-ordered) events falling in [startPpq, endPpq). */
    void render (double startPpq, double endPpq, const GuitarSettings& settings, std::vector<NoteEvent>& out);

    /** Releases everything currently sounding at the given time. */
    void allNotesOff (double ppq, std::vector<NoteEvent>& out);

    bool anyNoteSounding() const noexcept;

    /** The groove before the section is applied: a preset, the random groove or the user pattern. */
    static GuitarPattern basePattern (const GuitarSettings& s);

    /** The groove actually played: the base pattern, in the A-D order, its section's own
        version, then rewritten for the song section. */
    static GuitarPattern resolvePattern (const GuitarSettings& s);
    static int patternLength (const GuitarSettings& s) noexcept;

    /** Length (in 16th steps) after which groove, chords, variations and fills all repeat. */
    static int loopLengthInSteps (const GuitarSettings& s) noexcept;

    /** Offline render of a whole clip starting at bar 1, with every note closed. */
    static std::vector<NoteEvent> renderClip (const GuitarSettings& s, double lengthInBeats);

    /** Renders a whole song (intro, main, break...) with section markers. */
    struct Marker { double ppq; std::string name; };
    static std::vector<NoteEvent> renderArrangement (const GuitarSettings& s, const std::vector<ArrangementPart>& parts,
                                                     std::vector<Marker>* markers = nullptr);

    /** The fill style a bar plays ("Mix" picks one per bar). */
    static FillStyle effectiveFillStyle (FillStyle style, long long bar) noexcept;

    /** The chord tones of a bar: the chord of its slot, or the one held on the MIDI input. */
    static ChordTones chordAt (const GuitarSettings& s, long long bar) noexcept;

private:
    struct PendingOn { double ppq; int note; int velocity; double duration; int lane; bool choke = false; };

    const GuitarPattern& patternFor (const GuitarSettings& s);
    void collectStep (long long step, const GuitarSettings& s, const GuitarPattern& pattern, const GuitarPattern& variation);
    void flushOffsUpTo (double ppq, bool inclusive, std::vector<NoteEvent>& out);

    std::array<double, 128> offTime {};   // < 0 when the note is not sounding
    std::array<int, 128> laneOf {};
    std::vector<PendingOn> pending;
    std::vector<int> scratch;

    // Cache of the resolved pattern, rebuilt only when its inputs change.
    GuitarPattern cachedPattern, cachedVariation;
    int cachedIndex = -1;
    std::uint32_t cachedSeed = 0;
    float cachedDensity = -1.0f;
    int cachedGenre = -1;
    Section cachedSection = Section::main;
    std::array<std::array<char, userPatternLength>, numLanes> cachedUser {};
    std::array<int, numSegments> cachedOrder {};
    int cachedBars = 2;
    std::uint32_t cachedSectionVariation = 0;
};

} // namespace fgg

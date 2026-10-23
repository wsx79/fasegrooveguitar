// Fase Groove Guitar - Copyright (C) 2026 Fabio 'Pitch' Serra
// SPDX-License-Identifier: AGPL-3.0-or-later

// Engine tests (no JUCE): grooves, chords and voicings, sections, random grooves, fills and
// the note stream.

#include "GrooveEngine.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <array>
#include <map>
#include <set>
#include <string>

using namespace fgg;

static int failures = 0;

#define CHECK(cond) \
    do { if (! (cond)) { std::printf ("FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond); ++failures; } } while (false)

namespace
{
    int countHits (const std::string& line)
    {
        return (int) std::count_if (line.begin(), line.end(), isHit);
    }

    /** Every note-on is closed by a note-off, in time order, and no note is started twice. */
    bool wellFormed (const std::vector<NoteEvent>& events)
    {
        std::map<int, bool> on;
        double last = -1.0e9;
        for (const auto& e : events)
        {
            if (e.ppq < last - 1.0e-9)
                return false;
            last = e.ppq;
            if (e.isNoteOn())
            {
                if (on[e.note])
                    return false;
                on[e.note] = true;
            }
            else
                on[e.note] = false;
        }
        return std::none_of (on.begin(), on.end(), [] (const auto& p) { return p.second; });
    }

    int countOns (const std::vector<NoteEvent>& events, int lane = -1)
    {
        return (int) std::count_if (events.begin(), events.end(),
                                    [lane] (const NoteEvent& e) { return e.isNoteOn() && (lane < 0 || e.lane == lane); });
    }

    /** A groove with nothing but the lines given, played straight. */
    GuitarSettings only (std::initializer_list<std::pair<Lane, const char*>> lines)
    {
        GuitarSettings s;
        s.swing = 0.0f;
        s.humanize = 0.0f;
        s.accentGroove = 0.0f;
        s.fillEveryBars = 0;
        s.variationEvery = 0;
        s.strumAfterFill = false;
        s.pattern = userPatternIndex();
        for (auto& lane : s.user)
            lane.fill ('.');
        for (const auto& [lane, steps] : lines)
        {
            std::string line;
            for (const char* c = steps; *c != 0; ++c)
                if (*c != ' ')
                    line += *c;
            for (int i = 0; i < userPatternLength; ++i)
                s.user[(size_t) lane][(size_t) i] = line[(size_t) (i % (int) line.size())];
        }
        return s;
    }

    bool isChordTone (const ChordTones& c, int note)
    {
        return (c.mask >> (unsigned) (((note - c.root) % 12 + 12) % 12)) & 1u;
    }

    /** A shape a hand can play: one note per string on side by side strings (low to high), the
        fretted notes within four frets. */
    bool playable (const NoteList& notes)
    {
        for (int first = 0; first + notes.size() <= 6; ++first)
        {
            int low = 99, high = 0;
            bool fits = true;
            for (int i = 0; i < notes.size(); ++i)
            {
                const int fret = notes[i] - openStrings[(size_t) (first + i)];
                fits = fits && fret >= 0 && fret <= 15;
                if (fret > 0)
                    low = std::min (low, fret), high = std::max (high, fret);
            }
            if (fits && (high < low || high - low <= 3))
                return true;
        }
        return false;
    }

    /** Whether the note stream has a note shorter than `beats`. */
    bool hasNoteShorterThan (const std::vector<NoteEvent>& events, double beats)
    {
        std::map<int, double> on;
        for (const auto& e : events)
        {
            if (e.isController() || e.lane == keyswitchLane)
                continue;
            if (e.isNoteOn())
                on[e.note] = e.ppq;
            else if (on.count (e.note) != 0)
            {
                if (e.ppq - on[e.note] < beats)
                    return true;
                on.erase (e.note);
            }
        }
        return false;
    }
}

//==============================================================================
static void testShapes()
{
    // Every chord type on every root: real shapes, one note per string, within four frets, the
    // 3rd always there; from no chord, after another chord, with room for the band.
    for (int band = 0; band < 2; ++band)
    {
        ChordVoicing previous;
        bool chained = false;
        for (int root = 0; root < 12; ++root)
            for (int type = 0; type < numChordTypes; ++type)
            {
                const auto chord = tonesOf ({ root, type });
                const auto v = voiceChord (chord, chained ? &previous : nullptr, band == 1);
                for (const auto* shape : { &v.full, &v.stab, &v.four, &v.stabAlt })
                {
                    CHECK (! shape->empty() && playable (*shape));
                    for (int n : *shape)
                        CHECK (isChordTone (chord, n) && n >= 40 && n <= 84);
                }
                CHECK (v.full.size() >= 4 && v.full.front() % 12 == root);
                if (band == 1)
                    CHECK (v.full.front() >= 48 && v.stab.front() >= 60);
                const bool hasThird = (chord.mask & ((1u << 3u) | (1u << 4u))) != 0;
                bool third = ! hasThird;
                for (int n : v.full)
                    third = third || (((n - root) % 12 + 12) % 12 == 3 || ((n - root) % 12 + 12) % 12 == 4);
                CHECK (third);
                previous = v;
                chained = true;
            }
    }

    // The shapes move: from Am7 to Dm7 the strummed chord stays near (no jump across the neck).
    const auto am = voiceChord (tonesOf ({ 9, (int) ChordType::m7 }));
    const auto dm = voiceChord (tonesOf ({ 2, (int) ChordType::m7 }), &am);
    CHECK (std::abs (dm.stab.back() - am.stab.back()) <= 5);

    // An inversion keeps its root: C/E is C, not E.
    CHECK (tonesOfNotes ({ 52, 55, 60 }) == tonesOf ({ 0, (int) ChordType::major }));
    CHECK (tonesOfNotes ({ 45, 48, 52, 55 }).root == 9);
    // The power chord takes the flat 5th of a diminished chord; the lick leaves the 5th out
    // over a half-diminished one.
    const auto dim = tonesOf ({ 11, (int) ChordType::halfDim });
    const auto power = laneNotes (Lane::power, dim, 0);
    CHECK (power[1] - power[0] == 6);
    for (int step = 0; step < 16; ++step)
        CHECK (((laneNotes (Lane::lick, dim, step)[0] - 11) % 12 + 12) % 12 != 7);
    // The octave control never leaves the guitar.
    for (int o : { -2, 2 })
        for (int l = 0; l < numLanes; ++l)
            for (int n : laneNotes ((Lane) l, tonesOf ({ 4, (int) ChordType::m11 }), 0, o))
                CHECK (n >= 40 && n <= 88);
}

static void testPlayingStyle()
{
    // A big strum and a scratch on the same downbeat: no string cut to nothing.
    {
        GuitarSettings s;
        s.pattern = 15;   // Jack Stab: offbeat stabs over a scratch on every 16th
        CHECK (guitarPatterns()[(size_t) s.pattern].name == "Jack Stab");
        s.fillEveryBars = 2;
        s.strumAfterFill = true;
        const auto events = GrooveEngine::renderClip (s, 32.0);
        CHECK (wellFormed (events));
        CHECK (! hasNoteShorterThan (events, 0.02));
    }
    // Two chord hits on one step are one strum.
    {
        auto s = only ({ { Lane::strumDown, "x... .... .... ...." }, { Lane::stab, "X... .... .... ...." } });
        const auto events = GrooveEngine::renderClip (s, 4.0);
        CHECK (countOns (events, (int) Lane::strumDown) == 0 && countOns (events, (int) Lane::stab) == 3);
        CHECK (! hasNoteShorterThan (events, 0.02));
    }
    // Up strums are quicker than down strums.
    {
        auto spread = [] (Lane lane, const char* line)
        {
            auto s = only ({ { lane, line } });
            s.strumMs = 20.0f;
            double first = 1.0e9, last = -1.0e9;
            int n = 0;
            for (const auto& e : GrooveEngine::renderClip (s, 1.0))
                if (e.isNoteOn())
                    first = std::min (first, e.ppq), last = std::max (last, e.ppq), ++n;
            return (last - first) / std::max (1, n - 1);
        };
        CHECK (spread (Lane::strumUp, "..x.") < spread (Lane::strumDown, "x...") * 0.8);
    }
    // Let ring: a strum sounds until the next chord hit.
    {
        auto s = only ({ { Lane::strumDown, "x... .... x... ...." } });
        auto ringLength = [&s]
        {
            double longest = 0.0;
            std::map<int, double> on;
            for (const auto& e : GrooveEngine::renderClip (s, 4.0))
                if (e.isNoteOn())
                    on[e.note] = e.ppq;
                else if (on.count (e.note) != 0 && on[e.note] < 1.0)
                    longest = std::max (longest, e.ppq - on[e.note]);
            return longest;
        };
        const double plain = ringLength();
        s.letRing = true;
        CHECK (ringLength() > plain * 2.0 && ringLength() <= 2.0 + 1.0e-9);
    }
    // Room for the band: no strummed note below C3, the root and power chord up an octave.
    {
        auto s = only ({ { Lane::strumDown, "x... x... x... x..." }, { Lane::power, "..x. ..x. ..x. ..x." } });
        s.roomForBand = true;
        for (const auto& e : GrooveEngine::renderClip (s, 16.0))
            if (e.isNoteOn())
                CHECK (e.note >= 48);
    }
    // The last 8th of a bar pushes into the next chord.
    {
        auto s = only ({ { Lane::stab, ".... .... .... ..x." } });
        s.chords = { Chord { 9, (int) ChordType::m7 }, Chord { 1, (int) ChordType::major }, Chord { 9, (int) ChordType::m7 }, Chord { 1, (int) ChordType::major } };
        s.numChords = 2;
        for (const auto& e : GrooveEngine::renderClip (s, 4.0))
            if (e.isNoteOn())
                CHECK (isChordTone (tonesOf (s.chords[1]), e.note));
        s.pushChanges = false;
        for (const auto& e : GrooveEngine::renderClip (s, 4.0))
            if (e.isNoteOn())
                CHECK (isChordTone (tonesOf (s.chords[0]), e.note));
    }
    // Keyswitches: one before the first note of each articulation, below the guitar; the swell
    // rises with CC 11; the slide's grace note overlaps its chord.
    {
        auto s = only ({ { Lane::stab, "x... .... .... ...." }, { Lane::scratch, ".x.. .... .... ...." }, { Lane::chank, "..x. .... .... ...." },
                         { Lane::swell, ".... X... .... ...." }, { Lane::slide, ".... .... X... ...." } });
        s.keyswitches = true;
        const auto events = GrooveEngine::renderClip (s, 4.0);
        CHECK (wellFormed (events));
        std::vector<int> switches;
        for (const auto& e : events)
            if (e.isNoteOn() && e.lane == keyswitchLane)
                switches.push_back (e.note);
        CHECK ((switches == std::vector<int> { 24, 26, 25, 24, 27 }));
        int rising = 0;
        for (const auto& e : events)
            rising += e.isController() && e.note == 11 ? 1 : 0;
        CHECK (rising >= 2);
        double graceOff = -1.0, chordOn = 1.0e9;
        for (const auto& e : events)
            if (e.lane == (int) Lane::slide)
            {
                if (e.isNoteOn() && e.ppq < 2.0 - 1.0e-9 && graceOff < 0.0)
                    graceOff = 0.0;
                if (e.isNoteOn() && e.ppq >= 2.0 - 1.0e-9)
                    chordOn = std::min (chordOn, e.ppq);
                if (! e.isNoteOn() && ! e.isController() && e.ppq > 1.5 && e.ppq < 2.5 && graceOff >= 0.0 && graceOff < 1.0)
                    graceOff = e.ppq;
            }
        CHECK (graceOff > chordOn);
        s.keyswitches = false;
        for (const auto& e : GrooveEngine::renderClip (s, 4.0))
            CHECK (e.lane != keyswitchLane && ! e.isController());
    }
}

//==============================================================================
static void testPresets()
{
    const auto& presets = guitarPatterns();
    CHECK (presets.size() >= 39);
    std::map<std::string, int> perGenre;
    std::set<std::string> names;
    for (const auto& p : presets)
    {
        CHECK (p.length() == 16 || p.length() == 32);
        CHECK (categoryIndex (p.category) >= 0);
        CHECK (names.insert (p.name).second);   // every name once
        int hits = 0;
        for (const auto& line : p.lanes)
        {
            CHECK ((int) line.size() == p.length());
            hits += countHits (line);
        }
        CHECK (hits >= 3);
        ++perGenre[p.category];
    }
    for (const auto* genre : grooveCategories())
        CHECK (perGenre[genre] >= 3);   // every genre has a choice of grooves

    // ...and a real choice: two grooves of a genre differ by at least 10 hits over two bars.
    for (size_t a = 0; a < presets.size(); ++a)
        for (size_t b = a + 1; b < presets.size(); ++b)
            if (presets[a].category == presets[b].category)
            {
                auto x = presets[a], y = presets[b];
                fitPattern (x, 32);
                fitPattern (y, 32);
                int differences = 0;
                for (size_t l = 0; l < (size_t) numLanes; ++l)
                    for (size_t i = 0; i < 32; ++i)
                        differences += isHit (x.lanes[l][i]) != isHit (y.lanes[l][i]) ? 1 : 0;
                CHECK (differences >= 10);
            }
    CHECK (grooveCategories().size() == 13);
    CHECK (userPatternIndex() == randomPatternIndex() + 1);
}

static void testLanesAndChords()
{
    for (int l = 0; l < numLanes; ++l)
        CHECK (std::string (laneName (l)).size() > 2 && std::string (laneShortName (l)).size() > 1);
    CHECK (isChordLane ((int) Lane::strumDown) && isChordLane ((int) Lane::scratch) && ! isChordLane ((int) Lane::root));

    CHECK (chordName ({ 9, (int) ChordType::m7 }) == "Am7");
    CHECK (chordName ({ 5, (int) ChordType::maj9 }) == "Fmaj9");
    CHECK (chordName ({ 0, (int) ChordType::major }) == "C");
    CHECK (chordTypeMask ((int) ChordType::m7) == ((1u << 0) | (1u << 3) | (1u << 7) | (1u << 10)));

    // A progression follows the key.
    const auto inA = progressionChords (0, 9), inC = progressionChords (0, 0);
    CHECK (inA[0] == (Chord { 9, (int) ChordType::m7 }) && inA[1] == (Chord { 2, (int) ChordType::m7 }));
    CHECK (inC[0] == (Chord { 0, (int) ChordType::m7 }) && inC[1] == (Chord { 5, (int) ChordType::m7 }));
    for (int g = 0; g < 13; ++g)
        CHECK (genreProgression (g) >= 0 && genreProgression (g) < (int) progressions().size());

    // Held notes: the lowest is the root.
    const auto held = tonesOfNotes ({ 57, 60, 64, 67 });   // A C E G
    CHECK (held.root == 9 && held == tonesOf ({ 9, (int) ChordType::m7 }));
    CHECK (tonesOfNotes ({}).mask == 0);
    {
        std::array<bool, 128> keys {};
        keys[57] = keys[60] = keys[64] = keys[67] = true;
        int count = 0;
        CHECK (tonesOfHeld (keys, count) == tonesOfNotes ({ 57, 60, 64, 67 }) && count == 4);
        keys.fill (false);
        CHECK (tonesOfHeld (keys, count).mask == 0 && count == 0);
    }
}

static void testVoicings()
{
    // Every lane over every chord: on the neck, only chord tones (the lick: its pentatonic), the
    // right shape.
    for (int root = 0; root < 12; ++root)
        for (int type = 0; type < numChordTypes; ++type)
        {
            const auto chord = tonesOf ({ root, type });
            for (int l = 0; l < numLanes; ++l)
                for (int step = 0; step < 16; ++step)
                {
                    const auto notes = laneNotes ((Lane) l, chord, step);
                    CHECK (! notes.empty());
                    CHECK (std::is_sorted (notes.begin(), notes.end()));
                    for (int n : notes)
                    {
                        CHECK (n >= 40 && n <= 84);   // E2 .. C6
                        if ((Lane) l != Lane::lick && (Lane) l != Lane::power)   // (a power chord is root and 5th, always)
                            CHECK (isChordTone (chord, n));
                    }
                }

            const auto full = laneNotes (Lane::strumDown, chord, 0);
            CHECK (full.size() >= 4 && full.size() <= 6);
            CHECK (full.front() % 12 == root);                       // the root on the bottom
            CHECK (full.back() - full.front() <= 36);
            const auto stab = laneNotes (Lane::stab, chord, 0);
            CHECK (stab.size() == 3);
            for (size_t i = 0; i + 1 < stab.size(); ++i)
                CHECK (stab[i + 1] - stab[i] >= 2);                  // no semitone clusters
            const auto chank = laneNotes (Lane::chank, chord, 0);
            CHECK (chank.size() == 2 && chank.back() == stab.back());   // the stab's top two strings
            CHECK (laneNotes (Lane::root, chord, 0) == std::vector<int> { full.front() });
            const auto oct = laneNotes (Lane::octave, chord, 0);
            CHECK (oct.size() == 2 && oct[1] - oct[0] == 12 && oct[0] % 12 == root);
            const auto power = laneNotes (Lane::power, chord, 0);
            CHECK (power.size() == 3 && power[2] - power[0] == 12);
            CHECK (isChordTone (chord, power[1])                                       // its own 5th (flat on a diminished chord)
                   || (power[1] - power[0] == 7 && ! isChordTone (chord, power[0] + 6) && ! isChordTone (chord, power[0] + 8)));
            CHECK (laneNotes (Lane::strumUp, chord, 0).size() <= 4);
        }

    // The octave control moves a lane.
    const auto c = tonesOf ({ 0, (int) ChordType::maj7 });
    CHECK (laneNotes (Lane::stab, c, 0, 1)[0] == laneNotes (Lane::stab, c, 0)[0] + 12);
    // The arpeggio walks the strings.
    std::set<int> arp;
    for (int step = 0; step < 6; ++step)
        arp.insert (laneNotes (Lane::arp, c, step)[0]);
    CHECK (arp.size() >= 3);
}

static void testRenderBasics()
{
    for (size_t i = 0; i < guitarPatterns().size(); ++i)
    {
        GuitarSettings s;
        s.pattern = (int) i;
        const auto events = GrooveEngine::renderClip (s, 32.0);
        CHECK (wellFormed (events));
        CHECK (countOns (events) > 0);
        for (const auto& e : events)
            CHECK (e.velocity >= 0 && e.velocity <= 127 && e.lane >= 0 && e.lane < numLanes);
    }

    // A block-by-block render is the same as one render.
    GuitarSettings s;
    s.pattern = 4;
    const auto whole = GrooveEngine::renderClip (s, 16.0);
    GrooveEngine engine;
    std::vector<NoteEvent> blocks;
    for (double t = 0.0; t < 16.0; t += 0.37)
        engine.render (t, std::min (16.0, t + 0.37), s, blocks);
    engine.allNotesOff (16.0, blocks);
    CHECK (countOns (blocks) == countOns (whole));
}

static void testStrumsAndChords()
{
    // One down strum on beat 1: the strings low to high, a few ms apart, the chord of bar 1.
    auto s = only ({ { Lane::strumDown, "X... .... .... ...." } });
    s.chords = { Chord { 9, (int) ChordType::m7 }, Chord { 2, (int) ChordType::m7 }, Chord { 7, (int) ChordType::dom7 }, Chord { 0, (int) ChordType::maj7 } };
    s.tempo = 120.0;
    s.strumMs = 10.0f;
    auto events = GrooveEngine::renderClip (s, 16.0);
    CHECK (wellFormed (events));
    std::vector<NoteEvent> bar1, bar2;
    for (const auto& e : events)
        if (e.isNoteOn())
            (e.ppq < 4.0 ? bar1 : e.ppq < 8.0 ? bar2 : bar1).push_back (e);
    // (bar1 here has bars 1, 3 and 4: only check bar 1)
    std::vector<NoteEvent> first;
    for (const auto& e : events)
        if (e.isNoteOn() && e.ppq < 1.0)
            first.push_back (e);
    CHECK (first.size() >= 4);
    for (size_t i = 0; i + 1 < first.size(); ++i)
    {
        CHECK (first[i].note < first[i + 1].note);                                  // low to high
        CHECK (std::abs ((first[i + 1].ppq - first[i].ppq) - 0.85 * 10.0 * 120.0 / 60000.0) < 1.0e-6);   // 10 ms apart (an accent: quicker)
    }
    for (const auto& e : first)
        CHECK (isChordTone (tonesOf (s.chords[0]), e.note));
    for (const auto& e : bar2)
        CHECK (isChordTone (tonesOf (s.chords[1]), e.note));
    CHECK (first.front().note % 12 == 9);    // A on the bottom
    CHECK (bar2.front().note % 12 == 2);     // D

    // An up strum: high to low.
    s = only ({ { Lane::strumUp, "..x. .... .... ...." } });
    events = GrooveEngine::renderClip (s, 4.0);
    std::vector<int> up;
    for (const auto& e : events)
        if (e.isNoteOn())
            up.push_back (e.note);
    CHECK (up.size() >= 3 && std::is_sorted (up.rbegin(), up.rend()));

    // Strum 0 ms: every string at once.
    s = only ({ { Lane::strumDown, "X... .... .... ...." } });
    s.strumMs = 0.0f;
    events = GrooveEngine::renderClip (s, 4.0);
    for (const auto& e : events)
        if (e.isNoteOn())
            CHECK (std::abs (e.ppq) < 1.0e-9);

    // A rake spreads the strings wider than a plain strum.
    auto spread = [] (char symbol)
    {
        auto st = only ({ { Lane::strumDown, symbol == 'f' ? "f..." : "x..." } });
        st.strumMs = 10.0f;
        double first = 1.0e9, last = -1.0e9;
        for (const auto& e : GrooveEngine::renderClip (st, 1.0))
            if (e.isNoteOn())
                first = std::min (first, e.ppq), last = std::max (last, e.ppq);
        return last - first;
    };
    CHECK (spread ('f') > spread ('x') * 2.0);

    // The chord held on the MIDI input replaces the slots.
    s = only ({ { Lane::stab, "X... X... X... X..." } });
    s.useLiveChord = true;
    s.liveChord = tonesOf ({ 4, (int) ChordType::dom9 });
    for (const auto& e : GrooveEngine::renderClip (s, 16.0))
        if (e.isNoteOn())
            CHECK (isChordTone (s.liveChord, e.note));
}

static void testOneGuitar()
{
    // A swell rings for most of the bar... until a stab: one guitar, six strings.
    auto s = only ({ { Lane::swell, "X... .... .... ...." }, { Lane::stab, ".... .... X... ...." } });
    const auto events = GrooveEngine::renderClip (s, 4.0);
    CHECK (wellFormed (events));
    for (const auto& e : events)
        if (! e.isNoteOn() && e.lane == (int) Lane::swell)
            CHECK (e.ppq <= 2.0 + 1.0e-9);

    // The scratch fills the gaps: where a strum plays, no scratch.
    s = only ({ { Lane::scratch, "xxxx xxxx xxxx xxxx" }, { Lane::strumDown, "X... X... X... X..." } });
    std::set<int> scratchSteps;
    for (const auto& e : GrooveEngine::renderClip (s, 4.0))
        if (e.isNoteOn() && e.lane == (int) Lane::scratch)
            scratchSteps.insert ((int) std::floor (e.ppq / stepLength + 1.0e-6));
    CHECK (scratchSteps.size() == 12);
    for (int step : { 0, 4, 8, 12 })
        CHECK (scratchSteps.count (step) == 0);

    // A muted lane is silent; the octave control moves its notes.
    s = only ({ { Lane::root, "X... .... .... ...." } });
    const int low = GrooveEngine::renderClip (s, 4.0).front().note;
    s.octaves[(size_t) Lane::root] = 1;
    CHECK (GrooveEngine::renderClip (s, 4.0).front().note == low + 12);
    s.muted[(size_t) Lane::root] = true;
    CHECK (countOns (GrooveEngine::renderClip (s, 4.0)) == 0);
}

static void testSwingAndHumanize()
{
    auto s = only ({ { Lane::chank, "xxxx xxxx xxxx xxxx" } });
    s.swing = 1.0f;
    s.strumMs = 0.0f;
    for (const auto& e : GrooveEngine::renderClip (s, 4.0))
        if (e.isNoteOn())
        {
            const double inStep = std::fmod (e.ppq, 2.0 * stepLength);
            CHECK (std::abs (inStep) < 1.0e-6 || std::abs (inStep - stepLength * 4.0 / 3.0) < 1.0e-6);   // triplet shuffle
        }

    // Humanize: repeatable, and NEW FEEL changes it.
    s.swing = 0.0f;
    s.humanize = 0.5f;
    const auto a = GrooveEngine::renderClip (s, 16.0), b = GrooveEngine::renderClip (s, 16.0);
    CHECK (a.size() == b.size());
    s.humanizeSeed = 77;
    const auto c = GrooveEngine::renderClip (s, 16.0);
    bool differs = c.size() != a.size();
    for (size_t i = 0; ! differs && i < a.size(); ++i)
        differs = std::abs (a[i].ppq - c[i].ppq) > 1.0e-9 || a[i].velocity != c[i].velocity;
    CHECK (differs);
}

static bool barsDiffer (const GuitarPattern& p)
{
    for (const auto& line : p.lanes)
        if (line.size() >= 32 && line.compare (0, 16, line, 16, 16) != 0)
            return true;
    return false;
}

static void testStyleCoherence()
{
    CHECK (std::string (genreProgressionCategory (8)) == "Jackin' & funky");
    CHECK (std::string (genreProgressionCategory (12)) == "Dance stabs");
    int danceProg = 0;
    for (const auto& progression : progressions())
        if (std::string (progression.category) == "Dance stabs")
        {
            ++danceProg;
            CHECK (std::string (progression.name).find ("Anthem") == std::string::npos);
        }
    CHECK (danceProg >= 3);

    for (std::uint32_t seed = 1; seed < 24; ++seed)
    {
        const auto funk = makeRandomPattern (seed, 0.85f, 8);
        CHECK (barsDiffer (funk));
        for (size_t i = 0; i < funk[Lane::strumDown].size(); ++i)
            if (isHit (funk[Lane::strumDown][i]))
                CHECK (i % 2 == 0);
        for (size_t i = 0; i < funk[Lane::strumUp].size(); ++i)
            if (isHit (funk[Lane::strumUp][i]))
                CHECK (i % 2 == 1);
        const auto gospel = makeRandomPattern (seed, 0.7f, 3);
        CHECK (barsDiffer (gospel));
        for (const auto& line : gospel.lanes)
            for (size_t i = 0; i < line.size(); ++i)
                if (isHit (line[i]))
                    CHECK (i % 2 == 0);

        const auto dance = makeRandomPattern (seed, 0.8f, 12);
        CHECK (barsDiffer (dance));
        CHECK (countHits (dance[Lane::lick]) == 0);
        CHECK (countHits (dance[Lane::scratch]) == 0);
        for (size_t i = 0; i < dance[Lane::power].size(); ++i)
            if (isHit (dance[Lane::power][i]))
                CHECK (i % 4 == 0);
        const auto varied = makeSectionVariation (dance, seed + 9);
        CHECK (countHits (varied[Lane::lick]) == 0);
        CHECK (countHits (varied[Lane::scratch]) == 0);
        for (const auto& line : varied.lanes)
            for (size_t i = 0; i < line.size(); ++i)
                if (isHit (line[i]))
                    CHECK (i % 2 == 0);

        const auto afro = makeRandomPattern (seed, 0.75f, 7);
        CHECK (barsDiffer (afro));
        CHECK (countHits (afro[Lane::arp]) + countHits (afro[Lane::chank]) > 0);
    }

    const auto gospel = makeRandomPattern (4, 0.6f, 3);
    const auto section = makeSectionVariation (gospel, 11);
    for (const auto& line : section.lanes)
        for (size_t i = 0; i < line.size(); ++i)
            if (isHit (line[i]))
                CHECK (i % 2 == 0);

    const auto dom9 = tonesOf ({ 0, (int) ChordType::dom9 });
    const auto funkShape = voiceChord (dom9, nullptr, false, GuitarColour::funkGrip);
    const auto shell = voiceChord (dom9, nullptr, false, GuitarColour::shell);
    const auto triad = voiceChord (dom9, nullptr, false, GuitarColour::triad);
    CHECK (funkShape.full.size() >= 2 && funkShape.full.size() <= 3);
    CHECK (shell.full.size() >= 3 && shell.full.size() <= 4);
    CHECK (triad.full.size() >= 3 && triad.full.size() <= 4);
    for (int note : triad.full)
    {
        const int interval = ((note - dom9.root) % 12 + 12) % 12;
        CHECK (interval != 10 && interval != 11 && interval != 2);
    }
    NoteList classicLick, funkLick;
    const auto classic = voiceChord (dom9);
    laneNotes (Lane::lick, dom9, classic, 4, 0, classicLick);
    laneNotes (Lane::lick, dom9, classic, 4, 0, funkLick, false, false, 2);
    CHECK (classicLick.size() == 1 && funkLick.size() == 1 && classicLick[0] != funkLick[0]);

    const auto intro = applySection (guitarPatterns()[0], Section::intro);
    CHECK (countHits (intro[Lane::strumDown]) == 0);
    bool foundGospel = false;
    for (const auto& preset : guitarPatterns())
        if (preset.category == "Gospel" && countHits (preset[Lane::strumDown]) > 0)
        {
            foundGospel = true;
            const auto gospelIntro = applySection (preset, Section::intro);
            CHECK (countHits (gospelIntro[Lane::strumDown]) + countHits (gospelIntro[Lane::strumUp]) > 0);
            CHECK (countHits (applySection (preset, Section::buildup)[Lane::scratch]) == 0);
        }
    CHECK (foundGospel);
}

static int maxOnVelocity (const std::vector<NoteEvent>& events)
{
    int m = 0;
    for (const auto& e : events)
        if (e.isNoteOn())
            m = std::max (m, e.velocity);
    return m;
}

static bool hasSymbol (const GuitarPattern& p, char symbol, int lane = -1)
{
    for (int l = 0; l < numLanes; ++l)
    {
        if (lane >= 0 && l != lane)
            continue;
        if (p.lanes[(size_t) l].find (symbol) != std::string::npos)
            return true;
    }
    return false;
}

/** Patterns that used to render as a whisper: no accent, only weak 16ths, then the accent
    curve and the velocity floor. Ghosts stay quieter than the accent. */
static void testGeneratedAudibility()
{
    auto base = []
    {
        GuitarSettings s;
        s.pattern = randomPatternIndex();
        s.humanize = 0.3f;
        s.accentGroove = 0.85f;
        s.velocity = 100;
        s.roomForBand = true;
        s.fillEveryBars = 0;
        s.variationEvery = 0;
        s.strumAfterFill = false;
        return s;
    };

    for (int genre = 0; genre < 13; ++genre)
        for (float density : { 0.0f, 0.22f, 0.35f, 0.5f, 0.68f, 1.0f })
            for (std::uint32_t seed = 1; seed <= 60; ++seed)
            {
                const auto pattern = makeRandomPattern (seed, density, genre);
                CHECK (hasSymbol (pattern, 'X'));
                bool speaking = false;
                for (int l = 0; l < numLanes; ++l)
                    if (l != (int) Lane::scratch && hasSymbol (pattern, 'X', l))
                        speaking = true;
                CHECK (speaking);

                auto s = base();
                s.riffSeed = seed;
                s.density = density;
                s.randomGenre = genre;
                s.swing = genreSwing (genre);
                const auto events = GrooveEngine::renderClip (s, 8.0);
                CHECK (countOns (events) > 0);
                CHECK (maxOnVelocity (events) >= 74);
            }

    // The grooves a player reported as silent (piano house, jackin', density at the floor).
    for (const auto& bad : { std::array<double, 3> { 0, 57, 0.0 }, { 4, 14, 0.0 }, { 4, 14, 0.35 }, { 4, 14, 1.0 } })
    {
        auto s = base();
        s.randomGenre = (int) bad[0];
        s.riffSeed = (std::uint32_t) bad[1];
        s.density = (float) bad[2];
        CHECK (maxOnVelocity (GrooveEngine::renderClip (s, 8.0)) >= 84);
    }

    // Funk / jackin' intros used to keep only a ghost scratch.
    for (int genre : { 4, 5, 8 })
        for (std::uint32_t seed = 1; seed <= 24; ++seed)
        {
            auto s = base();
            s.riffSeed = seed;
            s.density = 0.35f;
            s.randomGenre = genre;
            s.section = Section::intro;
            CHECK (maxOnVelocity (GrooveEngine::renderClip (s, 8.0)) >= 68);
        }

    // Accents stay accents: a written accent is louder than a weak 16th, and a ghost is quieter still.
    {
        auto s = only ({ { Lane::chank, "X.x. .x.x X.x. .x.x" }, { Lane::scratch, "g... g... g... g..." } });
        s.accentGroove = 0.85f;
        s.velocity = 100;
        s.humanize = 0.0f;
        int accent = 0, weak = 127, ghost = 127;
        for (const auto& e : GrooveEngine::renderClip (s, 4.0))
        {
            if (! e.isNoteOn())
                continue;
            const int step = (int) std::llround (e.ppq / stepLength);
            if (e.lane == (int) Lane::scratch)
                ghost = std::min (ghost, e.velocity);
            else if (step % 4 == 0)
                accent = std::max (accent, e.velocity);
            else if (step % 4 == 1)
                weak = std::min (weak, e.velocity);
        }
        CHECK (accent >= 85);
        CHECK (weak <= accent - 8);
        CHECK (ghost <= weak - 8);
    }
}

/** Real NEW GROOVE path (makeRandomPattern + render) and the section regenerate, per genre. */
static void testGenreIdentity()
{
    auto barOf = [] (const GuitarPattern& p, int bar)
    {
        std::string s;
        for (const auto& line : p.lanes)
        {
            s += line.size() >= (size_t) (bar * 16 + 16) ? line.substr ((size_t) bar * 16, 16) : line;
            s += '|';
        }
        return s;
    };
    auto gridOf = [] (const GuitarPattern& p)
    {
        std::string s;
        for (const auto& line : p.lanes) { s += line; s += '|'; }
        return s;
    };
    auto hitsOn = [] (const std::string& line, int pred)
    {
        int n = 0;
        for (int i = 0; i < (int) line.size(); ++i)
            if (isHit (line[(size_t) i]) && (pred < 0 || i % 4 == pred))
                ++n;
        return n;
    };

    for (int genre = 0; genre < 13; ++genre)
    {
        const bool extended = [&]
        {
            const auto chords = progressionChords (genreProgression (genre), 0);
            const auto voiced = voiceChord (tonesOf (chords[0]), nullptr, true, guitarColourFor (genre));
            const auto& shape = voiced.stab.empty() ? voiced.full : voiced.stab;
            for (int i = 0; i < shape.size(); ++i)
            {
                const int iv = ((shape[i] - chords[0].root) % 12 + 12) % 12;
                if (iv == 1 || iv == 2 || iv == 6 || iv == 9 || iv == 10 || iv == 11)
                    return true;
            }
            return false;
        }();
        if (genre == 1 || genre == 2 || genre == 3 || genre == 10 || genre == 11)
            CHECK (extended);

        for (float density : { genre == 8 ? 0.68f : genre == 4 ? 0.22f : 0.35f, 0.5f })
        {
            std::set<std::string> grids, bar1s, bar2s;
            int differ = 0, regen = 0, faithful = 0;
            for (std::uint32_t seed = 1; seed <= 20; ++seed)
            {
                const auto p = makeRandomPattern (seed, density, genre);
                grids.insert (gridOf (p));
                bar1s.insert (barOf (p, 0));
                bar2s.insert (barOf (p, 1));
                differ += barsDiffer (p) ? 1 : 0;
                regen += gridOf (p) != gridOf (makeSectionVariation (p, seed * 97u + 11u)) ? 1 : 0;

                const int stab = countHits (p[Lane::stab]);
                const int stabOff = hitsOn (p[Lane::stab], 2);
                const int odd = [&]
                {
                    int n = 0;
                    for (const auto& line : p.lanes)
                        for (size_t i = 0; i < line.size(); ++i)
                            if (isHit (line[i]) && i % 2 == 1)
                                ++n;
                    return n;
                }();
                int total = 0;
                for (const auto& line : p.lanes)
                    total += countHits (line);
                const bool accent = hasSymbol (p, 'X');
                const bool skip = [&]
                {
                    std::string any (32, '.');
                    for (int l = 0; l < numLanes; ++l)
                        if (l != (int) Lane::scratch)
                            for (int i = 0; i < 32 && i < (int) p.lanes[(size_t) l].size(); ++i)
                                if (isHit (p.lanes[(size_t) l][(size_t) i]))
                                    any[(size_t) i] = 'x';
                    int gaps = 0;
                    for (int bar = 0; bar < 2; ++bar)
                    {
                        int prev = -4;
                        for (int i = 0; i < 16; ++i)
                            if (any[(size_t) (bar * 16 + i)] != '.')
                            {
                                if (i - prev >= 3) ++gaps;
                                prev = i;
                            }
                    }
                    return gaps >= 2;
                }();

                bool ok = false;
                switch (genre)
                {
                    case 0: case 5: case 9:   // house, jackin' piano, '90s: offbeat stabs
                        ok = stab >= 3 && stabOff * 100 >= stab * 70 && countHits (p[Lane::power]) == 0;
                        break;
                    case 4: case 8:           // jackin', funk: 16th chank or scratch, with an accent
                        ok = (countHits (p[Lane::chank]) > 0 || countHits (p[Lane::scratch]) > 0) && odd > 0 && accent;
                        break;
                    case 6:                   // garage: 2-step skips
                        ok = skip && total <= 20;
                        break;
                    case 1: case 2:           // soulful, deep: sparse extended chords
                        ok = total <= 18 && extended;
                        break;
                    case 7:                   // afro: picked / clave, not a straight chord stab
                        ok = countHits (p[Lane::arp]) + countHits (p[Lane::chank]) > 0 && odd + hitsOn (p[Lane::arp], 2) + hitsOn (p[Lane::chank], 2) > 0;
                        break;
                    case 10: case 11:         // neo-soul, chill: laid-back, extended, a lick
                        ok = countHits (p[Lane::lick]) > 0 && total <= 22 && extended;
                        break;
                    case 12:                  // dance: short stabs, no 16th scratch, no power chord
                        ok = stab >= 3 && countHits (p[Lane::power]) == 0 && countHits (p[Lane::scratch]) == 0 && odd == 0;
                        break;
                    case 3:                   // gospel: 8th strums, shell voicing
                        ok = odd == 0 && countHits (p[Lane::strumDown]) + countHits (p[Lane::strumUp]) >= 4
                          && countHits (p[Lane::scratch]) == 0 && extended;
                        break;
                    default:
                        ok = true;
                        break;
                }
                faithful += ok ? 1 : 0;

                GuitarSettings s;
                s.pattern = randomPatternIndex();
                s.riffSeed = seed;
                s.density = density;
                s.randomGenre = genre;
                s.swing = genreSwing (genre);
                s.fillEveryBars = 0;
                s.variationEvery = 0;
                s.humanize = 0.2f;
                s.accentGroove = 0.85f;
                CHECK (countOns (GrooveEngine::renderClip (s, 8.0)) > 0);
            }
            CHECK (differ == 20);
            CHECK ((int) grids.size() >= 17);
            CHECK ((int) bar1s.size() >= 16);
            CHECK ((int) bar2s.size() >= 16);
            CHECK (regen >= 17);
            CHECK (faithful >= 16);
        }
    }
}

static void testRandomGrooves()
{
    for (int genre = 0; genre < 13; ++genre)
        for (float density : { 0.0f, 0.5f, 1.0f })
            for (std::uint32_t seed = 1; seed < 40; ++seed)
            {
                const auto p = makeRandomPattern (seed, density, genre);
                CHECK (p.length() == randomBeatLength);
                int hits = 0;
                for (const auto& line : p.lanes)
                    hits += countHits (line);
                CHECK (hits >= 2);
                CHECK (p.category == grooveCategories()[(size_t) genre]);
            }
    // The same seed gives the same groove; another seed another one (most of the time).
    CHECK (makeRandomPattern (5, 0.5f, 8).lanes == makeRandomPattern (5, 0.5f, 8).lanes);
    int same = 0;
    for (std::uint32_t seed = 1; seed < 30; ++seed)
        same += makeRandomPattern (seed, 0.5f, 8).lanes == makeRandomPattern (seed + 100, 0.5f, 8).lanes ? 1 : 0;
    CHECK (same < 5);

    // One lane's die never comes back empty.
    for (int l = 0; l < numLanes; ++l)
        for (int genre = 0; genre < 13; ++genre)
            CHECK (countHits (makeRandomLane ((Lane) l, 9, 0.5f, genre)) > 0);

    // A random groove plays.
    GuitarSettings s;
    s.pattern = randomPatternIndex();
    s.riffSeed = 1234;
    CHECK (countOns (GrooveEngine::renderClip (s, 16.0)) > 0);
}

static void testSections()
{
    GuitarSettings s;
    s.pattern = 1;   // House Strummer
    s.fillEveryBars = 0;
    s.variationEvery = 0;
    for (int section = 0; section < numSections; ++section)
    {
        s.section = (Section) section;
        const auto events = GrooveEngine::renderClip (s, 16.0);
        CHECK (wellFormed (events));
        CHECK (countOns (events) > 0);
        if (s.section == Section::breakdown)
            for (const auto& e : events)
                CHECK (! e.isNoteOn() || e.lane == (int) Lane::swell || e.lane == (int) Lane::arp);
        if (s.section == Section::intro)
            CHECK (countOns (events, (int) Lane::strumDown) == 0);
    }
    const auto build = applySection (guitarPatterns()[0], Section::buildup);
    CHECK (countHits (build[Lane::scratch]) > 4);
}

static void testFillsAndVariations()
{
    for (int style = 0; style < numFillStyles; ++style)
    {
        if (style == (int) FillStyle::custom)
            continue;
        GuitarSettings s;
        s.pattern = 0;
        s.fillEveryBars = 2;
        s.fillStyle = (FillStyle) style;
        const auto events = GrooveEngine::renderClip (s, 16.0);
        CHECK (wellFormed (events));
        // Something plays in the fill's half bar (a drop out: its last stab).
        int inFill = 0;
        for (const auto& e : events)
            inFill += e.isNoteOn() && e.ppq >= 6.0 && e.ppq < 8.0 ? 1 : 0;
        CHECK (inFill > 0);
        CHECK (std::string (fillStyleName (style)).size() > 2);
    }

    // The big strum on the bar after a fill.
    auto s = only ({ { Lane::stab, "..X. ..X. ..X. ..X." } });
    s.fillEveryBars = 1;
    s.fillStyle = FillStyle::chankLift;
    s.strumAfterFill = true;
    int downs = 0;
    for (const auto& e : GrooveEngine::renderClip (s, 8.0))
        downs += e.isNoteOn() && e.lane == (int) Lane::strumDown && e.ppq < 4.5 && e.ppq >= 4.0 ? 1 : 0;
    CHECK (downs >= 4);

    // The custom fill.
    s = only ({ { Lane::stab, "..X. ..X. ..X. ..X." } });
    s.fillEveryBars = 1;
    s.fillStyle = FillStyle::custom;
    s.customFill[(size_t) Lane::lick].fill ('.');
    s.customFill[(size_t) Lane::lick][12] = 'x';
    s.customFill[(size_t) Lane::lick][14] = 'X';
    CHECK (countOns (GrooveEngine::renderClip (s, 4.0), (int) Lane::lick) == 2);

    // Variations change the last bar only.
    const auto p = guitarPatterns()[4];
    for (std::uint32_t seed = 1; seed < 20; ++seed)
    {
        const auto v = makeVariation (p, seed);
        for (int l = 0; l < numLanes; ++l)
            CHECK (v.lanes[(size_t) l].size() == p.lanes[(size_t) l].size());
        const auto section = makeSectionVariation (p, seed);
        CHECK (section.length() == p.length());
    }
}

static void testLoopLengthAndArrangement()
{
    GuitarSettings s;
    s.fillEveryBars = 0;
    s.variationEvery = 0;
    auto setChords = [&s] (const std::vector<Chord>& chords)
    {
        s.numChords = (int) chords.size();
        for (size_t i = 0; i < s.chords.size(); ++i)
            s.chords[i] = chords[i % chords.size()];
    };
    setChords (progressionChords (0, 9));   // i - iv - i - iv: two bars
    CHECK (GrooveEngine::loopLengthInSteps (s) == 32);
    setChords (progressionChords (4, 9));   // four chords
    CHECK (GrooveEngine::loopLengthInSteps (s) == 64);
    setChords (progressionChords (6, 9));   // one chord
    s.grooveBars = 4;
    CHECK (GrooveEngine::loopLengthInSteps (s) == 64);
    s.grooveBars = 2;
    s.fillEveryBars = 8;
    CHECK (GrooveEngine::loopLengthInSteps (s) == 128);

    std::vector<GrooveEngine::Marker> markers;
    const auto song = GrooveEngine::renderArrangement (s, defaultArrangement(), &markers);
    CHECK (wellFormed (song));
    CHECK (markers.size() == defaultArrangement().size());
    CHECK (markers.front().name == "Intro");
    CHECK (std::abs (markers.back().ppq - (8 + 16 + 8 + 4 + 16) * 4.0) < 1.0e-9);
}

static void testPartsAndUserPattern()
{
    // Parts in another order.
    auto p = guitarPatterns()[3];
    fitPattern (p, 32);
    const auto swapped = applySegmentOrder (p, { 1, 0, 2, 3 });
    for (int l = 0; l < numLanes; ++l)
        CHECK (swapped.lanes[(size_t) l].substr (0, 8) == p.lanes[(size_t) l].substr (8, 8));

    // Four-bar grooves.
    GuitarSettings s;
    s.grooveBars = 4;
    CHECK (GrooveEngine::basePattern (s).length() == 64);
    s.pattern = randomPatternIndex();
    CHECK (GrooveEngine::basePattern (s).length() == 64);

    // Chance and roll of a step of the user pattern.
    s = only ({ { Lane::stab, "X..." } });
    s.userRoll[(size_t) Lane::stab][0] = 3;
    int stabs = 0;
    for (const auto& e : GrooveEngine::renderClip (s, 1.0))
        stabs += e.isNoteOn() ? 1 : 0;
    CHECK (stabs == 9);   // three strums of three strings
}

static void testProgressionsAndCircle()
{
    // Many progressions, every one in a known category, one to eight chords, all playable.
    const auto& list = progressions();
    CHECK (list.size() >= 150);
    std::set<std::string> names;
    for (size_t i = 0; i < list.size(); ++i)
    {
        CHECK (names.insert (list[i].name).second);
        bool known = false;
        for (const auto* c : progressionCategories())
            known = known || std::string (c) == list[i].category;
        CHECK (known);
        for (int colour = 0; colour < numChordColours; ++colour)
        {
            const auto chords = progressionChords ((int) i, 9, colour);
            CHECK (! chords.empty() && (int) chords.size() <= maxChords);
            for (const auto& c : chords)
                CHECK (c.root >= 0 && c.root < 12 && c.type >= 0 && c.type < numChordTypes);
        }
    }
    for (const auto* c : progressionCategories())
    {
        int n = 0;
        for (const auto& p : list)
            n += std::string (p.category) == c ? 1 : 0;
        CHECK (n >= 5);
    }

    // Chords built on a scale, as a keyboard player: C major.
    CHECK (diatonicChord (0, 0, 1, (int) ChordColour::ninths) == (Chord { 2, (int) ChordType::m9 }));      // Dm9
    CHECK (diatonicChord (0, 0, 2, (int) ChordColour::ninths) == (Chord { 4, (int) ChordType::m7 }));      // Em7 (no b9)
    CHECK (diatonicChord (0, 0, 4, (int) ChordColour::sevenths) == (Chord { 7, (int) ChordType::dom7 }));  // G7
    CHECK (diatonicChord (0, 0, 6, (int) ChordColour::sevenths) == (Chord { 11, (int) ChordType::halfDim }));
    CHECK (diatonicChord (0, 0, 0, (int) ChordColour::triads) == (Chord { 0, (int) ChordType::major }));
    CHECK (codeChord (0, 31, (int) ChordColour::sevenths) == (Chord { 7, (int) ChordType::dom7 }));        // V7 code
    CHECK (codeChord (0, 17, (int) ChordColour::triads) == (Chord { 5, (int) ChordType::minor }));         // iv

    // Round the circle of fifths: each chord's root a fifth below the one before (or a secondary
    // dominant a fifth above the next), ending on the tonic or the dominant.
    for (std::uint32_t seed = 1; seed < 60; ++seed)
        for (int length : { 3, 4, 6, 8 })
        {
            const auto c = circleOfFifthsProgression (seed, 9, 2, length, (int) ChordColour::sevenths);
            CHECK ((int) c.size() == length);
            CHECK (c.back().root == 9 || c.back().root == 4);
            for (size_t i = 0; i + 1 < c.size(); ++i)
            {
                const int move = ((c[i + 1].root - c[i].root) % 12 + 12) % 12;
                CHECK (move == 5 || move == 6 || (ChordType) c[i].type == ChordType::dom7);   // a fifth down (a tritone in the scale's diminished step)
            }
        }
    CHECK (circleOfFifthsProgression (7, 0, 0, 4, 2) == circleOfFifthsProgression (7, 0, 0, 4, 2));
    CHECK (transitionScore ({ 2, (int) ChordType::m7 }, { 7, (int) ChordType::dom7 }) > transitionScore ({ 2, (int) ChordType::m7 }, { 8, (int) ChordType::dom7 }));
}


static void testSlowRakeAcrossBlocks()
{
    // A whole clip and small host blocks must contain exactly the same delayed strings.
    for (double bpm : { 124.0, 400.0 })
    {
        GuitarSettings s;
        s.pattern = userPatternIndex();
        for (auto& lane : s.user) lane.fill ('.');
        s.user[(size_t) Lane::strumDown][0] = 'f';
        s.user[(size_t) Lane::strumDown][16] = 'f';
        s.strumMs = 40.0f;
        s.tempo = bpm;
        s.humanize = 0.0f;
        s.fillEveryBars = 0;
        s.variationEvery = 0;
        const auto whole = GrooveEngine::renderClip (s, 16.0);
        GrooveEngine engine;
        std::vector<NoteEvent> small;
        for (double t = 0.0; t < 16.0; t += 0.037)
            engine.render (t, std::min (t + 0.037, 16.0), s, small);
        engine.allNotesOff (16.0, small);
        CHECK (whole.size() == small.size());
        for (size_t i = 0; i < std::min (whole.size(), small.size()); ++i)
            CHECK (whole[i].note == small[i].note && whole[i].velocity == small[i].velocity
                   && std::abs (whole[i].ppq - small[i].ppq) < 1.0e-8);
    }
}

static void testSlideOnDownbeatAndAnswer()
{
    // A slide on step 0 used to place its grace note before the clip, and the note was dropped.
    {
        auto s = only ({ { Lane::slide, "X... .... .... ...." } });
        s.strumMs = 0.0f;
        s.humanize = 0.0f;
        const auto shape = laneNotes (Lane::slide, GrooveEngine::chordAt (s, 0), 0);
        const auto events = GrooveEngine::renderClip (s, 1.0);
        CHECK (wellFormed (events));
        std::set<int> on;
        for (const auto& e : events)
            if (e.isNoteOn() && e.lane == (int) Lane::slide)
                on.insert (e.note);
        for (int n : shape)
            CHECK (on.count (n) == 1 && on.count (n - 1) == 1);
    }

    // Humanize answers with the other top note only now and then, not on every hit of bar 4.
    {
        NoteList plain, alt;
        int root = 0, type = 0;
        for (int r = 0; r < 12 && plain == alt; ++r)
            for (int t = 0; t < numChordTypes && plain == alt; ++t)
            {
                const auto v = voiceChord (tonesOf ({ r, t }));
                plain = v.stab;
                alt = v.stabAlt;
                root = r;
                type = t;
            }
        CHECK (plain.back() != alt.back());
        auto s = only ({ { Lane::stab, "X... X... X... X..." } });
        s.chords[0] = { root, type };
        s.numChords = 1;
        s.humanize = 1.0f;
        s.pushChanges = false;
        s.fillEveryBars = 0;
        s.variationEvery = 0;
        std::map<long long, int> top;
        for (const auto& e : GrooveEngine::renderClip (s, 16.0))
            if (e.isNoteOn() && e.lane == (int) Lane::stab)
            {
                const auto step = (long long) std::llround (e.ppq / stepLength);
                if (step >= 48 && step < 64)
                    top[step] = std::max (top.count (step) != 0 ? top[step] : 0, e.note);
            }
        int original = 0, answered = 0;
        for (const auto& [step, note] : top)
            (note == alt.back() ? answered : original) += note == plain.back() || note == alt.back() ? 1 : 0;
        CHECK (original > 0 && answered > 0);
    }
}

static void testEarlyTemplateHitsAcrossBlocks()
{
    GuitarSettings s;
    s.pattern = userPatternIndex();
    for (auto& row : s.user) row.fill ('.');
    const auto lane = (size_t) Lane::slide;
    for (int step = 3; step < 32; step += 4) s.user[lane][(size_t) step] = 'f';
    s.tempo = 400.0;
    s.humanize = 0.0f;
    s.fillEveryBars = 0;
    s.variationEvery = 0;
    s.laneShift[lane] = -40.0f;
    s.laneSwing[lane] = 2.0f;
    s.grooveTemplate.active = true;
    s.grooveTemplate.timing.fill (-0.5f);
    s.grooveTemplate.level.fill (1.0f);
    const auto whole = GrooveEngine::renderClip (s, 16.0);
    GrooveEngine engine;
    std::vector<NoteEvent> small;
    for (double t = 0.0; t < 16.0; t += 0.019)
        engine.render (t, std::min (t + 0.019, 16.0), s, small);
    engine.allNotesOff (16.0, small);
    CHECK (whole.size() == small.size());
    for (size_t i = 0; i < std::min (whole.size(), small.size()); ++i)
        CHECK (whole[i].note == small[i].note && whole[i].velocity == small[i].velocity
               && std::abs (whole[i].ppq - small[i].ppq) < 1.0e-8);
}

/** The dice, measured on the MIDI it plays: for every genre and a few densities, many seeds must
    give different rhythms, lane mixes, pitches and lengths (not only other velocities), no
    groove may come back too often, the genre's rules hold on every throw, and the same seed
    always plays the same notes. Diversity numbers are a floor, not a proof of taste: the
    identity checks beside them keep the variation inside the style. */
static void testDiceDiversity()
{
    struct Stats { int onsets = 0, lanes = 0, pitches = 0, lengths = 0, velocities = 0, maxRepeat = 0; };
    auto settingsFor = [] (int genre, float density, std::uint32_t seed)
    {
        GuitarSettings s;
        s.pattern = randomPatternIndex();
        s.riffSeed = seed;
        s.density = density;
        s.randomGenre = genre;
        s.swing = genreSwing (genre);
        s.humanize = 0.0f;   // the groove itself, not the feel
        s.fillEveryBars = 0;
        s.variationEvery = 0;
        s.strumAfterFill = false;
        return s;
    };
    const int seeds = 40;
    auto seedOf = [] (int k) { return (std::uint32_t) (1 + (k * 2477) % 99991); };   // spread like the dice

    for (int genre = 0; genre < 13; ++genre)
        for (float density : { 0.25f, 0.5f, 0.8f })
        {
            std::set<std::string> onsets, lanes, pitches, lengths, velocities;
            std::map<std::string, int> repeats;
            for (int k = 0; k < seeds; ++k)
            {
                const auto s = settingsFor (genre, density, seedOf (k));
                const auto events = GrooveEngine::renderClip (s, 8.0);
                CHECK (wellFormed (events));
                const auto again = GrooveEngine::renderClip (s, 8.0);   // same seed, same notes
                CHECK (std::equal (events.begin(), events.end(), again.begin(), again.end(), [] (const NoteEvent& a, const NoteEvent& b)
                       { return a.ppq == b.ppq && a.note == b.note && a.velocity == b.velocity && a.lane == b.lane && a.controller == b.controller; }));

                std::string on (32, '.'), laneMix (numLanes, '.'), pitch, length, velocity;
                std::map<int, double> started;
                for (const auto& e : events)
                {
                    if (e.isController() || e.lane == keyswitchLane)
                        continue;
                    const int step = std::clamp ((int) std::floor (e.ppq / stepLength + 1.0e-6), 0, 31);
                    if (e.isNoteOn())
                    {
                        if (e.lane != (int) Lane::scratch)   // the rhythm of the chords, not the ghost scratch under them
                            on[(size_t) step] = 'x';
                        laneMix[(size_t) e.lane] = 'x';
                        pitch += std::to_string (step) + ":" + std::to_string (e.note) + " ";
                        velocity += std::to_string (e.velocity) + " ";
                        started[e.note] = e.ppq;
                    }
                    else if (started.count (e.note) != 0)
                    {
                        length += std::to_string ((int) std::lround ((e.ppq - started[e.note]) / 0.0625)) + " ";
                        started.erase (e.note);
                    }
                }
                onsets.insert (on);
                lanes.insert (laneMix);
                pitches.insert (pitch);
                lengths.insert (length);
                velocities.insert (velocity);
                ++repeats[on];
            }
            Stats st { (int) onsets.size(), (int) lanes.size(), (int) pitches.size(), (int) lengths.size(), (int) velocities.size(), 0 };
            for (const auto& r : repeats)
                st.maxRepeat = std::max (st.maxRepeat, r.second);
            if (std::getenv ("FGG_DICE_REPORT") != nullptr)
                std::printf ("dice %-18s d=%.2f onsets %2d lanes %2d pitch %2d length %2d vel %2d maxRepeat %d\n", grooveCategories()[(size_t) genre],
                             (double) density, st.onsets, st.lanes, st.pitches, st.lengths, st.velocities, st.maxRepeat);

            CHECK (st.onsets >= 32);       // the rhythm itself changes from throw to throw
            CHECK (st.pitches >= 34);      // ...and what is played on it
            CHECK (st.lengths >= 20);      // not only the velocities
            CHECK (st.maxRepeat <= 3);     // no rhythm comes back again and again
            CHECK (st.lanes >= 2);         // more than one way of playing the genre
        }

    // Genres with several roles really use them (the lanes, not just the hits, change).
    for (int genre : { 1, 2, 4, 6, 7, 8, 10, 11 })
    {
        std::set<std::string> mixes;
        for (int k = 0; k < seeds; ++k)
        {
            const auto p = makeRandomPattern (seedOf (k), 0.5f, genre);
            std::string mix;
            for (const auto& line : p.lanes)
                mix += countHits (line) > 0 ? 'x' : '.';
            mixes.insert (mix);
        }
        CHECK ((int) mixes.size() >= 5);
    }

    // Neighbouring seeds are not shifted copies of one another.
    for (int genre = 0; genre < 13; ++genre)
    {
        int same = 0;
        for (std::uint32_t seed = 1; seed <= 60; ++seed)
            same += makeRandomPattern (seed, 0.5f, genre).lanes == makeRandomPattern (seed + 1, 0.5f, genre).lanes ? 1 : 0;
        CHECK (same <= 1);
    }

    // Style identity over the whole population of throws.
    auto ratio = [&] (int genre, auto&& count)
    {
        int yes = 0, all = 0;
        for (int k = 0; k < seeds; ++k)
        {
            const auto p = makeRandomPattern (seedOf (k), 0.5f, genre);
            count (p, yes, all);
        }
        return all > 0 ? (double) yes / (double) all : 0.0;
    };
    auto offbeatStabs = [] (const GuitarPattern& p, int& yes, int& all)
    {
        for (int i = 0; i < p.length(); ++i)
            if (isHit (p[Lane::stab][(size_t) i]))
            {
                ++all;
                yes += i % 4 == 2 ? 1 : 0;
            }
    };
    for (int genre : { 0, 5, 9 })   // house piano styles: the stab lives on the "&"
        CHECK (ratio (genre, offbeatStabs) >= 0.75);
    CHECK (ratio (5, [] (const GuitarPattern& p, int& yes, int& all)   // jackin' piano bounces on the 16ths
                  { ++all; for (int i = 0; i < p.length(); ++i) if (isHit (p[Lane::stab][(size_t) i]) && i % 4 == 3) { ++yes; break; } }) >= 0.4);
    CHECK (ratio (8, [] (const GuitarPattern& p, int& yes, int& all)   // disco octaves are part of funky & disco
                  { ++all; yes += countHits (p[Lane::octave]) > 0 ? 1 : 0; }) >= 0.08);
    CHECK (ratio (3, [] (const GuitarPattern& p, int& yes, int& all)   // gospel answers with an 8th-note lick
                  { ++all; yes += countHits (p[Lane::lick]) > 0 ? 1 : 0; }) >= 0.15);
    for (int genre : { 3, 12 })   // gospel and dance stay on the 8ths
        CHECK (ratio (genre, [] (const GuitarPattern& p, int& yes, int& all)
                      { for (const auto& line : p.lanes) for (size_t i = 0; i < line.size(); ++i) if (isHit (line[i])) { ++all; yes += i % 2 == 0 ? 1 : 0; } }) == 1.0);

    // The lane dice, the turnaround and a section's version: repeatable, and seeds differ.
    for (int genre = 0; genre < 13; ++genre)
    {
        std::set<std::string> laneLines;
        for (int k = 0; k < 20; ++k)
        {
            CHECK (makeRandomLane (Lane::stab, seedOf (k), 0.5f, genre) == makeRandomLane (Lane::stab, seedOf (k), 0.5f, genre));
            laneLines.insert (makeRandomLane (Lane::stab, seedOf (k), 0.5f, genre));
        }
        CHECK ((int) laneLines.size() >= 6);
        const auto p = makeRandomPattern (77, 0.5f, genre);
        CHECK (makeVariation (p, 5).lanes == makeVariation (p, 5).lanes);
        CHECK (makeSectionVariation (p, 5).lanes == makeSectionVariation (p, 5).lanes);
    }
}

int main()
{
    testSlideOnDownbeatAndAnswer();
    testEarlyTemplateHitsAcrossBlocks();
    testSlowRakeAcrossBlocks();
    testPresets();
    testLanesAndChords();
    testVoicings();
    testShapes();
    testPlayingStyle();
    testRenderBasics();
    testStrumsAndChords();
    testOneGuitar();
    testSwingAndHumanize();
    testStyleCoherence();
    testGeneratedAudibility();
    testGenreIdentity();
    testRandomGrooves();
    testDiceDiversity();
    testSections();
    testFillsAndVariations();
    testLoopLengthAndArrangement();
    testPartsAndUserPattern();
    testProgressionsAndCircle();

    if (failures == 0)
        std::printf ("All engine tests passed.\n");
    return failures == 0 ? 0 : 1;
}

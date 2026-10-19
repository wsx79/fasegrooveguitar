// Fase Groove Guitar - Copyright (C) 2026 Fabio 'Pitch' Serra
// SPDX-License-Identifier: AGPL-3.0-or-later

// Engine tests (no JUCE): grooves, chords and voicings, sections, random grooves, fills and
// the note stream.

#include "GrooveEngine.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
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
            CHECK (laneNotes (Lane::chank, chord, 0).size() == 1);
            CHECK (laneNotes (Lane::root, chord, 0) == std::vector<int> { full.front() });
            const auto oct = laneNotes (Lane::octave, chord, 0);
            CHECK (oct.size() == 2 && oct[1] - oct[0] == 12 && oct[0] % 12 == root);
            const auto power = laneNotes (Lane::power, chord, 0);
            CHECK (power.size() == 3 && power[1] - power[0] == 7);
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
        CHECK (std::abs ((first[i + 1].ppq - first[i].ppq) - 10.0 * 120.0 / 60000.0) < 1.0e-6);   // 10 ms apart
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

int main()
{
    testSlowRakeAcrossBlocks();
    testPresets();
    testLanesAndChords();
    testVoicings();
    testRenderBasics();
    testStrumsAndChords();
    testOneGuitar();
    testSwingAndHumanize();
    testRandomGrooves();
    testSections();
    testFillsAndVariations();
    testLoopLengthAndArrangement();
    testPartsAndUserPattern();
    testProgressionsAndCircle();

    if (failures == 0)
        std::printf ("All engine tests passed.\n");
    return failures == 0 ? 0 : 1;
}

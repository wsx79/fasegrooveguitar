// Fase Groove Guitar - Copyright (C) 2026 Fabio 'Pitch' Serra
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "Guitar.h"

#include <algorithm>

namespace fgg
{

namespace
{
    int mod12 (int v) noexcept { return ((v % 12) + 12) % 12; }
}

const char* laneName (int lane) noexcept
{
    static const char* names[] { "Down strum", "Up strum", "Stab", "Chank", "Muted scratch", "Slide",
                                 "Swell", "Arpeggio", "Lick", "Octaves", "Power chord", "Root" };
    return lane >= 0 && lane < numLanes ? names[lane] : "";
}

const char* laneShortName (int lane) noexcept
{
    static const char* names[] { "DOWN", "UP", "STAB", "CHANK", "SCRATCH", "SLIDE",
                                 "SWELL", "ARP", "LICK", "OCTAVE", "POWER", "ROOT" };
    return lane >= 0 && lane < numLanes ? names[lane] : "";
}

bool isChordLane (int lane) noexcept
{
    switch ((Lane) lane)
    {
        case Lane::strumDown: case Lane::strumUp: case Lane::stab: case Lane::chank:
        case Lane::scratch: case Lane::slide: case Lane::swell:
            return true;
        default:
            return false;
    }
}

//==============================================================================
const char* chordTypeName (int type) noexcept
{
    static const char* names[] { "maj7", "m7", "7", "m9", "maj9", "9", "m11", "13", "7sus4", "6/9", "", "m" };
    return type >= 0 && type < numChordTypes ? names[type] : "";
}

unsigned chordTypeMask (int type) noexcept
{
    auto bits = [] (std::initializer_list<int> intervals)
    {
        unsigned m = 0;
        for (int i : intervals)
            m |= 1u << (unsigned) mod12 (i);
        return m;
    };
    switch ((ChordType) type)
    {
        case ChordType::maj7:    return bits ({ 0, 4, 7, 11 });
        case ChordType::dom7:    return bits ({ 0, 4, 7, 10 });
        case ChordType::m9:      return bits ({ 0, 3, 7, 10, 14 });
        case ChordType::maj9:    return bits ({ 0, 4, 7, 11, 14 });
        case ChordType::dom9:    return bits ({ 0, 4, 7, 10, 14 });
        case ChordType::m11:     return bits ({ 0, 3, 7, 10, 14, 17 });
        case ChordType::dom13:   return bits ({ 0, 4, 10, 14, 21 });
        case ChordType::sus4:    return bits ({ 0, 5, 7, 10 });
        case ChordType::sixNine: return bits ({ 0, 4, 7, 9, 14 });
        case ChordType::major:   return bits ({ 0, 4, 7 });
        case ChordType::minor:   return bits ({ 0, 3, 7 });
        case ChordType::m7:
        default:                 return bits ({ 0, 3, 7, 10 });
    }
}

const char* pitchClassName (int pitchClass) noexcept
{
    static const char* names[] { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    return names[mod12 (pitchClass)];
}

std::string chordName (const Chord& c)
{
    return std::string (pitchClassName (c.root)) + chordTypeName (c.type);
}

ChordTones tonesOf (const Chord& c) noexcept
{
    return { mod12 (c.root), chordTypeMask (c.type) };
}

ChordTones tonesOfNotes (const std::vector<int>& notes) noexcept
{
    ChordTones t { 0, 0 };
    if (notes.empty())
        return t;
    const int lowest = *std::min_element (notes.begin(), notes.end());
    t.root = mod12 (lowest);
    for (int n : notes)
        t.mask |= 1u << (unsigned) mod12 (n - lowest);
    return t;
}

//==============================================================================
namespace
{
    bool inChord (const ChordTones& c, int note) noexcept
    {
        return (c.mask >> (unsigned) mod12 (note - c.root)) & 1u;
    }

    /** The chord's intervals, most telling first: the 3rd, the 7th (or 6th), the colour notes,
        the 5th, the root. */
    std::vector<int> intervalsByPriority (const ChordTones& c)
    {
        static constexpr int order[] { 4, 3, 10, 11, 9, 2, 5, 7, 1, 6, 8, 0 };
        std::vector<int> out;
        for (int i : order)
            if ((c.mask >> (unsigned) i) & 1u)
                out.push_back (i);
        if (out.empty())
            out.push_back (0);
        return out;
    }

    /** The root on the low strings: E2 (40) to Eb3 (51). */
    int bassRoot (const ChordTones& c) noexcept { return 40 + mod12 (c.root - 4); }

    /** The whole chord across six strings: the root on the bottom, wide intervals low and
        closer ones on top, every chord tone once before any is doubled. */
    std::vector<int> fullVoicing (const ChordTones& c)
    {
        std::vector<int> notes { bassRoot (c) };
        unsigned used = 1u;
        while (notes.size() < 6)
        {
            const int prev = notes.back();
            const int from = prev + (prev < 50 ? 5 : 3);
            int found = -1;
            for (int p = from; p <= from + 8 && found < 0; ++p)
                if (inChord (c, p) && ((used >> (unsigned) mod12 (p - c.root)) & 1u) == 0)
                    found = p;
            for (int p = from; p <= from + 8 && found < 0; ++p)
                if (inChord (c, p))
                    found = p;
            if (found < 0 || found > 76)
                break;
            notes.push_back (found);
            used |= 1u << (unsigned) mod12 (found - c.root);
        }
        return notes;
    }

    /** A few strings high on the neck (B3 and up), the most telling tones of the chord: what
        stabs, chanks and scratches play. Never two notes a semitone apart. */
    std::vector<int> highVoicing (const ChordTones& c, int count)
    {
        auto intervals = intervalsByPriority (c);
        std::vector<int> notes;
        for (size_t i = 0; i < intervals.size() && (int) notes.size() < count; ++i)
            notes.push_back (59 + mod12 (c.root + intervals[i] - 59));
        if ((int) notes.size() < count)   // a triad on four strings: its most telling tone on top
            notes.push_back (59 + mod12 (c.root + intervals.front() - 59) + 12);
        std::sort (notes.begin(), notes.end());
        for (int pass = 0; pass < 6; ++pass)
        {
            bool moved = false;
            for (size_t i = 0; i + 1 < notes.size(); ++i)
                if (notes[i + 1] - notes[i] <= 1)
                {
                    notes[i] += 12;
                    moved = true;
                    break;
                }
            std::sort (notes.begin(), notes.end());
            notes.erase (std::unique (notes.begin(), notes.end()), notes.end());
            if (! moved)
                break;
        }
        return notes;
    }

    /** The pentatonic of the chord (minor over a minor chord, major otherwise), around the
        high strings. */
    std::vector<int> pentatonic (const ChordTones& c)
    {
        const bool minor = ((c.mask >> 3u) & 1u) != 0 && ((c.mask >> 4u) & 1u) == 0;
        static constexpr int minorScale[] { 0, 3, 5, 7, 10 }, majorScale[] { 0, 2, 4, 7, 9 };
        std::vector<int> notes;
        for (int p = 62; p <= 77; ++p)
            for (int i : (minor ? minorScale : majorScale))
                if (mod12 (p - c.root) == i)
                    notes.push_back (p);
        return notes;
    }
}

std::vector<int> laneNotes (Lane lane, const ChordTones& chord, int stepInBar, int octave)
{
    std::vector<int> notes;
    const int step = ((stepInBar % 16) + 16) % 16;
    switch (lane)
    {
        case Lane::strumDown:
        case Lane::swell:
            notes = fullVoicing (chord);
            break;
        case Lane::strumUp:   // the top four strings
        {
            auto full = fullVoicing (chord);
            notes.assign (full.size() > 4 ? full.end() - 4 : full.begin(), full.end());
            break;
        }
        case Lane::stab:
        case Lane::slide:
            notes = highVoicing (chord, 3);
            break;
        case Lane::scratch:
            notes = highVoicing (chord, 4);
            break;
        case Lane::chank:     // one high string: the top of the stab
            notes = { highVoicing (chord, 3).back() };
            break;
        case Lane::arp:       // up and down four strings
        {
            const auto strings = highVoicing (chord, 4);
            static constexpr int order[] { 0, 1, 2, 3, 2, 1 };
            notes = { strings[(size_t) std::min ((int) strings.size() - 1, order[step % 6])] };
            break;
        }
        case Lane::lick:      // a phrase on the pentatonic that rises and falls over the bar
        {
            const auto scale = pentatonic (chord);
            static constexpr int contour[] { 0, 2, 3, 4, 3, 2, 0, 1, 2, 4, 5, 4, 3, 2, 1, 0 };
            notes = { scale[(size_t) std::min ((int) scale.size() - 1, contour[step] + 2)] };
            break;
        }
        case Lane::octave:
        {
            const int r = 52 + mod12 (chord.root - 4);
            notes = { r, r + 12 };
            break;
        }
        case Lane::power:
        {
            const int r = bassRoot (chord);
            notes = { r, r + 7, r + 12 };
            break;
        }
        case Lane::root:
        default:
            notes = { bassRoot (chord) };
            break;
    }

    for (auto& n : notes)
        n = std::clamp (n + 12 * std::clamp (octave, -3, 3), 0, 127);
    return notes;
}

//==============================================================================
const std::vector<Progression>& progressions()
{
    using T = ChordType;
    static const std::vector<Progression> list {
        { "i7 - iv7 (minor vamp)",           "Deep",              { 0, 5, 0, 5 },    { T::m7, T::m7, T::m7, T::m7 } },
        { "im9 - bVImaj9 - bVII6/9 - im9",   "Deep",              { 0, 8, 10, 0 },   { T::m9, T::maj9, T::sixNine, T::m9 } },
        { "ii9 - V13 - Imaj9 - vi9",         "Soulful",           { 2, 7, 0, 9 },    { T::m9, T::dom13, T::maj9, T::m9 } },
        { "IVmaj7 - V7 - iii7 - vi7",        "Gospel",            { 5, 7, 4, 9 },    { T::maj7, T::dom7, T::m7, T::m7 } },
        { "vi7 - IVmaj7 - I - V",            "Piano House",       { 9, 5, 0, 7 },    { T::m7, T::maj7, T::major, T::major } },
        { "i7 - IV9 (Dorian disco)",         "Funky & Disco",     { 0, 5, 0, 5 },    { T::m7, T::dom9, T::m7, T::dom9 } },
        { "I9 (one-chord funk)",             "Funky & Disco",     { 0, 0, 0, 0 },    { T::dom9, T::dom9, T::dom9, T::dom9 } },
        { "i7 - i7 - bVII7 - bVII7",         "Jackin'",           { 0, 0, 10, 10 },  { T::m7, T::m7, T::dom7, T::dom7 } },
        { "im9 - bVIImaj9 - bVImaj9 - v7",   "Garage & UK",       { 0, 10, 8, 7 },   { T::m9, T::maj9, T::maj9, T::m7 } },
        { "i7 - i7 - ii7 - i7 (modal)",      "Afro & Latin",      { 0, 0, 2, 0 },    { T::m7, T::m7, T::m7, T::m7 } },
        { "vi - IV - I - V",                 "'90s Piano House",  { 9, 5, 0, 7 },    { T::minor, T::major, T::major, T::major } },
        { "IVmaj9 - iii7 - ii9 - V13",       "Neo Soul & Broken", { 5, 4, 2, 7 },    { T::maj9, T::m7, T::m9, T::dom13 } },
        { "Imaj7 - IVmaj7 - Imaj9 - IVmaj7", "Chill & Pads",      { 0, 5, 0, 5 },    { T::maj7, T::maj7, T::maj9, T::maj7 } },
        { "i - bVI - bIII - bVII",           "Dance Stabs",       { 0, 8, 3, 10 },   { T::minor, T::major, T::major, T::major } },
        { "I7sus4 - I7 - I7sus4 - I9",       "Funky & Disco",     { 0, 0, 0, 0 },    { T::sus4, T::dom7, T::sus4, T::dom9 } },
        { "i7 - IV7 (jack bounce)",          "Jackin' Piano",     { 0, 5, 0, 5 },    { T::m7, T::dom7, T::m7, T::dom7 } },
        { "im11 - im11 - bVIImaj9 - ivm9",   "Afro & Latin",      { 0, 0, 10, 5 },   { T::m11, T::m11, T::maj9, T::m9 } },
        { "Imaj9 - vi9 - ii9 - V7sus4",      "Soulful",           { 0, 9, 2, 7 },    { T::maj9, T::m9, T::m9, T::sus4 } },
    };
    return list;
}

std::array<Chord, 4> progressionChords (int progression, int key)
{
    const auto& list = progressions();
    const auto& p = list[(size_t) std::clamp (progression, 0, (int) list.size() - 1)];
    std::array<Chord, 4> chords;
    for (size_t i = 0; i < 4; ++i)
        chords[i] = { mod12 (key + p.degree[i]), (int) p.type[i] };
    return chords;
}

//==============================================================================
void normalisePattern (GuitarPattern& p, int minimumLength)
{
    size_t length = (size_t) std::max (1, minimumLength);
    for (const auto& line : p.lanes)
        length = std::max (length, line.size());

    for (auto& line : p.lanes)
    {
        for (auto& c : line)
            if (! isHit (c))
                c = '.';

        if (line.empty())
            line.assign (length, '.');

        // A one-bar line in a two-bar groove plays twice.
        const std::string base = line;
        while (line.size() < length)
            line += base.substr (0, std::min (base.size(), length - line.size()));
    }
}

void fitPattern (GuitarPattern& p, int length)
{
    length = std::max (1, length);
    for (auto& line : p.lanes)
    {
        for (auto& c : line)
            if (! isHit (c))
                c = '.';
        if (line.empty())
            line.assign ((size_t) length, '.');
        const std::string base = line;
        while ((int) line.size() < length)
            line += base.substr (0, std::min (base.size(), (size_t) length - line.size()));
        line.resize ((size_t) length);
    }
}

namespace
{
    std::string compact (const char* s)
    {
        std::string out;
        for (; *s != 0; ++s)
            if (*s != ' ')
                out += *s;
        return out;
    }

    struct Line { Lane lane; const char* steps; };

    GuitarPattern groove (const char* name, const char* category, std::initializer_list<Line> lines)
    {
        GuitarPattern p;
        p.name = name;
        p.category = category;
        for (const auto& l : lines)
            p[l.lane] = compact (l.steps);
        normalisePattern (p);
        return p;
    }

    constexpr auto DN = Lane::strumDown, UP = Lane::strumUp, ST = Lane::stab, CK = Lane::chank, SC = Lane::scratch,
                   SL = Lane::slide, SW = Lane::swell, AR = Lane::arp, LK = Lane::lick, OC = Lane::octave,
                   PW = Lane::power, RT = Lane::root;

    // The muted scratch fills the 16ths no other chord hit plays (the engine leaves it out
    // where one does): a funk player's hand never stops.
    constexpr const char* allScratch = "gggg gggg gggg gggg";
    constexpr const char* offbeats   = "..X. ..X. ..X. ..X.";
}

const std::vector<GuitarPattern>& guitarPatterns()
{
    static const std::vector<GuitarPattern> patterns {
        // ---- Piano House: bright offbeat chords and an open strum ----
        groove ("Offbeat Chords", "Piano House",
                { { ST, offbeats }, { SC, "g... g... g... g.g." } }),
        groove ("House Strummer", "Piano House",
                { { DN, "X... ..x. X... x.x." }, { UP, "...x .x.. ...x ...x" }, { SC, ".g.. g... .g.. .g.." } }),
        groove ("Piano House Chank", "Piano House",
                { { CK, "..x. ..x. ..xx ..x." }, { ST, "X... .... .... ...." }, { RT, "X... .... X... ...." } }),
        groove ("Uplifting Chords", "Piano House",
                { { DN, "X... .... X... ...." }, { UP, "..x. ..x. ..x. .xx." }, { SC, ".... g... .... g..." } }),

        // ---- Soulful: warm strums and chanks under the vocal ----
        groove ("Soulful Sunday", "Soulful",
                { { DN, "X... ..x. .... x..." }, { UP, "...x .... .x.. ...x" }, { SC, "..g. g... g.g. ..g." } }),
        groove ("Soul Chank & Root", "Soulful",
                { { CK, "..x. ..x. ..x. ..x." }, { RT, "X... .... X..x ...." }, { SC, ".g.. .g.. .g.. .g.g" } }),
        groove ("Soulful Sixteenths", "Soulful",
                { { DN, "X.g. x.g. X.g. x.g." }, { UP, ".g.x .g.x .g.x .x.x" } }),
        groove ("Sunday Arp", "Soulful",
                { { AR, "x.xx x.xx x.xx x.xx" }, { SW, "X... .... .... ...." } }),

        // ---- Deep: dub chords that echo, swells and soft picking ----
        groove ("Deep Swell", "Deep",
                { { SW, "X... .... .... ...." }, { CK, ".... ..x. .... ..g." } }),
        groove ("Deep Chord Echo", "Deep",
                { { ST, "..X. .... ..x. ...g" }, { SC, ".... .... .... g..." } }),
        groove ("Deep Arp", "Deep",
                { { AR, "x.x. x.x. x.x. x.x." }, { SW, "X... .... .... .... .... .... .... ...." } }),
        groove ("Late Night Stab", "Deep",
                { { ST, "...X .... ..x. .... ...X .... .... ..x." }, { RT, "X... .... .... .... X... .... .... ...." } }),

        // ---- Gospel: big eighth-note strums and shouts ----
        groove ("Gospel Strum", "Gospel",
                { { DN, "X... X... X... X..." }, { UP, "..x. ..x. ..x. ..x." } }),
        groove ("Gospel Shout", "Gospel",
                { { ST, "X..x ..x. X..x ..x." }, { RT, "X... .... X... ...." } }),
        groove ("Church Arp", "Gospel",
                { { AR, "x.xx x.xx x.xx x.xx" }, { SW, "X... .... .... ...." } }),

        // ---- Jackin': raw stabs and chanks with the swing ----
        groove ("Jack Stab", "Jackin'",
                { { ST, offbeats }, { SC, allScratch } }),
        groove ("Chicago Chank", "Jackin'",
                { { CK, "x.xx .x.x x.xx .x.x" }, { ST, ".... .... .... ..X." } }),
        groove ("Jack Octaves", "Jackin'",
                { { OC, "x..x ..x. x..x ..x." }, { ST, "..X. .... ..X. ...." } }),

        // ---- Jackin' Piano: bouncy stabs ----
        groove ("Bouncy Stabs", "Jackin' Piano",
                { { ST, "X..X ..X. .X.. X.X." }, { SC, ".g.. g... g.g. .g.." } }),
        groove ("Jack Strum", "Jackin' Piano",
                { { DN, "X... X.x. X... X.x." }, { UP, "..x. ...x ..x. ...x" } }),
        groove ("Pumping Chords", "Jackin' Piano",
                { { ST, offbeats }, { PW, "X... X... X... X..." } }),

        // ---- Garage & UK: two-step chops and skanks ----
        groove ("Two-Step Chops", "Garage & UK",
                { { ST, "X... ..x. ...x ...." }, { SC, "..g. g... g... g.g." } }),
        groove ("Garage Skank", "Garage & UK",
                { { CK, "..x. ..x. .x.. ..x." }, { SL, "...X .... .... ...." } }),
        groove ("UK Swell & Stab", "Garage & UK",
                { { SW, "X... .... .... ...." }, { ST, ".... ..X. .... ...x" } }),

        // ---- Afro & Latin: highlife picking, montunos and clave chanks ----
        groove ("Afro Highlife", "Afro & Latin",
                { { AR, "x.x. xx.x x.x. xx.x" }, { RT, "X... .... X... ...." } }),
        groove ("Afro Chank", "Afro & Latin",
                { { CK, "x..x ..x. ..x. x..." }, { RT, "X... .... X... ...." }, { SC, ".g.. g..g .g.. .g.g" } }),
        groove ("Latin Montuno", "Afro & Latin",
                { { AR, "x.xx .xx. x.xx .xx." }, { OC, "X... .... .... ...." } }),

        // ---- Funky & Disco: the chicken scratch, octaves, sixteenths ----
        groove ("Nile Scratch", "Funky & Disco",
                { { DN, "X... ..x. ..x. x..." }, { UP, ".x.x ...x .x.. .x.x" }, { SC, "..g. g... g... ..g." } }),
        groove ("Disco Octaves", "Funky & Disco",
                { { OC, "X.x. x.x. X.x. x.x." }, { SC, ".g.g .g.g .g.g .g.g" } }),
        groove ("Funk Sixteenths", "Funky & Disco",
                { { DN, "X... .... X... ...." }, { UP, ".... ..x. .... ..x." }, { SC, allScratch } }),
        groove ("Disco Chank Line", "Funky & Disco",
                { { CK, "x.xx .xx. x.xx .x.x" }, { ST, ".... X... .... X..." } }),

        // ---- '90s Piano House: strummed anthems and power chords ----
        groove ("90s Anthem Strum", "'90s Piano House",
                { { DN, "X... ..x. ..x. X..." }, { UP, ".... x... .... ..x." }, { RT, "X... X... X... X..." } }),
        groove ("Rave Power", "'90s Piano House",
                { { PW, "X... ..X. ..X. X..." }, { ST, ".... .... .... ..x." } }),
        groove ("Italo Offbeat", "'90s Piano House",
                { { ST, offbeats }, { OC, "X... X... X... X..." } }),

        // ---- Neo Soul & Broken: lazy chords and licks ----
        groove ("Neo Soul Lazy", "Neo Soul & Broken",
                { { ST, "X... ...x ..x. ...." }, { LK, ".... ..x. .... .x.x" } }),
        groove ("Broken Strum", "Neo Soul & Broken",
                { { DN, "X... ..x. .... ...." }, { UP, ".... .... .x.. ...." }, { LK, ".... .... .... x.x." },
                  { SL, ".... .... .... .... X... ..x. .... ...." } }),
        groove ("Neo Arp", "Neo Soul & Broken",
                { { AR, "x.x. .x.x x.x. .x.x" }, { RT, "X... .... ..x. ...." } }),

        // ---- Chill & Pads: swells and soft picking ----
        groove ("Chill Swell", "Chill & Pads",
                { { SW, "X... .... .... .... X... .... .... ...." }, { LK, ".... .... ..x. .... .... ..x. .x.. x.x." } }),
        groove ("Chill Picking", "Chill & Pads",
                { { AR, "x.x. x.x. x.x. x.x." }, { SW, "X... .... .... ...." } }),
        groove ("Sunset Chords", "Chill & Pads",
                { { DN, "f... .... ..x. ...." }, { UP, ".... ..x. .... .x.." } }),

        // ---- Dance Stabs: big straight club hits ----
        groove ("Dance Power", "Dance Stabs",
                { { PW, "X.x. X.x. X.x. X.x." } }),
        groove ("Big Stabs", "Dance Stabs",
                { { ST, "X... ..X. ..X. X..." }, { SC, ".g.. g... g... .g.." } }),
        groove ("Offbeat Power", "Dance Stabs",
                { { PW, offbeats }, { RT, "X... X... X... X..." } }),
    };
    return patterns;
}

const std::vector<const char*>& grooveCategories()
{
    static const std::vector<const char*> categories { "Piano House", "Soulful", "Deep", "Gospel", "Jackin'", "Jackin' Piano", "Garage & UK",
                                                       "Afro & Latin", "Funky & Disco", "'90s Piano House", "Neo Soul & Broken",
                                                       "Chill & Pads", "Dance Stabs" };
    return categories;
}

int categoryIndex (const std::string& category) noexcept
{
    const auto& c = grooveCategories();
    for (size_t i = 0; i < c.size(); ++i)
        if (category == c[i])
            return (int) i;
    return -1;
}

float genreSwing (int category) noexcept
{
    // The same swing per genre as Fase Groove Drums, so the guitar sits in the drums' pocket.
    static const float swing[] { 0.45f, 0.2f, 0.5f, 0.3f, 0.8f, 0.6f, 0.75f, 0.35f, 0.3f, 0.45f, 0.9f, 0.3f, 0.1f };
    return category >= 0 && category < (int) std::size (swing) ? swing[category] : 0.2f;
}

int genreProgression (int category) noexcept
{
    static const int progression[] { 4, 2, 1, 3, 7, 15, 8, 9, 5, 10, 11, 12, 13 };
    return category >= 0 && category < (int) std::size (progression) ? progression[category] : 0;
}

int randomPatternIndex() noexcept
{
    return (int) guitarPatterns().size();
}

int userPatternIndex() noexcept
{
    return randomPatternIndex() + 1;
}

} // namespace fgg

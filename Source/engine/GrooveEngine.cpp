// Fase Groove Guitar - Copyright (C) 2026 Fabio 'Pitch' Serra
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "GrooveEngine.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace fgg
{

namespace
{
    long long floorDiv (long long a, long long b) noexcept
    {
        long long q = a / b;
        if ((a % b != 0) && ((a < 0) != (b < 0))) --q;
        return q;
    }

    int mod (long long a, long long m) noexcept { return (int) (((a % m) + m) % m); }

    /** Deterministic 0..1 noise, so re-rendering a step always gives the same feel. */
    double noise (long long step, int salt) noexcept
    {
        auto x = (std::uint64_t) (step * 0x9E3779B97F4A7C15LL) ^ ((std::uint64_t) salt * 0xBF58476D1CE4E5B9ULL);
        x ^= x >> 31; x *= 0x94D049BB133111EBULL; x ^= x >> 29;
        return (double) (x >> 11) * (1.0 / 9007199254740992.0);
    }

    double symbolLevel (char symbol) noexcept
    {
        switch (symbol)
        {
            case 'X': return 1.0;
            case 'f': return 0.9;
            case 'g': return 0.42;
            case 'x':
            default:  return 0.8;
        }
    }

    /** How much a lane drifts with Humanize: the low strings stay with the kick, the strumming
        hand drifts more. */
    double laneLooseness (int lane) noexcept
    {
        switch ((Lane) lane)
        {
            case Lane::root:
            case Lane::power:     return 0.3;
            case Lane::strumDown:
            case Lane::stab:      return 0.6;
            case Lane::scratch:
            case Lane::strumUp:
            case Lane::chank:     return 1.1;
            default:              return 0.9;
        }
    }

    /** How long a hit rings, in 16ths. */
    double noteLength (int lane) noexcept
    {
        switch ((Lane) lane)
        {
            case Lane::strumDown: return 1.8;
            case Lane::strumUp:   return 0.9;
            case Lane::stab:      return 0.45;
            case Lane::chank:     return 0.4;
            case Lane::scratch:   return 0.22;
            case Lane::slide:     return 1.6;
            case Lane::swell:     return 15.5;
            case Lane::arp:       return 1.0;
            case Lane::lick:      return 0.9;
            case Lane::octave:    return 0.85;
            case Lane::power:     return 1.6;
            case Lane::root:
            default:              return 1.8;
        }
    }

    /** A lane's own loudness: the muted scratch is a whisper under the chords. */
    double laneLevel (int lane) noexcept
    {
        switch ((Lane) lane)
        {
            case Lane::scratch: return 0.72;
            case Lane::swell:   return 0.8;
            case Lane::strumUp: return 0.88;
            default:            return 1.0;
        }
    }

    int lcm (int a, int b) noexcept { return a / std::gcd (a, b) * b; }

    /** The level an up-strum, a scratch or a chank has at each 16th of the beat, as funk
        guitarists play it: the offbeat 8th sings, the 16ths around it are softer. */
    double accentProfile (int lane, int stepInBeat) noexcept
    {
        static constexpr double strums[] { 0.75, 0.45, 1.0, 0.55 };
        static constexpr double picking[] { 0.9, 0.6, 1.0, 0.7 };
        return (lane == (int) Lane::arp ? picking : strums)[stepInBeat & 3];
    }

    /** Whether a lane keeps playing the groove under a fill. */
    bool fillKeeps (FillStyle style, int lane, const std::array<bool, numLanes>& customLanes) noexcept
    {
        const auto l = (Lane) lane;
        const bool low = l == Lane::root || l == Lane::power;
        switch (style)
        {
            case FillStyle::dropOut:
            case FillStyle::swellOut:    return false;
            case FillStyle::bassWalk:    return isChordLane (lane);
            case FillStyle::chankLift:   return low || l == Lane::scratch;
            case FillStyle::lickRun:
            case FillStyle::arpRun:      return low || l == Lane::scratch;
            case FillStyle::custom:      return ! customLanes[(size_t) lane];
            default:                     return low;   // the low strings keep the floor
        }
    }

    bool followsAccentGroove (int lane) noexcept
    {
        switch ((Lane) lane)
        {
            case Lane::strumUp: case Lane::scratch: case Lane::chank: case Lane::arp: return true;
            default: return false;
        }
    }

    /** The strings of a hit sound one after the other, as the pick crosses them: low to high on
        a down strum, high to low on an up strum and a scratch. One string: no strum. */
    bool strumsUp (int lane, int stepInBar) noexcept
    {
        if (lane == (int) Lane::strumUp)
            return true;
        if (lane == (int) Lane::scratch || lane == (int) Lane::chank)
            return stepInBar % 2 == 1;   // the hand goes up on the odd 16ths
        return false;
    }
}

GuitarSettings::GuitarSettings() noexcept
{
    for (int l = 0; l < numLanes; ++l)
    {
        levels[(size_t) l] = 1.0f;
        laneSwing[(size_t) l] = 1.0f;
        user[(size_t) l].fill ('.');
        userVelocity[(size_t) l].fill (0);
        userChance[(size_t) l].fill (0);
        userRoll[(size_t) l].fill (0);
        customFill[(size_t) l].fill ('.');
    }
    const auto first = progressionChords (genreProgression (0), 9);
    numChords = (int) first.size();
    for (size_t i = 0; i < chords.size(); ++i)
        chords[i] = first[i % first.size()];
    grooveTemplate.level.fill (1.0f);
}

//==============================================================================
GrooveEngine::GrooveEngine()
{
    offTime.fill (-1.0);
    laneOf.fill (0);
    pending.reserve (1024);
    scratch.reserve (128);
}

ChordTones GrooveEngine::chordAt (const GuitarSettings& s, long long bar) noexcept
{
    if (s.useLiveChord && s.liveChord.mask != 0)
        return s.liveChord;
    return tonesOf (s.chords[(size_t) mod (bar, std::clamp (s.numChords, 1, maxChords))]);
}

GuitarPattern GrooveEngine::basePattern (const GuitarSettings& s)
{
    const auto& presets = guitarPatterns();

    const int length = patternLength (s);

    if (s.pattern == randomPatternIndex())
    {
        auto p = makeRandomPattern (s.riffSeed, s.density, s.randomGenre);
        fitPattern (p, length);
        if (length > randomBeatLength)   // four bars: the last one turns around
            p = makeVariation (p, s.riffSeed + 31u);
        return p;
    }

    if (s.pattern == userPatternIndex())
    {
        GuitarPattern p;
        p.name = "User pattern";
        for (int l = 0; l < numLanes; ++l)
            p.lanes[(size_t) l].assign (s.user[(size_t) l].begin(), s.user[(size_t) l].end());
        fitPattern (p, length);
        return p;
    }

    // Every groove plays as two (or four) bars, a one-bar groove repeated: A-D are always a
    // quarter of it.
    auto p = presets[(size_t) std::clamp (s.pattern, 0, (int) presets.size() - 1)];
    fitPattern (p, length);
    return p;
}

GuitarPattern GrooveEngine::resolvePattern (const GuitarSettings& s)
{
    auto p = applySegmentOrder (basePattern (s), s.segmentOrder);
    if (s.sectionVariation != 0)
        p = makeSectionVariation (p, s.sectionVariation);
    return applySection (p, s.section);
}

int GrooveEngine::patternLength (const GuitarSettings& s) noexcept
{
    return s.grooveBars >= 4 ? 64 : 32;
}

int GrooveEngine::loopLengthInSteps (const GuitarSettings& s) noexcept
{
    int steps = patternLength (s);
    if (! s.useLiveChord)
    {
        // The chords come round after numChords bars (or fewer, when they repeat inside).
        const int n = std::clamp (s.numChords, 1, maxChords);
        int chordBars = n;
        for (int period = 1; period < n; ++period)
            if (n % period == 0)
            {
                bool repeats = true;
                for (int i = period; i < n && repeats; ++i)
                    repeats = s.chords[(size_t) i] == s.chords[(size_t) (i % period)];
                if (repeats)
                {
                    chordBars = period;
                    break;
                }
            }
        steps = lcm (steps, chordBars * 16);
    }
    if (s.variationEvery > 0)
        steps = lcm (steps, s.variationEvery * 16);
    if (s.fillEveryBars > 0)
        steps = lcm (steps, s.fillEveryBars * 16);
    return steps;
}

FillStyle GrooveEngine::effectiveFillStyle (FillStyle style, long long bar) noexcept
{
    if (style != FillStyle::mix)
        return style;

    // Mix: a different fill now and then, always the same one on the same bar.
    static constexpr FillStyle choices[] { FillStyle::strumBuild, FillStyle::slideUp, FillStyle::scratchBuild, FillStyle::chankLift,
                                           FillStyle::strumBuild, FillStyle::lickRun, FillStyle::stabStutter, FillStyle::octaveRun };
    return choices[(size_t) (noise (bar, 4242) * (double) std::size (choices)) % std::size (choices)];
}

const GuitarPattern& GrooveEngine::patternFor (const GuitarSettings& s)
{
    const bool usesSeed = s.pattern == randomPatternIndex();
    const bool usesUser = s.pattern == userPatternIndex();

    if (s.pattern != cachedIndex || s.section != cachedSection || s.segmentOrder != cachedOrder
        || s.sectionVariation != cachedSectionVariation || s.grooveBars != cachedBars
        || (usesSeed && (s.riffSeed != cachedSeed || s.density != cachedDensity || s.randomGenre != cachedGenre))
        || (usesUser && s.user != cachedUser))
    {
        cachedPattern = resolvePattern (s);
        cachedVariation = makeVariation (cachedPattern, (std::uint32_t) std::max (0, s.pattern) * 7919u + s.riffSeed);
        cachedIndex = s.pattern;
        cachedSection = s.section;
        cachedSeed = s.riffSeed;
        cachedDensity = s.density;
        cachedGenre = s.randomGenre;
        cachedUser = s.user;
        cachedOrder = s.segmentOrder;
        cachedSectionVariation = s.sectionVariation;
        cachedBars = s.grooveBars;
    }
    return cachedPattern;
}

//==============================================================================
void GrooveEngine::collectStep (long long step, const GuitarSettings& s, const GuitarPattern& normal, const GuitarPattern& variation)
{
    const long long bar = floorDiv (step, 16);
    const int stepInBar = mod (step, 16);
    const auto chord = chordAt (s, bar);

    // Every N bars the last bar plays a variation of the groove (the turnaround).
    const bool varied = s.variationEvery > 0 && mod (bar, s.variationEvery) == s.variationEvery - 1
                     && variation.length() == normal.length();
    const GuitarPattern& pattern = varied ? variation : normal;
    const int len = std::max (1, pattern.length());
    const int pos = mod (step, len);

    const auto& groove = s.grooveTemplate;
    const double swingDelay = groove.active ? (double) groove.timing[(size_t) stepInBar] * stepLength
                            : (step % 2 != 0) ? std::clamp (s.swing, 0.0f, 1.5f) * stepLength / 3.0 : 0.0;
    const double human = std::clamp (s.humanize, 0.0f, 1.0f);
    const int seedSalt = (int) (s.humanizeSeed % 100003u) * 7919;
    const double beatsPerMs = std::clamp (s.tempo, 20.0, 400.0) / 60000.0;

    // Fills: the end of every N bars (or a fill asked for on one bar).
    auto isFillBar = [&s] (long long b)
    {
        if (s.forcedFillBar >= 0)
            return b == s.forcedFillBar && s.forcedFillStyle >= 0;
        return s.fillEveryBars > 0 && sectionAllowsFills (s.section) && mod (b, s.fillEveryBars) == s.fillEveryBars - 1;
    };
    const bool fillBar = isFillBar (bar);
    const FillStyle style = (s.forcedFillBar >= 0 && bar == s.forcedFillBar && s.forcedFillStyle >= 0)
                                ? effectiveFillStyle ((FillStyle) std::clamp (s.forcedFillStyle, 0, numFillStyles - 1), bar)
                                : effectiveFillStyle (s.fillStyle, bar);
    // The custom fill takes over from its first written step.
    int fillStart = fillStartStep (style);
    std::array<bool, numLanes> customLanes {};
    if (style == FillStyle::custom)
    {
        fillStart = 16;
        for (int l = 0; l < numLanes; ++l)
            for (int i = 0; i < 16; ++i)
                if (isHit (s.customFill[(size_t) l][(size_t) i]))
                {
                    customLanes[(size_t) l] = true;
                    fillStart = std::min (fillStart, i);
                }
    }
    const bool inFill = fillBar && stepInBar >= fillStart;

    // A velocity set by hand on this step of the user pattern (stored per step as written,
    // before the A-D order). It stays on the variation bar and under a fill, except on the hits
    // they change. The same goes for the chance and the roll set on a step.
    auto userIndex = [&] (int lane) -> int
    {
        if (s.pattern != userPatternIndex() || len != patternLength (s))
            return -1;
        if (varied && variation.lanes[(size_t) lane][(size_t) pos] != normal.lanes[(size_t) lane][(size_t) pos])
            return -1;
        const int segment = len / numSegments;
        return std::clamp (s.segmentOrder[(size_t) (pos / segment)], 0, numSegments - 1) * segment + pos % segment;
    };
    auto handVelocity = [&] (int lane) -> int
    {
        const int index = userIndex (lane);
        return index < 0 ? 0 : s.userVelocity[(size_t) lane][(size_t) index];
    };

    auto velocityFor = [&] (int lane, double level, int salt)
    {
        double v = s.velocity * level * (double) s.levels[(size_t) lane] * laneLevel (lane) * sectionVelocityScale (s.section);
        if (s.section == Section::buildup)   // crescendo through the build-up
            v *= 0.65 + 0.45 * (double) pos / (double) std::max (1, len - 1);
        v += human * 12.0 * (noise (step, seedSalt + salt + 7) - 0.5) * 2.0;
        return std::clamp ((int) std::lround (v), 1, 127);
    };

    auto timeFor = [&] (int lane, int salt)
    {
        // Each lane takes its share of the swing and sits ahead of or behind the beat; humanize
        // only ever delays a hit a little (a player sits on or behind the grid).
        const double shift = std::clamp ((double) s.laneShift[(size_t) lane], -40.0, 40.0) * beatsPerMs;
        const double t = (double) step * stepLength + swingDelay * std::clamp ((double) s.laneSwing[(size_t) lane], 0.0, 2.0) + shift
                       + human * laneLooseness (lane) * 0.025 * noise (step, seedSalt + salt);
        return step >= 0 ? std::max (t, 0.0) : t;
    };

    // Life (with Humanize): over a four-bar phrase the soft notes come and go, as a guitarist
    // plays a loop: ghosts, the scratches and up-strums between the offbeats. The phrase repeats
    // (NEW FEEL makes another). The accents and the downbeats never change.
    const int phraseBar = mod (bar, 4);
    auto isSoft = [&] (int lane, char symbol)
    {
        if (symbol == 'X')
            return false;
        if (symbol == 'g')
            return true;
        return followsAccentGroove (lane) && stepInBar % 2 == 1;
    };
    auto lifeNoise = [&] (int lane, int salt) { return noise (phraseBar * 16 + stepInBar, seedSalt + lane * 101 + salt); };

    auto add = [&] (int lane, char symbol, int transpose = 0, double levelScale = 1.0, bool fromFill = false, int roll = 1)
    {
        if (s.muted[(size_t) lane] || ! isHit (symbol))
            return;
        if (! fromFill && human > 0.0 && isSoft (lane, symbol) && lifeNoise (lane, 77) < 0.4 * human)
            return;   // not in this bar of the phrase
        const int salt = lane * 13 + 1;
        if (! fromFill)
            if (const int index = userIndex (lane); index >= 0)
            {
                // A step that only plays now and then (the same way every time it comes round).
                const int chance = s.userChance[(size_t) lane][(size_t) index];
                if (chance > 0 && chance < 100 && noise (step, seedSalt + salt + 31) * 100.0 >= chance)
                    return;
                roll = std::max (roll, (int) s.userRoll[(size_t) lane][(size_t) index]);
            }
        const double t = timeFor (lane, salt);
        double level = symbolLevel (symbol) * levelScale;
        if (groove.active)
            level *= std::clamp ((double) groove.level[(size_t) stepInBar], 0.2, 1.6);
        if (const int set = fromFill ? 0 : handVelocity (lane); set > 0)
            level = set / 100.0;   // 100 = as the Velocity control
        else if (followsAccentGroove (lane) && symbol != 'g' && ! groove.active)
        {
            // The accent groove: written accents stay on top, the rest follows the beat.
            const double a = std::clamp ((double) s.accentGroove, 0.0, 1.0);
            const double shape = 1.0 - a * (1.0 - accentProfile (lane, stepInBar % 4));
            level *= symbol == 'X' ? std::max (shape, 0.9) : shape * (1.0 + 0.25 * a);
        }

        auto notes = laneNotes ((Lane) lane, chord, stepInBar, s.octaves[(size_t) lane]);
        for (auto& n : notes)
            n += transpose;
        const bool up = strumsUp (lane, stepInBar);
        if (up)
            std::reverse (notes.begin(), notes.end());
        // The pick crosses the strings: a few ms each, a rake ('f') slower and every string heard.
        const double gap = std::clamp ((double) s.strumMs, 0.0, 60.0) * beatsPerMs * (symbol == 'f' ? 2.6 : 1.0)
                         * (lane == (int) Lane::scratch || lane == (int) Lane::stab ? 0.5 : 1.0);
        const bool chordHit = isChordLane (lane);

        auto strum = [&] (double at, double length, double strumLevel, int extraSalt)
        {
            for (size_t i = 0; i < notes.size(); ++i)
            {
                // The strings struck last are a touch softer (the pick slows down).
                const double stringLevel = strumLevel * (1.0 - 0.05 * (double) i);
                pending.push_back ({ at + gap * (double) i, notes[i], velocityFor (lane, stringLevel, salt + extraSalt + (int) i),
                                     std::max (0.02, length - gap * (double) i), lane, chordHit && i == 0 });
            }
        };

        roll = std::clamp (roll, 1, 4);
        if (roll > 1)
        {
            // A roll: that many quick strums in the 16th, the first one the loudest.
            const double each = stepLength / roll;
            for (int k = 0; k < roll; ++k)
                strum (t + each * k, each * 0.9, level * (k == 0 ? 1.0 : 0.82), k * 17);
            return;
        }
        if (lane == (int) Lane::slide)   // a semitone below, slid up into the chord
            for (size_t i = 0; i < notes.size(); ++i)
                pending.push_back ({ t - 0.09 + gap * (double) i, notes[i] - 1, velocityFor (lane, 0.6 * level, salt + 5), 0.08, lane, i == 0 });
        strum (t, noteLength (lane) * stepLength, level, 0);
    };

    // ---- The groove as written (what a fill leaves of it) ----
    auto plays = [&] (int lane)
    {
        return isHit (pattern.lanes[(size_t) lane][(size_t) pos]) && ! s.muted[(size_t) lane]
            && ! (inFill && ! fillKeeps (style, lane, customLanes));
    };
    for (int lane = 0; lane < numLanes; ++lane)
    {
        char symbol = pattern.lanes[(size_t) lane][(size_t) pos];
        if (inFill && ! fillKeeps (style, lane, customLanes))
            continue;
        if (lane == (int) Lane::scratch && isHit (symbol))
        {
            // The scratch fills the gaps: where another chord hit plays, the hand plays that.
            bool covered = false;
            for (int other = 0; other < numLanes; ++other)
                covered = covered || (other != lane && isChordLane (other) && plays (other));
            if (covered)
                continue;
        }
        // ...and now and then a soft scratch that isn't written, just before the next bar.
        if (symbol == '.' && lane == (int) Lane::scratch && human > 0.0 && ! inFill && stepInBar >= 13 && stepInBar % 2 == 1
            && phraseBar % 2 == 1 && std::any_of (pattern.lanes[(size_t) lane].begin(), pattern.lanes[(size_t) lane].end(), isHit)
            && lifeNoise (lane, 91) < 0.3 * human)
        {
            bool free = true;
            for (int other = 0; other < numLanes; ++other)
                free = free && ! (isChordLane (other) && plays (other));
            if (free)
                symbol = 'g';
        }
        add (lane, symbol);
    }

    // ---- The fill ----
    if (inFill)
    {
        const int start = fillStart;
        const double ramp = 0.62 + 0.45 * (double) (stepInBar - start) / (double) std::max (1, 15 - start);
        const int i = stepInBar;
        const bool minor = ((chord.mask >> 3u) & 1u) != 0 && ((chord.mask >> 4u) & 1u) == 0;
        const int run = std::min (3, std::max (0, i - 12));   // the last four 16ths of a run

        switch (style)
        {
            case FillStyle::strumBuild:   // 16th strums, down and up, louder and louder
                add (i % 2 == 0 ? (int) Lane::strumDown : (int) Lane::strumUp, i >= 14 ? 'X' : 'x', 0, ramp, true);
                break;
            case FillStyle::octaveRun:    // octaves climbing the chord into the next bar
            {
                static constexpr int minorRun[] { 0, 3, 5, 7 }, majorRun[] { 0, 4, 5, 7 };
                add ((int) Lane::octave, i == 15 ? 'X' : 'x', (minor ? minorRun : majorRun)[run], ramp, true);
                break;
            }
            case FillStyle::scratchBuild:
                add ((int) Lane::scratch, i == 15 ? 'X' : 'x', 0, ramp + 0.2, true);
                break;
            case FillStyle::chankLift:
                add ((int) Lane::chank, i == 14 ? 'X' : 'x', 0, ramp, true);
                break;
            case FillStyle::dropOut:      // silence, then one stab on the very last 16th
                if (i == 15)
                    add ((int) Lane::stab, 'X', 0, 1.0, true);
                break;
            case FillStyle::slideUp:
                if (i == 14)
                    add ((int) Lane::slide, 'X', 0, 1.0, true);
                else if (i == 12)
                    add ((int) Lane::scratch, 'x', 0, 1.0, true);
                break;
            case FillStyle::stabStutter:
                if (i != 13)
                    add ((int) Lane::stab, i == 15 ? 'X' : 'x', 0, ramp, true, i >= 14 ? 2 : 1);
                break;
            case FillStyle::lickRun:
                add ((int) Lane::lick, i == 15 ? 'X' : 'x', 0, ramp, true);
                break;
            case FillStyle::arpRun:
                add ((int) Lane::arp, i % 4 == 0 ? 'X' : 'x', 0, ramp, true);
                break;
            case FillStyle::bassWalk:     // the low string walks up to the next bar
            {
                static constexpr int minorWalk[] { 0, 2, 3, 5 }, majorWalk[] { 0, 2, 4, 5 };
                add ((int) Lane::root, i == 15 ? 'X' : 'x', (minor ? minorWalk : majorWalk)[run], ramp, true);
                break;
            }
            case FillStyle::swellOut:     // half a bar held, then nothing
                if (i == 8)
                    add ((int) Lane::swell, 'X', 0, 1.0, true);
                break;
            case FillStyle::custom:
                for (int l = 0; l < numLanes; ++l)
                    add (l, s.customFill[(size_t) l][(size_t) i], 0, 1.0, true);
                break;
            case FillStyle::mix:
            default:
                break;
        }
    }

    // ---- A big strum: after a fill, and every 8 bars of the drop ----
    if (stepInBar == 0 && s.strumAfterFill && pattern.lanes[(size_t) Lane::strumDown][(size_t) pos] == '.')
    {
        const bool afterFill = isFillBar (bar - 1);
        const bool dropPhrase = s.section == Section::drop && mod (bar - s.sectionStartBar, 8) == 0;
        if (afterFill || dropPhrase)
            add ((int) Lane::strumDown, 'X', 0, 1.0, true);
    }
}

//==============================================================================
void GrooveEngine::flushOffsUpTo (double ppq, bool inclusive, std::vector<NoteEvent>& out)
{
    scratch.clear();
    for (int n = 0; n < 128; ++n)
    {
        const double off = offTime[(size_t) n];
        if (off >= 0.0 && (off < ppq || (inclusive && off <= ppq)))
            scratch.push_back (n);
    }

    std::sort (scratch.begin(), scratch.end(),
               [this] (int a, int b) { return offTime[(size_t) a] < offTime[(size_t) b]; });

    for (int n : scratch)
    {
        out.push_back ({ offTime[(size_t) n], n, 0, laneOf[(size_t) n] });
        offTime[(size_t) n] = -1.0;
    }
}

void GrooveEngine::render (double startPpq, double endPpq, const GuitarSettings& s, std::vector<NoteEvent>& out)
{
    if (endPpq <= startPpq)
        return;

    const auto& pattern = patternFor (s);
    pending.clear();

    // Swing, a template, a lane pulled behind, humanize and the strum delay a note (by < 3
    // steps), so look back a little; a lane pushed ahead and a slide's grace notes come before
    // their step, so look ahead too.
    const auto firstStep = (long long) std::floor (startPpq / stepLength) - 3;
    const auto lastStep  = (long long) std::floor (endPpq / stepLength) + 2;
    for (auto step = firstStep; step <= lastStep; ++step)
        collectStep (step, s, pattern, cachedVariation);

    pending.erase (std::remove_if (pending.begin(), pending.end(),
                                   [&] (const PendingOn& p) { return p.ppq < startPpq || p.ppq >= endPpq || p.note < 0 || p.note > 127; }),
                   pending.end());
    std::stable_sort (pending.begin(), pending.end(), [] (const PendingOn& a, const PendingOn& b) { return a.ppq < b.ppq; });

    for (const auto& p : pending)
    {
        flushOffsUpTo (p.ppq, true, out);

        // One guitar, six strings: a new chord stops the chord still ringing.
        if (p.choke)
            for (int n = 0; n < 128; ++n)
                if (offTime[(size_t) n] >= 0.0 && isChordLane (laneOf[(size_t) n]))
                    offTime[(size_t) n] = p.ppq;
        flushOffsUpTo (p.ppq, true, out);

        auto& off = offTime[(size_t) p.note];
        if (off >= 0.0)
            out.push_back ({ p.ppq, p.note, 0, laneOf[(size_t) p.note] });   // the same string again

        out.push_back ({ p.ppq, p.note, p.velocity, p.lane });
        off = p.ppq + p.duration;
        laneOf[(size_t) p.note] = p.lane;
    }

    flushOffsUpTo (endPpq, false, out);
}

void GrooveEngine::allNotesOff (double ppq, std::vector<NoteEvent>& out)
{
    for (int n = 0; n < 128; ++n)
        if (offTime[(size_t) n] >= 0.0)
        {
            out.push_back ({ ppq, n, 0, laneOf[(size_t) n] });
            offTime[(size_t) n] = -1.0;
        }
}

bool GrooveEngine::anyNoteSounding() const noexcept
{
    return std::any_of (offTime.begin(), offTime.end(), [] (double t) { return t >= 0.0; });
}

//==============================================================================
std::vector<NoteEvent> GrooveEngine::renderClip (const GuitarSettings& s, double lengthInBeats)
{
    GrooveEngine engine;
    std::vector<NoteEvent> events;
    engine.render (0.0, lengthInBeats, s, events);
    engine.allNotesOff (lengthInBeats, events);
    return events;
}

std::vector<NoteEvent> GrooveEngine::renderArrangement (const GuitarSettings& s, const std::vector<ArrangementPart>& parts,
                                                        std::vector<Marker>* markers)
{
    GrooveEngine engine;
    std::vector<NoteEvent> events;
    long long bar = 0;

    for (const auto& part : parts)
    {
        if (part.bars <= 0)
            continue;

        auto settings = s;
        settings.section = part.section;
        settings.sectionStartBar = bar;
        settings.sectionVariation = part.variation;
        settings.forcedFillBar = -1;
        if (part.endFill != -1)
        {
            // Its own fill on its last bar (or none at all).
            settings.forcedFillBar = bar + part.bars - 1;
            settings.forcedFillStyle = part.endFill;
        }

        const double start = (double) bar * 4.0, end = (double) (bar + part.bars) * 4.0;
        if (markers != nullptr)
            markers->push_back ({ start, sectionName ((int) part.section) });

        engine.render (start, end, settings, events);
        engine.allNotesOff (end, events);
        bar += part.bars;
    }
    return events;
}

} // namespace fgg

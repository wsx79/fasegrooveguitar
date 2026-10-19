// Fase Groove Guitar - Copyright (C) 2026 Fabio 'Pitch' Serra
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Plug-in level tests (they need JUCE, unlike EngineTests): project state, grid editing,
// chords, styles, exported MIDI and audio files, DAW transport, the MIDI chord input and the
// guide guitar.

#include <array>
#include "../Source/PluginProcessor.h"
#include "../Source/Localisation.h"
#include "../Source/PluginEditor.h"
#include "../Source/Updater.h"
#include "../Source/GuideGuitar.h"

#include <cstdio>
#include <set>

namespace
{
    int failures = 0;

    #define CHECK(cond) \
        do { if (! (cond)) { ++failures; std::printf ("FAILED %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (false)

    struct PlayHead : juce::AudioPlayHead
    {
        double ppq = 0.0, bpm = 124.0;
        bool playing = true;
        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo p;
            p.setBpm (bpm);
            p.setPpqPosition (ppq);
            p.setIsPlaying (playing);
            return p;
        }
    };

    float value (FaseGrooveGuitarProcessor& p, const juce::String& id)
    {
        return p.apvts.getRawParameterValue (id)->load();
    }

    int countNoteOns (const juce::MidiFile& file, int channel = -1)
    {
        int count = 0;
        for (int t = 0; t < file.getNumTracks(); ++t)
            for (const auto* e : *file.getTrack (t))
                if (e->message.isNoteOn() && (channel < 0 || e->message.getChannel() == channel))
                    ++count;
        return count;
    }

    bool isChordTone (const fgg::Chord& chord, int note)
    {
        const auto t = fgg::tonesOf (chord);
        return (t.mask >> (unsigned) (((note - t.root) % 12 + 12) % 12)) & 1u;
    }

    /** Every note of a file clean, for any instrument: each note-on closed by its note-off on
        the same track, later than it, no pitch struck again while still sounding, nothing left open. */
    bool notesAreClean (const juce::MidiFile& file)
    {
        for (int t = 0; t < file.getNumTracks(); ++t)
        {
            std::array<double, 128> openAt;
            openAt.fill (-1.0);
            for (const auto* e : *file.getTrack (t))
            {
                const auto& m = e->message;
                if (m.isNoteOn())
                {
                    if (openAt[(size_t) m.getNoteNumber()] >= 0.0)
                    {
                        std::printf ("  note %d struck again at %.0f (open since %.0f)\n", m.getNoteNumber(), m.getTimeStamp(), openAt[(size_t) m.getNoteNumber()]);
                        return false;   // struck again while sounding
                    }
                    openAt[(size_t) m.getNoteNumber()] = m.getTimeStamp();
                }
                else if (m.isNoteOff())
                {
                    const double start = openAt[(size_t) m.getNoteNumber()];
                    if (start < 0.0 || m.getTimeStamp() <= start)
                    {
                        std::printf ("  note %d off at %.0f (open since %.0f)\n", m.getNoteNumber(), m.getTimeStamp(), start);
                        for (const auto* x : *file.getTrack (t))
                            if (x->message.isNoteOnOrOff() && x->message.getNoteNumber() == m.getNoteNumber() && std::abs (x->message.getTimeStamp() - m.getTimeStamp()) < 400)
                                std::printf ("    %s %.0f\n", x->message.isNoteOn() ? "on " : "off", x->message.getTimeStamp());
                        return false;
                    }   // an off with nothing open, or a zero-length note
                    openAt[(size_t) m.getNoteNumber()] = -1.0;
                }
            }
            for (double o : openAt)
                if (o >= 0.0)
                    return false;   // left hanging at the end
        }
        return true;
    }

    //==============================================================================
    /** The family's compatibility checks on every preset groove: clean MIDI, one track and the
        tracks per lane with the same notes, the same seed the same clip, the dice another one. */
    void testGrooveCompatibility()
    {
        FaseGrooveGuitarProcessor p;
        p.setParameterFromUI (ParamIDs::tempoSync, 0.0f);
        for (size_t i = 0; i < fgg::guitarPatterns().size(); ++i)
        {
            p.selectGroove ((int) i);
            const auto one = p.createMidiClip (4, FaseGrooveGuitarProcessor::ClipPart::all);
            const auto tracks = p.createMidiClip (4, FaseGrooveGuitarProcessor::ClipPart::tracks);
            CHECK (notesAreClean (one));
            CHECK (notesAreClean (tracks));
            CHECK (countNoteOns (one) > 0);
            CHECK (countNoteOns (tracks) >= countNoteOns (one));   // (two lanes on one note at once: one note on a single track)
            // The same settings: the same clip.
            juce::MemoryOutputStream a, b;
            one.writeTo (a);
            p.createMidiClip (4, FaseGrooveGuitarProcessor::ClipPart::all).writeTo (b);
            CHECK (a.getMemoryBlock() == b.getMemoryBlock());
        }
        // The whole song too, with the clip's end on a section boundary.
        const auto song = p.createMidiClip (FaseGrooveGuitarProcessor::songArrangement, FaseGrooveGuitarProcessor::ClipPart::all);
        CHECK (notesAreClean (song));
        CHECK (notesAreClean (p.createMidiClip (FaseGrooveGuitarProcessor::songArrangement, FaseGrooveGuitarProcessor::ClipPart::tracks)));
    }

    //==============================================================================
    void testStateRoundTrip()
    {
        FaseGrooveGuitarProcessor a;
        a.setParameterFromUI (ParamIDs::swing, 0.4f);
        a.setParameterFromUI (ParamIDs::kit, 2.0f);
        a.setParameterFromUI (ParamIDs::octave (3), 1.0f);
        a.setChord (2, { 4, (int) fgg::ChordType::dom9 });
        a.editStep (1, 3, 'f');   // turns the groove into the user pattern
        a.editorScale = 1.3f;

        juce::MemoryBlock data;
        a.getStateInformation (data);

        FaseGrooveGuitarProcessor b;
        b.setStateInformation (data.getData(), (int) data.getSize());
        CHECK (std::abs (value (b, ParamIDs::swing) - 0.4f) < 1.0e-4f);
        CHECK (juce::roundToInt (value (b, ParamIDs::kit)) == 2);
        CHECK (b.laneOctave (3) == 1);
        CHECK (b.chordAt (2) == (fgg::Chord { 4, (int) fgg::ChordType::dom9 }));
        CHECK (juce::roundToInt (value (b, ParamIDs::pattern)) == fgg::userPatternIndex());
        CHECK (b.stepSymbol (1, 3) == 'f');
        CHECK (std::abs (b.editorScale - 1.3f) < 1.0e-4f);
        for (int l = 0; l < fgg::numLanes; ++l)
            for (int s = 0; s < 32; ++s)
                CHECK (a.stepSymbol (l, s) == b.stepSymbol (l, s));

        // Garbage doesn't crash or change anything.
        const char junk[] = "not a state";
        b.setStateInformation (junk, (int) sizeof (junk));
        CHECK (b.stepSymbol (1, 3) == 'f');

        // Undo snapshots restore the grid and the chords.
        const auto snap = b.snapshot();
        b.clearGrid();
        b.setChord (2, { 0, 0 });
        b.restoreSnapshot (snap);
        CHECK (b.stepSymbol (1, 3) == 'f');
        CHECK (b.chordAt (2).root == 4);
    }

    void testGridEditing()
    {
        FaseGrooveGuitarProcessor p;
        p.selectGroove (0);
        const char before = p.stepSymbol ((int) fgg::Lane::stab, 2);
        CHECK (fgg::isHit (before));   // Offbeat Chords: a stab on the offbeat
        p.editStep ((int) fgg::Lane::lick, 5, 'x');
        CHECK (juce::roundToInt (value (p, ParamIDs::pattern)) == fgg::userPatternIndex());
        CHECK (p.stepSymbol ((int) fgg::Lane::stab, 2) == before);   // the groove is kept
        CHECK (p.stepSymbol ((int) fgg::Lane::lick, 5) == 'x');
        p.clearLane ((int) fgg::Lane::stab);
        for (int s = 0; s < p.steps(); ++s)
            CHECK (p.stepSymbol ((int) fgg::Lane::stab, s) == '.');
        p.randomizeLane ((int) fgg::Lane::arp);
        int arps = 0;
        for (int s = 0; s < p.steps(); ++s)
            arps += fgg::isHit (p.stepSymbol ((int) fgg::Lane::arp, s)) ? 1 : 0;
        CHECK (arps > 0);

        // A locked part stays when the dice roll.
        p.setSegmentLocked (0, true);
        std::array<char, 8> locked {};
        for (int s = 0; s < 8; ++s)
            locked[(size_t) s] = p.stepSymbol ((int) fgg::Lane::lick, s);
        p.rollNewBeat();
        for (int s = 0; s < 8; ++s)
            CHECK (p.stepSymbol ((int) fgg::Lane::lick, s) == locked[(size_t) s]);

        // Velocities, chance and roll.
        p.setStepVelocity ((int) fgg::Lane::lick, 5, 90);
        CHECK (p.stepVelocity ((int) fgg::Lane::lick, 5) == 90);
        p.setStepRoll ((int) fgg::Lane::lick, 5, 3);
        CHECK (p.stepRoll ((int) fgg::Lane::lick, 5) == 3);
        p.setStepChance ((int) fgg::Lane::lick, 5, 50);
        CHECK (p.stepChance ((int) fgg::Lane::lick, 5) == 50);
        p.clearGrid();
        CHECK (p.stepSymbol ((int) fgg::Lane::lick, 5) == '.');
    }

    void testChordsAndStyles()
    {
        FaseGrooveGuitarProcessor p;
        // A progression in the key.
        p.setParameterFromUI (ParamIDs::key, 0.0f);
        p.applyProgression (0);   // i7 - iv7
        CHECK (p.chordAt (0) == (fgg::Chord { 0, (int) fgg::ChordType::m7 }));
        CHECK (p.chordAt (1) == (fgg::Chord { 5, (int) fgg::ChordType::m7 }));
        // A new key moves the chords (the shortest way).
        p.setKey (2);
        CHECK (p.chordAt (0).root == 2 && p.chordAt (1).root == 7);
        CHECK (juce::roundToInt (value (p, ParamIDs::key)) == 2);
        // One chord by hand.
        p.setChord (3, { 11, (int) fgg::ChordType::sus4 });
        CHECK (fgg::chordName (p.chordAt (3)) == "B7sus4");
        // The dice: another progression.
        const int before = juce::roundToInt (value (p, ParamIDs::progression));
        bool changed = false;   // (the dice may land on the same one: a few throws)
        for (int t = 0; t < 8 && ! changed; ++t)
        {
            p.randomizeChords();
            changed = juce::roundToInt (value (p, ParamIDs::progression)) != before;
        }
        CHECK (changed);

        // A style: a groove of the genre, its swing and its chords.
        const auto names = FaseGrooveGuitarProcessor::styleNames();
        CHECK (names.size() == 13);
        for (int i = 0; i < names.size(); ++i)
        {
            p.applyStyle (i);
            const int pattern = juce::roundToInt (value (p, ParamIDs::pattern));
            CHECK (juce::isPositiveAndBelow (pattern, (int) fgg::guitarPatterns().size()));
            CHECK (fgg::guitarPatterns()[(size_t) pattern].category == fgg::grooveCategories()[(size_t) i]);
            CHECK (std::abs (value (p, ParamIDs::swing) - fgg::genreSwing (i)) < 1.0e-3f);
            CHECK (juce::roundToInt (value (p, ParamIDs::progression)) == fgg::genreProgression (i));
        }

        // Total random always ends on something that plays.
        for (int i = 0; i < 10; ++i)
        {
            p.totalRandom();
            CHECK (countNoteOns (p.createMidiClip (4, FaseGrooveGuitarProcessor::ClipPart::all)) > 0);
        }
        p.resetToDefaults();
        CHECK (juce::roundToInt (value (p, ParamIDs::pattern)) == 0);
    }

    void testMidiExport()
    {
        FaseGrooveGuitarProcessor p;
        p.setParameterFromUI (ParamIDs::tempoSync, 0.0f);
        p.setParameterFromUI (ParamIDs::tempo, 122.0f);
        p.setParameterFromUI (ParamIDs::fills, 0.0f);
        p.setParameterFromUI (ParamIDs::variation, 0.0f);
        p.setParameterFromUI (ParamIDs::humanize, 0.0f);
        p.selectGroove (0);   // Offbeat Chords: stabs on every offbeat

        const auto clip = p.createMidiClip (4, FaseGrooveGuitarProcessor::ClipPart::all);
        CHECK (clip.getNumTracks() == 1);
        CHECK (countNoteOns (clip) > 16 * 3);
        CHECK (countNoteOns (clip, 1) == countNoteOns (clip));   // channel 1 by default

        // The notes are the chords of the slots, bar by bar.
        for (const auto* e : *clip.getTrack (0))
            if (e->message.isNoteOn())
            {
                const int bar = (int) (e->message.getTimeStamp() / (960.0 * 4.0));
                CHECK (isChordTone (p.chordAt (bar % 4), e->message.getNoteNumber()));
            }

        // The tempo of the file.
        juce::MidiMessageSequence tempos;
        clip.findAllTempoEvents (tempos);
        CHECK (tempos.getNumEvents() == 1);
        if (tempos.getNumEvents() == 1)
            CHECK (std::abs (60.0 / tempos.getEventPointer (0)->message.getTempoSecondsPerQuarterNote() - 122.0) < 0.01);

        // One track per lane.
        const auto tracks = p.createMidiClip (4, FaseGrooveGuitarProcessor::ClipPart::tracks);
        CHECK (tracks.getNumTracks() >= 3);
        CHECK (countNoteOns (tracks) >= countNoteOns (clip));

        // Another channel; a muted lane is left out.
        p.setParameterFromUI (ParamIDs::midiChannel, 5.0f);
        CHECK (countNoteOns (p.createMidiClip (4, FaseGrooveGuitarProcessor::ClipPart::all), 5) > 0);
        p.setLaneMuted ((int) fgg::Lane::stab, true);
        p.setLaneMuted ((int) fgg::Lane::scratch, true);
        CHECK (countNoteOns (p.createMidiClip (4, FaseGrooveGuitarProcessor::ClipPart::all)) == 0);
        p.setLaneMuted ((int) fgg::Lane::stab, false);
        p.setLaneMuted ((int) fgg::Lane::scratch, false);

        // The whole song: 60 bars with markers.
        const auto song = p.createMidiClip (FaseGrooveGuitarProcessor::songArrangement, FaseGrooveGuitarProcessor::ClipPart::all);
        std::set<juce::String> markers;
        for (const auto* e : *song.getTrack (0))
            if (e->message.isTextMetaEvent() && e->message.getMetaEventType() == 6)
                markers.insert (e->message.getTextFromTextMetaEvent());
        CHECK (markers.count ("Intro") == 1 && markers.count ("Drop") == 1 && markers.count ("Outro") == 1);
        CHECK (std::abs (song.getLastTimestamp() - 60.0 * 4.0 * 960.0) < 1.0);

        // The file on disk, with a readable name.
        const auto file = p.writeMidiClip (0, FaseGrooveGuitarProcessor::ClipPart::all);
        CHECK (file.existsAsFile());
        CHECK (file.getFileName().startsWith ("Guitar "));
        CHECK (file.getFileName().contains ("122bpm"));
        file.deleteFile();
    }

    void testAudioExport()
    {
        FaseGrooveGuitarProcessor p;
        p.setParameterFromUI (ParamIDs::tempoSync, 0.0f);
        p.selectGroove (1);
        const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("fgg-audio-test");
        folder.deleteRecursively();
        folder.createDirectory();

        const auto files = p.writeAudio (FaseGrooveGuitarProcessor::AudioExport::clip, 2, folder.getChildFile ("clip.wav"));
        CHECK (files.size() == 1);
        if (files.size() == 1)
        {
            juce::AudioFormatManager formats;
            formats.registerBasicFormats();
            std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (files[0]));
            CHECK (reader != nullptr);
            if (reader != nullptr)
            {
                juce::AudioBuffer<float> audio (2, (int) reader->lengthInSamples);
                reader->read (&audio, 0, audio.getNumSamples(), 0, true, true);
                const float peak = audio.getMagnitude (0, audio.getNumSamples());
                CHECK (peak > 0.02f && peak < 4.0f);   // it sounds, and nothing explodes
                bool finite = true;
                for (int i = 0; i < audio.getNumSamples(); ++i)
                    finite = finite && std::isfinite (audio.getSample (0, i));
                CHECK (finite);
            }
        }
        const auto stems = p.writeAudio (FaseGrooveGuitarProcessor::AudioExport::stems, 1, folder.getChildFile ("stems"));
        CHECK (stems.size() >= 2);
        folder.deleteRecursively();
    }

    void testTransport()
    {
        FaseGrooveGuitarProcessor p;
        PlayHead head;
        p.setPlayHead (&head);
        p.setParameterFromUI (ParamIDs::fills, 0.0f);
        p.setParameterFromUI (ParamIDs::variation, 0.0f);
        p.setParameterFromUI (ParamIDs::humanize, 0.0f);
        p.selectGroove (0);
        p.setRateAndBufferSizeDetails (48000.0, 512);
        p.prepareToPlay (48000.0, 512);

        juce::AudioBuffer<float> buffer (2, 512);
        int notes = 0;
        bool sound = false;
        const double beatsPerBlock = 512.0 / (48000.0 * 60.0 / head.bpm);
        const int blocks = (int) std::ceil (8.0 / beatsPerBlock);   // two bars
        for (int b = 0; b < blocks; ++b)
        {
            head.ppq = b * beatsPerBlock;
            juce::MidiBuffer midi;
            p.processBlock (buffer, midi);
            for (const auto m : midi)
                notes += m.getMessage().isNoteOn() ? 1 : 0;
            sound = sound || buffer.getMagnitude (0, 512) > 0.01f;
        }
        CHECK (notes >= 8 * 3);   // eight offbeat stabs of three strings, and scratches
        CHECK (sound);           // the guide guitar plays

        // Stop: everything is released, nothing new starts.
        head.playing = false;
        juce::MidiBuffer midi;
        p.processBlock (buffer, midi);
        for (const auto m : midi)
            CHECK (! m.getMessage().isNoteOn());

        // The sequencer off: the DAW plays, the groove doesn't; PLAY still previews it.
        head.playing = true;
        p.setParameterFromUI (ParamIDs::sequencer, 0.0f);
        for (int b = 0; b < 40; ++b)
        {
            juce::MidiBuffer m2;
            head.ppq = 100.0 + b * beatsPerBlock;
            p.processBlock (buffer, m2);
            for (const auto m : m2)
                CHECK (! m.getMessage().isNoteOn());
        }
        p.previewPlay();
        int previewed = 0;
        for (int b = 0; b < 40; ++b)
        {
            juce::MidiBuffer m2;
            head.ppq = 200.0 + b * beatsPerBlock;
            p.processBlock (buffer, m2);
            for (const auto m : m2)
                previewed += m.getMessage().isNoteOn() ? 1 : 0;
        }
        CHECK (previewed > 0);
        p.setPlayHead (nullptr);
    }

    void testChordInput()
    {
        FaseGrooveGuitarProcessor p;
        PlayHead head;
        p.setPlayHead (&head);
        p.setParameterFromUI (ParamIDs::fills, 0.0f);
        p.setParameterFromUI (ParamIDs::variation, 0.0f);
        p.selectGroove (0);
        p.setRateAndBufferSizeDetails (48000.0, 512);
        p.prepareToPlay (48000.0, 512);
        juce::AudioBuffer<float> buffer (2, 512);
        const double beatsPerBlock = 512.0 / (48000.0 * 60.0 / head.bpm);

        // E9 held on the MIDI input (E G# B D F#): the guitar plays it, the notes held don't pass through.
        const fgg::Chord e9 { 4, (int) fgg::ChordType::dom9 };
        int played = 0;
        for (int b = 0; b < 60; ++b)
        {
            juce::MidiBuffer midi;
            if (b == 0)
                for (int n : { 40, 56, 59, 62, 66 })
                    midi.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0);
            head.ppq = b * beatsPerBlock;
            p.processBlock (buffer, midi);
            for (const auto m : midi)
                if (m.getMessage().isNoteOn())
                {
                    ++played;
                    CHECK (isChordTone (e9, m.getMessage().getNoteNumber()));
                    CHECK (m.getMessage().getVelocity() != 100 || m.samplePosition > 0 || b > 0);
                }
        }
        CHECK (played > 0);
        CHECK (p.followsHeldChord());
        CHECK (ChordPanel::nameOf (p.heldChord()) == "E9");

        // Released: back to the chord slots.
        {
            juce::MidiBuffer midi;
            for (int n : { 40, 56, 59, 62, 66 })
                midi.addEvent (juce::MidiMessage::noteOff (1, n), 0);
            p.processBlock (buffer, midi);
            CHECK (! p.followsHeldChord());
        }

        // The same note struck twice: held until both are released; All Notes Off lets go of all.
        {
            juce::MidiBuffer midi;
            for (int n : { 40, 56, 59, 62, 66 })
                midi.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0);
            midi.addEvent (juce::MidiMessage::noteOn (1, 40, (juce::uint8) 90), 1);
            midi.addEvent (juce::MidiMessage::noteOff (1, 40), 2);
            p.processBlock (buffer, midi);
            CHECK (p.followsHeldChord() && ChordPanel::nameOf (p.heldChord()) == "E9");   // E still held once
            juce::MidiBuffer off;
            off.addEvent (juce::MidiMessage::allNotesOff (1), 0);
            p.processBlock (buffer, off);
            CHECK (! p.followsHeldChord());
        }

        // MIDI IN off: the notes pass through.
        p.setParameterFromUI (ParamIDs::chordInput, 0.0f);
        head.playing = false;
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 64, (juce::uint8) 99), 10);
        p.processBlock (buffer, midi);
        bool passed = false;
        for (const auto m : midi)
            passed = passed || (m.getMessage().isNoteOn() && m.getMessage().getNoteNumber() == 64 && m.getMessage().getVelocity() == 99);
        CHECK (passed);
        CHECK (! p.followsHeldChord());

        // A lane played by hand: its notes now, released half a second later.
        p.audition ((int) fgg::Lane::stab, 110);
        juce::MidiBuffer first;
        p.processBlock (buffer, first);
        int ons = 0;
        for (const auto m : first)
            ons += m.getMessage().isNoteOn() ? 1 : 0;
        CHECK (ons == 3);
        int offs = 0;
        for (int b = 0; b < 60; ++b)
        {
            juce::MidiBuffer m2;
            p.processBlock (buffer, m2);
            for (const auto m : m2)
                offs += m.getMessage().isNoteOff() ? 1 : 0;
        }
        CHECK (offs >= 3);
        p.setPlayHead (nullptr);
    }

    void testMidiImport()
    {
        // Four bars: Dm7, G7, Cmaj7, Am7 strummed on the beats, a chank on the offbeats.
        const fgg::Chord chords[] { { 2, (int) fgg::ChordType::m7 }, { 7, (int) fgg::ChordType::dom7 },
                                    { 0, (int) fgg::ChordType::maj7 }, { 9, (int) fgg::ChordType::m7 } };
        juce::MidiMessageSequence track;
        for (int bar = 0; bar < 4; ++bar)
        {
            const auto notes = fgg::laneNotes (fgg::Lane::strumDown, fgg::tonesOf (chords[bar]), 0);
            for (int beat = 0; beat < 4; ++beat)
            {
                const double t = (bar * 4 + beat) * 480.0;
                for (int n : notes)
                {
                    track.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), t);
                    track.addEvent (juce::MidiMessage::noteOff (1, n), t + 400.0);
                }
                const int high = fgg::laneNotes (fgg::Lane::chank, fgg::tonesOf (chords[bar]), 2)[0];
                track.addEvent (juce::MidiMessage::noteOn (1, high, (juce::uint8) 80), t + 240.0);
                track.addEvent (juce::MidiMessage::noteOff (1, high), t + 300.0);
            }
        }
        track.updateMatchedPairs();
        juce::MidiFile midi;
        midi.setTicksPerQuarterNote (480);
        midi.addTrack (track);
        const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("fgg-import-test.mid");
        file.deleteFile();
        {
            juce::FileOutputStream out (file);
            midi.writeTo (out);
        }

        FaseGrooveGuitarProcessor p;
        CHECK (p.importMidiGroove (file));
        CHECK (juce::roundToInt (value (p, ParamIDs::pattern)) == fgg::userPatternIndex());
        for (int bar = 0; bar < 4; ++bar)
            CHECK (p.chordAt (bar) == chords[bar]);
        CHECK (fgg::isHit (p.stepSymbol ((int) fgg::Lane::strumDown, 0)));
        CHECK (fgg::isHit (p.stepSymbol ((int) fgg::Lane::chank, 2)));
        file.deleteFile();

        // An empty file: nothing changes.
        juce::MidiFile empty;
        empty.setTicksPerQuarterNote (480);
        empty.addTrack (juce::MidiMessageSequence());
        {
            juce::FileOutputStream out (file);
            empty.writeTo (out);
        }
        CHECK (! p.importMidiGroove (file));
        file.deleteFile();
    }

    void testGuideGuitar()
    {
        static constexpr double rate = 44100.0;
        std::array<float, fgg::numLanes> volume {};
        volume.fill (1.0f);

        // The level of a note (in a lane) `at` seconds after it starts, with a note-off at `off`.
        auto levelAt = [&] (int lane, double at, double off)
        {
            GuideGuitar g;
            g.prepare (rate);
            juce::AudioBuffer<float> buffer (2, (int) (at * rate) + 2048);
            buffer.clear();
            std::vector<GuideGuitar::Event> events { { 0, 57, 1.0f, lane } };
            if (off >= 0.0)
                events.push_back ({ (int) (off * rate), 57, 0.0f, lane });
            g.render (buffer, events, volume);
            return buffer.getRMSLevel (0, (int) (at * rate), 2048);
        };
        CHECK (levelAt ((int) fgg::Lane::strumDown, 0.05, -1.0) > 0.005f);    // it sounds
        CHECK (levelAt ((int) fgg::Lane::strumDown, 0.5, -1.0) > 0.001f);     // and rings
        CHECK (levelAt ((int) fgg::Lane::strumDown, 0.5, 0.1) < 1.0e-4f);     // the finger lifts: it stops
        CHECK (levelAt ((int) fgg::Lane::scratch, 0.3, -1.0) < levelAt ((int) fgg::Lane::strumDown, 0.3, -1.0) * 0.1f);   // muted
        CHECK (levelAt ((int) fgg::Lane::swell, 0.01, -1.0) < levelAt ((int) fgg::Lane::strumDown, 0.01, -1.0));        // fades in

        // In tune: the period of a plucked A2, A3 and E4 (autocorrelation over 50 ms).
        for (int note : { 45, 57, 64 })
        {
            GuideGuitar g;
            g.prepare (rate);
            juce::AudioBuffer<float> buffer (2, (int) (rate * 0.4));
            buffer.clear();
            g.render (buffer, { { 0, note, 1.0f, (int) fgg::Lane::strumDown } }, volume);
            const float* x = buffer.getReadPointer (0) + (int) (rate * 0.2);
            const int n = (int) (rate * 0.05);
            double bestScore = -1.0e9, bestLag = 0.0;
            for (int lag = (int) (rate / 1000.0); lag < (int) (rate / 70.0); ++lag)
            {
                double sum = 0.0;
                for (int i = 0; i < n; ++i)
                    sum += (double) x[i] * x[i + lag];
                if (sum > bestScore)
                    bestScore = sum, bestLag = lag;
            }
            const double expected = 440.0 * std::pow (2.0, (note - 69) / 12.0);
            CHECK (std::abs (rate / bestLag / expected - 1.0) < 0.02);
        }

        // Many notes: no NaN, no blow-up, on every sound.
        for (int sound = 0; sound < GuideGuitar::numTones; ++sound)
        {
            GuideGuitar g;
            g.prepare (rate);
            g.setSound (sound);
            juce::AudioBuffer<float> buffer (2, (int) rate);
            buffer.clear();
            std::vector<GuideGuitar::Event> events;
            for (int i = 0; i < 200; ++i)
                events.push_back ({ i * 200, 40 + (i * 7) % 40, 0.9f, i % fgg::numLanes });
            g.render (buffer, events, volume);
            const float peak = buffer.getMagnitude (0, buffer.getNumSamples());
            CHECK (std::isfinite (peak) && peak > 0.01f && peak < 8.0f);
        }
    }

    void testUpdater()
    {
        CHECK (Updater::isNewer ("v0.2.0", "0.1.0"));
        CHECK (Updater::isNewer ("v1.0.0", "0.10.3"));
        CHECK (! Updater::isNewer ("v0.1.0", "0.1.0"));
        const auto notes = Updater::changelogSection ("# Changelog\n\n## [0.2.0] - x\n\n- New A\n\n## [0.1.0] - y\n\n- Old\n", "0.2.0");
        CHECK (notes.contains ("New A") && ! notes.contains ("Old"));
    }

    void testTranslations()
    {
        const auto before = Localisation::current();
        Localisation::setCurrent (Localisation::Language::italian);
        CHECK (tr ("CLOSE") == "CHIUDI");
        CHECK (tr ("Chords") == "Accordi");
        CHECK (tr ("Down strum") == "Pennata giù");
        CHECK (tr ("Swing") == "Swing");                         // technical terms stay in English
        CHECK (tr ("Some groove name") == "Some groove name");   // no translation: as it is

        // Every language has a text for the guitar's own words.
        for (int lang = 1; lang < Localisation::numLanguages; ++lang)
        {
            Localisation::setCurrent ((Localisation::Language) lang);
            for (const auto* key : { "Guitar part", "Chords", "Plays", "HEAR IT", "Random groove", "Down strum", "Muted scratch",
                                     "Hold a chord on the MIDI input", "Groove feel & song", "CLOSE" })
                CHECK (tr (key).isNotEmpty() && tr (key) != key);
        }
        Localisation::setCurrent (before);
    }

    void testEditor()
    {
        // The editor opens, follows the processor, and closes, in every language.
        FaseGrooveGuitarProcessor p;
        for (int lang = 0; lang < Localisation::numLanguages; ++lang)
        {
            Localisation::setCurrent ((Localisation::Language) lang);
            std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditor());
            editor->setSize (1180, 860);
            if (auto* t = dynamic_cast<juce::Timer*> (editor.get()))
                t->timerCallback();
            const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
            CHECK (image.isValid());
        }
        Localisation::setCurrent (Localisation::Language::english);
    }

    //==============================================================================
    void saveScreenshots (const juce::File& folder)
    {
        folder.createDirectory();
        FaseGrooveGuitarProcessor p;
        p.applyStyle (8);   // Funky & Disco
        std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditor());
        editor->setSize (1180, 860);
        if (auto* t = dynamic_cast<juce::Timer*> (editor.get()))
            t->timerCallback();

        auto save = [&] (const juce::String& name)
        {
            const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
            juce::PNGImageFormat png;
            juce::FileOutputStream out (folder.getChildFile (name + ".png"));
            out.setPosition (0);
            out.truncate();
            png.writeImageToStream (image, out);
        };
        std::function<juce::Button* (juce::Component&, const juce::String&)> findButton = [&] (juce::Component& c, const juce::String& text) -> juce::Button*
        {
            for (auto* child : c.getChildren())
            {
                if (auto* b = dynamic_cast<juce::Button*> (child); b != nullptr && b->getButtonText() == text)
                    return b;
                if (auto* found = findButton (*child, text))
                    return found;
            }
            return nullptr;
        };
        auto click = [&] (const juce::String& text) { if (auto* b = findButton (*editor, tr (text)); b != nullptr && b->onClick) b->onClick(); };

        save ("1-main");
        click ("PROGRESSION...");
        save ("1c-progressions");
        std::function<void (juce::Component&)> hideBrowsers = [&] (juce::Component& c)
        {
            for (auto* child : c.getChildren())
            {
                if (dynamic_cast<ProgressionBrowser*> (child) != nullptr)
                    child->setVisible (false);
                hideBrowsers (*child);
            }
        };
        hideBrowsers (*editor);
        click ("EDITOR...");
        save ("1b-editor");
        click ("VELOCITY");
        save ("2-velocity");
        click ("MIXER");
        save ("2b-mixer");
        click ("FILL");
        p.customFillFromStyle ((int) fgg::FillStyle::octaveRun);
        save ("2d-fill");
        click ("STEP GRID");
        click ("FULL SONG...");
        save ("3-song");
        click ("CLOSE");
        click ("OPTIONS");
        save ("4-options");
    }
    void testLinkRoles()
    {
        using P = FaseGrooveGuitarProcessor;
        // KEY: another plug-in's key arrives only with KEY on (the chords move with it).
        {
            const auto link = P::linkFile();
            const bool existed = link.existsAsFile();
            const auto backup = link.loadFileAsString();
            auto writeTheirs = [&link] (int key)
            {
                juce::XmlElement xml ("FaseGrooveLink");
                xml.setAttribute ("instance", "someone else");
                xml.setAttribute ("key", key);
                xml.setAttribute ("scale", 2);
                xml.writeTo (link);
            };
            P p;
            p.setLinked (true);
            const int before = juce::roundToInt (p.raw (ParamIDs::key));
            writeTheirs ((before + 3) % 12);
            p.updateLink();
            CHECK (juce::roundToInt (p.raw (ParamIDs::key)) == before);
            p.setKeyLinked (true);
            writeTheirs ((before + 5) % 12);
            p.updateLink();
            CHECK (juce::roundToInt (p.raw (ParamIDs::key)) == (before + 5) % 12);
            p.setLinked (false);
            if (existed) link.replaceWithText (backup); else link.deleteFile();
        }

        // LEAD: written beside the link file only while linked, cleared when switched off.
        const auto lead = P::leadFile();
        const bool existed = lead.existsAsFile();
        const auto backup = lead.loadFileAsString();
        lead.deleteFile();
        {
            P p;
            p.setLeadMode (true);
            CHECK (! lead.existsAsFile());          // not linked: nothing changes
            p.setLinked (true);
            CHECK (lead.existsAsFile());
            if (auto xml = juce::parseXML (lead))
                CHECK (xml->getBoolAttribute ("lead"));
            p.setLeadMode (false);
            CHECK (! lead.existsAsFile());
            p.setLeadMode (true);
            p.setLinked (false);
            CHECK (! lead.existsAsFile());
        }
        if (existed) lead.replaceWithText (backup);
    }

}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juce;

    if (argc > 3 && juce::String (argv[1]) == "--screenshots")
        Localisation::setCurrent ((Localisation::Language) juce::String (argv[3]).getIntValue());
    if (argc > 2 && juce::String (argv[1]) == "--screenshots")
    {
        saveScreenshots (juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]));
        return 0;
    }

    testStateRoundTrip();
    testGridEditing();
    testChordsAndStyles();
    testMidiExport();
    testGrooveCompatibility();
    testAudioExport();
    testTransport();
    testChordInput();
    testMidiImport();
    testGuideGuitar();
    testUpdater();
    testTranslations();
    testLinkRoles();
    testEditor();

    if (failures == 0)
        std::printf ("All plug-in tests passed.\n");
    return failures == 0 ? 0 : 1;
}

// Fase Groove Guitar - Copyright (C) 2026 Fabio 'Pitch' Serra
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include "Branding.h"
#include "Localisation.h"
#include "Updater.h"
#include "PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

// The "studio" theme of Fase Groove Piano: graphite surfaces, one warm and one cool accent.
namespace Palette
{
    const juce::Colour background { 0xff0d0f13 };   // window
    const juce::Colour panel      { 0xff161920 };   // panels
    const juce::Colour panelEdge  { 0xff272c35 };   // borders, tracks, empty cells
    const juce::Colour control    { 0xff1d2129 };   // buttons
    const juce::Colour field      { 0xff0f1216 };   // menus, displays
    const juce::Colour accent     { 0xffffa94d };   // amber: the strums, the main actions
    const juce::Colour accent2    { 0xff4cc3f0 };   // cyan: stabs, chanks, scratches, playing
    const juce::Colour cymbals    { 0xff5fd68a };   // green: picking and licks
    const juce::Colour percussion { 0xffb69cff };   // violet: slides and swells
    const juce::Colour toms       { 0xffff5a5f };   // red: the low strings (octaves, power chords, root)
    const juce::Colour text       { 0xffe7e9ec };
    const juce::Colour dim        { 0xff7f8794 };
}

/** Each lane's colour, everywhere: strums amber, stabs and chanks cyan, picking green, slides and
    swells violet, the low strings red. */
juce::Colour laneColour (int lane);

//==============================================================================
class GrooveLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GrooveLookAndFeel();

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& background, bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isDown, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
    juce::Font getPopupMenuFont() override;
    void drawTooltip (juce::Graphics&, const juce::String& text, int width, int height) override;
    juce::Rectangle<int> getTooltipBounds (const juce::String& text, juce::Point<int> screenPos, juce::Rectangle<int> parentArea) override;
    juce::Font getLabelFont (juce::Label&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;

private:
    juce::Typeface::Ptr regular, semiBold;
};

//==============================================================================
/** A flat button with a drawn icon (and optional text): transport, dice, undo, locks... */
class IconButton : public juce::Button
{
public:
    enum class Icon { none, play, pause, stop, rewind, dice, undo, redo, gear, close, fullScreen, exitFullScreen,
                      lock, unlock, left, right, song, folder };

    explicit IconButton (Icon i = Icon::none) : juce::Button ({}), icon (i) {}
    void setIcon (Icon i) { if (i != icon) { icon = i; repaint(); } }
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
    static void drawIcon (juce::Graphics&, Icon, juce::Rectangle<float>, juce::Colour);

    juce::Colour onColour { Palette::accent };
    juce::Colour iconColour;   // when set: the icon's own colour while off (the dice are amber)
    bool primary = false;      // filled with onColour even when off (the main action of a panel)

private:
    Icon icon;
};

//==============================================================================
/** Rotary knob with a caption under it. */
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& caption);
    void resized() override;
    void setTip (const juce::String& tip) { Localisation::bindTip (slider, tip); }

private:
    juce::Slider slider;
    juce::Label label;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};

/** A drop-down menu for a parameter, with a caption above it (or none). The items can be grouped
    under headings in any order: an item's id is always "parameter index + 1". */
class ParamCombo : public juce::Component
{
public:
    ParamCombo (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& caption);
    void resized() override;

    /** Re-reads the parameter (call it a few times a second). */
    void refresh();
    juce::ComboBox& comboBox() noexcept { return box; }
    void setTip (const juce::String& tip) { Localisation::bindTip (box, tip); }

private:
    juce::ComboBox box;
    juce::Label label;
    juce::RangedAudioParameter* parameter = nullptr;
    bool hasCaption = true;
};

//==============================================================================
/** The rhythm grid: one row per lane (with its die), 32 steps (two bars). */
class GuitarGrid : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit GuitarGrid (FaseGrooveGuitarProcessor&);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    /** Re-reads the groove and the playhead (editor timer). */
    void refresh();
    /** The lanes' lights (they flash on the hits) and the lane picked in the part panel. */
    void setLights (const std::array<float, fgg::numLanes>& l, int selected) { lights = l; selectedLane = selected; }
    std::function<void (int lane)> onSelectLane;

    static constexpr int headerWidth = 112;
    /** Called when a lane's name is double-clicked: show it in the velocity view. */
    std::function<void (int lane)> onShowVelocity;

private:
    juce::Rectangle<float> cellBounds (int lane, int step) const;
    juce::Rectangle<float> headerBounds (int lane) const;
    juce::Rectangle<float> diceBounds (int lane) const;
    bool cellAt (juce::Point<float>, int& lane, int& step) const;
    void showCellMenu (int lane, int step);
    void showLaneMenu (int lane);
    void paintCell (int lane, int step, char symbol);

    FaseGrooveGuitarProcessor& processor;
    fgg::GuitarPattern pattern;
    std::array<bool, fgg::numLanes> muted {};
    std::array<float, fgg::numLanes> lights {};
    int playStep = -1, selectedLane = -1;
    int hoverLane = -1, hoverStep = -1;
    bool hoverDice = false;
    char paintSymbol = 0;
    int lastPaintLane = -1, lastPaintStep = -1;
};

//==============================================================================
/** The velocity piano roll: one lane at a time (chosen from a menu), its hits on top and a
    velocity bar under each one, to drag up and down. */
class VelocityView : public juce::Component
{
public:
    explicit VelocityView (FaseGrooveGuitarProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    void refresh();
    void setLane (int lane);
    int getLane() const noexcept { return lane; }

private:
    juce::Rectangle<float> stripArea() const;
    juce::Rectangle<float> laneArea() const;
    juce::Rectangle<float> column (int step, juce::Rectangle<float> area) const;
    int stepAt (float x) const;
    void setFromMouse (juce::Point<float>);

    FaseGrooveGuitarProcessor& processor;
    int lane = (int) fgg::Lane::scratch;
    juce::ComboBox laneBox, shapeBox;
    IconButton diceButton { IconButton::Icon::dice };
    juce::TextButton autoButton { "AUTO" }, autoAllButton { "ALL AUTO" };
    std::array<char, FaseGrooveGuitarProcessor::gridSteps> symbols {};
    std::array<int, FaseGrooveGuitarProcessor::gridSteps> levels {}, setByHand {};
    int playStep = -1, hoverStep = -1;
    bool painting = false;
};

//==============================================================================
/** The fill drawn by hand (Fill style: Custom): one bar, every lane. It takes over from its first
    written step; the lanes left empty keep playing the groove. */
class FillView : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit FillView (FaseGrooveGuitarProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { paintSymbol = 0; }

private:
    juce::Rectangle<float> gridArea() const;
    bool cellAt (juce::Point<float>, int& lane, int& step) const;
    void paintCell (int lane, int step, char symbol);

    FaseGrooveGuitarProcessor& processor;
    juce::TextButton startButton { "START FROM..." }, clearButton { "CLEAR FILL" };
    char paintSymbol = 0;
    int lastLane = -1, lastStep = -1;
};

//==============================================================================
/** Drag me into the DAW: writes the MIDI clip(s) and starts an external file drag. */
class DragTile : public juce::Component, public juce::SettableTooltipClient
{
public:
    DragTile (juce::String titleText = "DRAG MIDI TO DAW", juce::String subtitleText = "drag this tile onto a track")
        : title (std::move (titleText)), subtitle (std::move (subtitleText)) {}
    std::function<juce::StringArray()> createFiles;
    bool filled = false;   // the big orange tile of the export panel
    void setSubtitle (const juce::String& s) { if (s != subtitle) { subtitle = s; repaint(); } }
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragging = false; repaint(); }
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }

private:
    juce::String title, subtitle;
    bool dragging = false;
};

//==============================================================================
/** The song window: bars, own variation and fill of each section; drag or save the whole song,
    one section, or every section as its own clip. */
class SongOverlay : public juce::Component
{
public:
    explicit SongOverlay (FaseGrooveGuitarProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void show();
    void refresh();

    FaseGrooveGuitarProcessor::ClipPart clipPart() const;

private:
    juce::Rectangle<float> card() const;
    juce::Rectangle<float> timelineArea() const;
    juce::Rectangle<int> rowArea (int part) const;
    void saveSong();
    void saveSections();

    FaseGrooveGuitarProcessor& processor;
    juce::OwnedArray<juce::ComboBox> barBoxes, fillBoxes;
    juce::OwnedArray<IconButton> newButtons;
    juce::OwnedArray<juce::TextButton> originalButtons;
    juce::TextButton allButton { "ONE TRACK" }, tracksButton { "ONE TRACK PER LANE" };
    juce::TextButton saveSongButton { "SAVE SONG .MID..." }, saveSectionsButton { "ONE .MID PER SECTION..." }, closeButton { "CLOSE" };
    juce::TextButton resetButton { "RESET SONG" };
    DragTile sectionTile { "DRAG THIS SECTION" }, sectionsTile { "DRAG ALL SECTIONS", "one clip per section" },
             songTile { "DRAG THE WHOLE SONG", "every section, with markers" };
    std::unique_ptr<juce::FileChooser> chooser;
    int selected = 1;   // the section the tile drags (part index)
};

//==============================================================================
/** OPTIONS, as in Fase Groove Piano: a SETTINGS tab (language, interface size, tips, updates,
    shortcuts) and an ABOUT tab (credits, licence, links). OPTIONS, or a click on the logo. */
class OptionsOverlay : public juce::Component
{
public:
    OptionsOverlay();

    std::function<void (float)> onScaleChosen;
    std::function<void (bool)> onTipsChanged;
    std::function<void()> onCheckUpdates;

    juce::TextButton checkUpdatesButton;

    void show (bool aboutTab, float currentScale);
    void refreshText();

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> card() const { return getLocalBounds().toFloat().withSizeKeepingCentre (620.0f, 620.0f); }
    void selectTab (bool about);

    bool showingAbout = false;
    juce::Image artist;
    juce::TextButton settingsTab, aboutTab, closeButton;
    juce::TextButton sourceButton, kofiButton, paypalButton, spotifyButton;
    juce::Label languageLabel, sizeLabel;
    juce::ComboBox languageBox, sizeBox;
    juce::ToggleButton tipsToggle, updatesToggle;
};

//==============================================================================
/** "A new version is ready": what's new (the changelog) and the user confirms the install. */
class UpdateOverlay : public juce::Component
{
public:
    UpdateOverlay();
    void show (const Updater::Result&);
    void refreshText();
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Rectangle<float> card() const { return getLocalBounds().toFloat().withSizeKeepingCentre (640.0f, 560.0f); }
    Updater::Result result;
    juce::TextEditor notes;
    juce::TextButton installButton, laterButton, skipButton;
};

//==============================================================================
/** One of the four parts of the groove (a half bar), shown like the piano's chord slots: its
    position, the part playing there (A-D), a picture of its hits, its die and its lock. */
class PartCard : public juce::Component
{
public:
    PartCard (FaseGrooveGuitarProcessor&, int position);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh (const fgg::GuitarPattern& shown, int playStep);

private:
    FaseGrooveGuitarProcessor& processor;
    const int position;
    ParamCombo combo;
    IconButton dice { IconButton::Icon::dice }, lock { IconButton::Icon::unlock };
    fgg::GuitarPattern pattern;
    bool playing = false, locked = false;
};

//==============================================================================
/** The groove browser: the genres on the left, their grooves on the right, each with a picture
    of its rhythm. A click loads it (you hear it at once if it plays). */
class GrooveBrowser : public juce::Component
{
public:
    explicit GrooveBrowser (FaseGrooveGuitarProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
    bool keyPressed (const juce::KeyPress&) override;
    void show();

private:
    juce::Rectangle<float> card() const;
    juce::Rectangle<float> genreBounds (int genre) const;
    juce::Rectangle<float> tileBounds (int index) const;
    std::vector<int> groovesOf (int genre) const;
    void pick (int pattern);

    FaseGrooveGuitarProcessor& processor;
    int genre = 0, hover = -1;
    IconButton randomButton { IconButton::Icon::dice };
    juce::TextButton closeButton { "CLOSE" };
};

//==============================================================================
/** The mixer: a volume fader, a mute, push / pull and swing for each lane of the guide guitar. */
class MixerView : public juce::Component
{
public:
    explicit MixerView (FaseGrooveGuitarProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void setLights (const std::array<float, fgg::numLanes>& l) { lights = l; repaint(); }

private:
    juce::Rectangle<int> strip (int lane) const;
    FaseGrooveGuitarProcessor& processor;
    juce::OwnedArray<juce::Slider> faders, pushKnobs, swingKnobs;
    juce::OwnedArray<juce::TextButton> mutes;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> faderAttachments, knobAttachments;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ButtonAttachment> muteAttachments;
    std::array<float, fgg::numLanes> lights {};
};

//==============================================================================
/** The chords, as the piano's chord slots: the key, a progression by genre, four chords (one bar
    each; click one to change it), the dice for new ones, and MIDI IN: a chord held on the MIDI
    input (your keyboard, or Fase Groove Piano's MIDI out) takes over. */
class ChordPanel : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit ChordPanel (FaseGrooveGuitarProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void refresh();

    /** "Am7", "Fmaj9"... the name of the chord some notes make ("A (5)" for a root and a 5th). */
    static juce::String nameOf (const fgg::ChordTones&);

private:
    juce::Rectangle<float> slotBounds (int slot) const;
    void showSlotMenu (int slot);
    void showProgressionMenu();

    FaseGrooveGuitarProcessor& processor;
    juce::ComboBox keyBox;
    juce::TextButton progressionButton { "PROGRESSION..." }, inputButton { "MIDI IN" };
    IconButton diceButton { IconButton::Icon::dice };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> inputAttachment;
    std::array<fgg::Chord, 4> chords {};
    int playingSlot = -1;
    fgg::ChordTones held { 0, 0 };
    bool following = false;
};

//==============================================================================
class FaseGrooveGuitarEditor : public juce::AudioProcessorEditor, public juce::FileDragAndDropTarget, public juce::Timer
{
public:
    explicit FaseGrooveGuitarEditor (FaseGrooveGuitarProcessor&);
    ~FaseGrooveGuitarEditor() override;

    void resized() override;
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int, int) override;

    static constexpr int designWidth = 1180, designHeight = 860;

    void timerCallback() override;

private:

    /** Everything is laid out at the design size, then scaled (and centred) in the window. */
    struct Content : public juce::Component
    {
        explicit Content (FaseGrooveGuitarEditor& e) : editor (e) {}
        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseDown (const juce::MouseEvent&) override;
        bool keyPressed (const juce::KeyPress&) override;
        FaseGrooveGuitarEditor& editor;
    };

    float currentScale() const;
    void applyTips();
    void translateHostWindow();
    void parentHierarchyChanged() override;
    void layout();
    void paintContent (juce::Graphics&);
    int selectedClipBars() const;
    FaseGrooveGuitarProcessor::ClipPart selectedClipPart() const;
    void saveMidiFile();
    void showPresetsMenu();
    void showGroovesMenu();
    void askName (const juce::String& title, const juce::String& suggestion, std::function<void (const juce::String&)> onName);
    void importMidi();
    void importFile (const juce::File&);
    void setView (int mode);   // 0 step grid, 1 velocity, 2 mixer, 3 fill
    void showTemplateMenu();
    void showAudioExportMenu();
    void stepGroove (int delta);
    void selectLane (int lane);
    void refreshKitPanel();
    void undo();
    void redo();
    bool isFullScreen() const;
    void toggleFullScreen();
    void showMessage (const juce::String& text);

    FaseGrooveGuitarProcessor& processor;
    GrooveLookAndFeel lookAndFeel;
    std::unique_ptr<juce::TooltipWindow> tooltips;
    Content content { *this };

    // Header
    juce::TextButton presetsButton { "PRESETS" }, styleButton { "STYLE" }, resetButton { "RESET" };
    IconButton totalRandomButton { IconButton::Icon::dice }, undoButton { IconButton::Icon::undo }, redoButton { IconButton::Icon::redo },
               fullScreenButton { IconButton::Icon::fullScreen }, optionsButton { IconButton::Icon::gear };
    IconButton rewindButton { IconButton::Icon::rewind }, playButton { IconButton::Icon::play }, stopButton { IconButton::Icon::stop };
    juce::ToggleButton sequencerToggle { "Sequencer" };
    juce::TextButton syncButton { "SYNC DAW" };
    juce::Slider tempoSlider;
    juce::Label dawTempo;   // the DAW's tempo, in place of the slider while SYNC DAW is on
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttachment, soundAttachment, bigStrumAttachment, sequencerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tempoAttachment;

    // Groove row
    ParamCombo grooveCombo, genreCombo, partLengthCombo;
    juce::TextButton grooveButton;
    IconButton prevGroove { IconButton::Icon::left }, nextGroove { IconButton::Icon::right };
    IconButton newBeatButton { IconButton::Icon::dice };
    juce::Slider densitySlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> densityAttachment;
    std::unique_ptr<PartCard> partCards[fgg::numSegments];

    // Guitar part (the lane picked)
    int selectedLane = 0;
    juce::ComboBox laneBox, octaveBox;
    juce::TextButton hearButton { "HEAR IT" }, muteButton { "MUTE" };
    IconButton laneDiceButton { IconButton::Icon::dice };
    juce::Slider levelKnob, volumeKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> levelAttachment, volumeAttachment;

    // Chords
    ChordPanel chordPanel;

    // Groove panel
    ParamCombo sectionCombo, fillsCombo, variationCombo, fillStyleCombo;
    juce::TextButton bigStrumButton { "BIG STRUM" }, newFeelButton { "NEW FEEL" };
    Knob swing, humanize, velocity, accents;

    // Guide sound & MIDI out
    juce::ToggleButton soundToggle { "Guide sound" };
    ParamCombo kitCombo, channelCombo;
    juce::Slider strumSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> strumAttachment;
    Knob tone, reverb, volume;

    // Edit
    juce::TextButton gridTab { "STEP GRID" }, velocityTab { "VELOCITY" }, mixerTab { "MIXER" }, fillTab { "FILL" }, clearButton { "CLEAR" },
                     myGroovesButton { "MY GROOVES" }, importButton { "IMPORT MIDI" }, templateButton { "TEMPLATE..." }, timingButton { "TIMING..." };
    IconButton newRhythmButton { IconButton::Icon::dice };
    GuitarGrid grid;
    VelocityView velocityView;
    MixerView mixerView;
    FillView fillView;
    int viewMode = 0;

    // Export
    juce::ComboBox notesBox, lengthBox;
    DragTile dragTile;
    juce::TextButton saveButton { "SAVE .MID..." }, songButton { "FULL SONG..." }, wavButton { "SAVE .WAV..." };
    juce::TextButton linkButton { "LINK" };
    std::unique_ptr<juce::FileChooser> chooser;

    SongOverlay songOverlay { processor };
    GrooveBrowser browser { processor };
    OptionsOverlay options;
    UpdateOverlay updateOverlay;
    Updater::Checker updateChecker;
    std::vector<ParamCombo*> combos;
    juce::Rectangle<int> tempoArea, logoArea, displayArea;
    std::array<float, fgg::numLanes> lights {};

    // Undo history: a snapshot twice a second; a change pushes the previous one.
    juce::StringArray undoStack, redoStack;
    juce::String lastSnapshot;
    int snapshotTicks = 0;

    juce::Rectangle<int> sizeBeforeFullScreen;
    juce::String message;
    int messageTicks = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FaseGrooveGuitarEditor)
};

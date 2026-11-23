#pragma once

#include "LyricsModel.h"
#include "LyricsService.h"
#include "LyricsViewport.h"
#include "PluginProcessor.h"
#include "Theme.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <memory>
#include <vector>

class StudioLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void setPalette(const theme::Palette& p);
    const theme::Palette& getPalette() const noexcept { return palette; }

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;
    void fillTextEditorBackground(juce::Graphics&, int width, int height, juce::TextEditor&) override;
    void drawTextEditorOutline(juce::Graphics&, int width, int height, juce::TextEditor&) override;
    void drawScrollbar(juce::Graphics&, juce::ScrollBar&, int x, int y, int width, int height,
                       bool isScrollbarVertical, int thumbStartPosition, int thumbSize,
                       bool isMouseOver, bool isMouseDown) override;

private:
    theme::Palette palette = theme::darkPalette();
};

class LyricsDisplayContent : public juce::Component,
                             private juce::Timer
{
public:
    explicit LyricsDisplayContent(LyricsDisplayAudioProcessor& p);
    ~LyricsDisplayContent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    class CandidatesListModel : public juce::ListBoxModel
    {
    public:
        explicit CandidatesListModel(LyricsDisplayContent& ownerRef) : owner(ownerRef) {}
        int getNumRows() override;
        void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
        void listBoxItemClicked(int row, const juce::MouseEvent&) override;

    private:
        LyricsDisplayContent& owner;
    };

    void timerCallback() override;

    void applyTheme();
    void syncFromProcessorState();
    void updateMetadataUi();
    void updateOffsetButtonText();
    void setStatusMessage(const juce::String& text, bool isError = false);

    // Lyrics actions
    void startLrclibSearch(const juce::String& query, double expectedDurationSec, bool autoPickBest);
    void applySelectedLyrics(const LyricsRecord& record);
    void nudgeOffset(double deltaSec);
    void resetOffset();
    void enterEditMode();
    void exitEditMode(bool saveChanges);
    void importLyricsFile();
    void exportLyricsFile();

    double getEffectivePlaybackTimeSec() const noexcept;

    LyricsDisplayAudioProcessor& processor;
    StudioLookAndFeel lookAndFeel;
    theme::Palette palette = theme::darkPalette();

    juce::ThreadPool workerPool { 2 };
    std::unique_ptr<juce::FileChooser> activeFileChooser;

    // Header controls
    juce::Label brandLabel;
    juce::TextButton stayOpenBtn { "" };
    juce::TextButton fontMinusBtn { "A-" };
    juce::TextButton fontPlusBtn { "A+" };
    juce::TextButton themeToggleBtn { "Light" };

    // Search & action bar
    juce::TextEditor searchInput;
    juce::TextButton findBtn { "Find Lyrics" };
    juce::TextButton importBtn { "Import" };
    juce::TextButton exportBtn { "Export" };
    juce::TextButton editBtn { "Edit" };
    juce::TextButton offsetMinusBtn { "-0.5s" };
    juce::TextButton offsetResetBtn { "0.0s" };
    juce::TextButton offsetPlusBtn { "+0.5s" };

    // Candidate results drawer
    bool showCandidates = false;
    std::vector<LyricsRecord> candidates;
    CandidatesListModel candidatesModel { *this };
    juce::ListBox candidatesList { "Candidates", &candidatesModel };
    juce::TextButton closeCandidatesBtn { "Close" };

    // Main lyrics display & raw editor
    LyricsViewport lyricsViewport;
    bool isEditing = false;
    juce::TextEditor rawEditor;
    juce::TextButton rawSaveBtn { "Save Lyrics" };
    juce::TextButton rawCancelBtn { "Cancel" };

    // Preview transport bar (Standalone / manual audition)
    juce::TextButton previewPlayBtn { "Play" };
    juce::TextButton previewRestartBtn { "0:00" };
    juce::Slider previewSlider;
    bool previewPlaying = false;
    double previewTimeSec = 0.0;
    double lastTickMs = 0.0;

    // Status & state sync
    juce::String statusMessage;
    bool statusIsError = false;
    juce::uint64 lastSyncedLyricsVersion = 0;
    juce::uint64 lastSyncedStateVersion = 0;
    juce::uint64 searchToken = 0;
    juce::String lastKnownHostTrackName;
    bool lastAppliedDarkTheme = true;

    // Cached layout rects for painting badges/dividers
    juce::Rectangle<int> headerArea;
    juce::Rectangle<int> transportPillRect;
    juce::Rectangle<int> metaStripArea;
    juce::Rectangle<int> candidatesHeaderRect;
    juce::Rectangle<int> previewBarArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LyricsDisplayContent)
};

class FloatingWindowManager : private juce::Timer
{
public:
    explicit FloatingWindowManager(LyricsDisplayAudioProcessor& p);
    ~FloatingWindowManager() override;

    void onHostEditorOpened(juce::AudioProcessorEditor* editor);
    void onHostEditorClosed(juce::AudioProcessorEditor* editor, juce::Rectangle<int> lastScreenBounds);
    void onStayOpenChanged(bool enabled);
    void shutdown();

private:
    class PinnedWindow;

    void timerCallback() override;
    void ensureContentCreated();
    void attachContentToHost();
    void showFloatingWindow();
    void hideFloatingWindow(bool rememberBounds);
    void handleFloatingClosedByUser();

    LyricsDisplayAudioProcessor& processor;
    juce::Component::SafePointer<juce::AudioProcessorEditor> hostEditor;
    std::unique_ptr<LyricsDisplayContent> sharedContent;
    std::unique_ptr<PinnedWindow> floatingWindow;
    juce::Rectangle<int> savedBounds;

    bool userDismissedFloating = false;
    bool shuttingDown = false;
    bool lastHostVisible = false;
    std::shared_ptr<bool> aliveToken = std::make_shared<bool>(true);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FloatingWindowManager)
};

class LyricsDisplayAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit LyricsDisplayAudioProcessorEditor(LyricsDisplayAudioProcessor&);
    ~LyricsDisplayAudioProcessorEditor() override;

    void resized() override;

private:
    LyricsDisplayAudioProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LyricsDisplayAudioProcessorEditor)
};

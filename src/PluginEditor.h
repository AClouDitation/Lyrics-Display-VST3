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

    enum class ActiveTab { Lyrics, Info };

    void timerCallback() override;

    void applyTheme();
    void setActiveTab(ActiveTab tab);
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

    // Info / GitHub Release Updater actions
    void checkForGithubUpdate(bool pullAfterCheck = false);
    void pullLatestGithubRelease();
    void setUpdateStatus(const juce::String& text, bool isError = false, double progress = -1.0);
    void updateInfoButtonsState();

    double getEffectivePlaybackTimeSec() const noexcept;

    LyricsDisplayAudioProcessor& processor;
    StudioLookAndFeel lookAndFeel;
    theme::Palette palette = theme::darkPalette();
    ActiveTab activeTab = ActiveTab::Lyrics;

    juce::ThreadPool workerPool { 2 };
    std::unique_ptr<juce::FileChooser> activeFileChooser;

    // Header controls
    juce::Label brandLabel;
    juce::TextButton tabLyricsBtn { "Lyrics" };
    juce::TextButton tabInfoBtn { "Info" };
    juce::TextButton stayOpenBtn { "" };
    juce::TextButton fontMinusBtn { "A-" };
    juce::TextButton fontPlusBtn { "A+" };
    juce::TextButton themeToggleBtn { "Light" };

    // Info tab controls
    juce::TextButton openGithubBtn { "Open GitHub" };
    juce::TextButton openReleasesBtn { "Releases Page" };
    juce::TextButton checkUpdateBtn { "Check Updates" };
    juce::TextButton pullReleaseBtn { "Pull Latest Release" };
    juce::TextButton openDownloadDirBtn { "Open Folder" };
    juce::TextEditor releaseNotesBox;

    GithubReleaseInfo latestRelease;
    bool hasCheckedRelease = false;
    bool isUpdatingRelease = false;
    double updateProgress01 = -1.0;
    juce::String updateStatusText { "Click \"Check Updates\" or \"Pull Latest Release\" to check GitHub." };
    bool updateStatusIsError = false;
    juce::File lastDownloadedReleaseDir;
    juce::uint64 updateToken = 0;

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
    juce::Rectangle<int> infoAboutCardRect;
    juce::Rectangle<int> infoUpdateCardRect;
    juce::Rectangle<int> infoProgressRect;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LyricsDisplayContent)
};

class LyricsDisplayAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit LyricsDisplayAudioProcessorEditor(LyricsDisplayAudioProcessor&);
    ~LyricsDisplayAudioProcessorEditor() override;

    void resized() override;
    void parentHierarchyChanged() override;

private:
    void timerCallback() override;
    void attachHostPinHook();
    void detachHostPinHook();

    LyricsDisplayAudioProcessor& processor;
    LyricsDisplayContent content;

    void* subclassedHostHwnd = nullptr;
    bool allowHostHide = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LyricsDisplayAudioProcessorEditor)
};

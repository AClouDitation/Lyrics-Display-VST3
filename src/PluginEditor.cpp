#include "PluginEditor.h"

#include <cmath>

#if JUCE_WINDOWS
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
 #include <commctrl.h>
#endif

namespace
{
#if JUCE_WINDOWS
constexpr UINT_PTR kHostPinSubclassId = 0x4C595244; // 'LYRD'
#endif
} // namespace

//==============================================================================
// StudioLookAndFeel
//==============================================================================

void StudioLookAndFeel::setPalette(const theme::Palette& p)
{
    palette = p;
    setColour(juce::ResizableWindow::backgroundColourId, p.bg);
    setColour(juce::TextEditor::backgroundColourId, p.surfaceAlt);
    setColour(juce::TextEditor::textColourId, p.text);
    setColour(juce::TextEditor::outlineColourId, p.border);
    setColour(juce::TextEditor::focusedOutlineColourId, p.accent);
    setColour(juce::TextEditor::highlightColourId, p.accentGlow);
    setColour(juce::TextEditor::highlightedTextColourId, p.text);
    setColour(juce::CaretComponent::caretColourId, p.accent);
    setColour(juce::ListBox::backgroundColourId, p.surface);
    setColour(juce::ListBox::outlineColourId, p.border);
    setColour(juce::Slider::trackColourId, p.accent);
    setColour(juce::Slider::backgroundColourId, p.surfaceAlt);
    setColour(juce::Slider::thumbColourId, p.text);
    setColour(juce::Label::textColourId, p.text);
    setColour(juce::TextButton::buttonColourId, p.surfaceAlt);
    setColour(juce::TextButton::buttonOnColourId, p.accentSoft);
    setColour(juce::TextButton::textColourOffId, p.text);
    setColour(juce::TextButton::textColourOnId, p.accent);
}

juce::Font StudioLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return theme::makeUiFont(juce::jlimit(11.5f, 13.5f, static_cast<float>(buttonHeight) * 0.48f), true);
}

void StudioLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                             const juce::Colour&,
                                             bool isHighlighted, bool isDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    const bool isPrimary = button.getProperties().getWithDefault("primary", false);
    const bool isActive = button.getToggleState();

    juce::Colour fill = isPrimary ? palette.accent
                                  : (isActive ? palette.accentSoft : palette.surfaceAlt);

    if (!button.isEnabled())
        fill = fill.withAlpha(0.45f);
    else if (isDown)
        fill = fill.darker(0.14f);
    else if (isHighlighted)
        fill = isPrimary ? fill.brighter(0.10f) : palette.surfaceHover;

    g.setColour(fill);
    g.fillRoundedRectangle(bounds, 6.0f);

    g.setColour(isPrimary ? palette.accent.brighter(0.15f)
                          : (isActive ? palette.accent : palette.border));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
}

void StudioLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                       bool isHighlighted, bool)
{
    const bool isPrimary = button.getProperties().getWithDefault("primary", false);
    const bool isPinIcon = button.getProperties().getWithDefault("pinIcon", false);
    const bool isActive = button.getToggleState();

    if (isPinIcon)
    {
        juce::Colour pinCol = isActive ? palette.accent : (isHighlighted ? palette.text : palette.textMuted);
        if (!button.isEnabled())
            pinCol = pinCol.withAlpha(0.45f);

        const auto bounds = button.getLocalBounds().toFloat();
        const float iconSize = std::min(bounds.getWidth(), bounds.getHeight()) * 0.62f;
        const float scale = iconSize / 24.0f;

        juce::Path head;
        head.startNewSubPath(8.5f, 4.0f);
        head.lineTo(15.5f, 4.0f);
        head.lineTo(14.5f, 6.0f);
        head.lineTo(15.2f, 11.2f);
        head.lineTo(18.0f, 13.8f);
        head.lineTo(6.0f, 13.8f);
        head.lineTo(8.8f, 11.2f);
        head.lineTo(9.5f, 6.0f);
        head.closeSubPath();

        juce::Path needle;
        needle.startNewSubPath(12.0f, 13.8f);
        needle.lineTo(12.0f, 20.8f);

        auto transform = juce::AffineTransform::translation(-12.0f, -12.0f);
        if (!isActive)
            transform = transform.rotated(juce::MathConstants<float>::pi * 0.25f);
        transform = transform.scaled(scale).translated(bounds.getCentreX(), bounds.getCentreY());

        head.applyTransform(transform);
        needle.applyTransform(transform);

        g.setColour(pinCol);
        if (isActive)
        {
            g.fillPath(head);
            g.strokePath(head, juce::PathStrokeType(1.1f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
        }
        else
        {
            g.strokePath(head, juce::PathStrokeType(1.5f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
        }
        g.strokePath(needle, juce::PathStrokeType(1.75f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
        return;
    }

    juce::Colour textCol = isPrimary ? juce::Colours::white
                                     : (isActive ? palette.accent : palette.text);
    if (!button.isEnabled())
        textCol = textCol.withAlpha(0.45f);

    g.setFont(getTextButtonFont(button, button.getHeight()));
    g.setColour(textCol);
    g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(6, 2),
                     juce::Justification::centred, 1);
}

void StudioLookAndFeel::fillTextEditorBackground(juce::Graphics& g, int width, int height, juce::TextEditor&)
{
    g.setColour(palette.surfaceAlt);
    g.fillRoundedRectangle(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 6.0f);
}

void StudioLookAndFeel::drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& editor)
{
    g.setColour(editor.hasKeyboardFocus(true) ? palette.accent : palette.border);
    g.drawRoundedRectangle(0.5f, 0.5f, static_cast<float>(width) - 1.0f, static_cast<float>(height) - 1.0f, 6.0f, 1.0f);
}

void StudioLookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar&, int x, int y, int width, int height,
                                      bool isVertical, int thumbStart, int thumbSize, bool isOver, bool isDown)
{
    if (thumbSize <= 0)
        return;

    juce::Rectangle<float> thumb = isVertical
        ? juce::Rectangle<float>(static_cast<float>(x) + 2.0f, static_cast<float>(thumbStart),
                                 static_cast<float>(width) - 4.0f, static_cast<float>(thumbSize))
        : juce::Rectangle<float>(static_cast<float>(thumbStart), static_cast<float>(y) + 2.0f,
                                 static_cast<float>(thumbSize), static_cast<float>(height) - 4.0f);

    const float alpha = isDown ? 0.7f : (isOver ? 0.5f : 0.3f);
    g.setColour(palette.textDim.withAlpha(alpha));
    g.fillRoundedRectangle(thumb, 3.0f);
}

//==============================================================================
// CandidatesListModel
//==============================================================================

int LyricsDisplayContent::CandidatesListModel::getNumRows()
{
    return std::min(12, static_cast<int>(owner.candidates.size()));
}

void LyricsDisplayContent::CandidatesListModel::paintListBoxItem(
    int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= static_cast<int>(owner.candidates.size()))
        return;

    const auto& cand = owner.candidates[static_cast<size_t>(rowNumber)];
    auto rowRect = juce::Rectangle<float>(4.0f, 2.0f, static_cast<float>(width) - 8.0f, static_cast<float>(height) - 4.0f);

    if (rowIsSelected)
    {
        g.setColour(owner.palette.accentSoft);
        g.fillRoundedRectangle(rowRect, 6.0f);
    }

    // Right side: Synced/Plain pill + duration
    const bool synced = cand.hasSynced();
    const juce::Colour badgeCol = synced ? owner.palette.success : owner.palette.warning;
    auto badgeRect = juce::Rectangle<float>(static_cast<float>(width) - 66.0f, (static_cast<float>(height) - 18.0f) * 0.5f, 54.0f, 18.0f);
    g.setColour(badgeCol.withAlpha(0.18f));
    g.fillRoundedRectangle(badgeRect, 4.0f);
    g.setFont(theme::makeUiFont(10.5f, true));
    g.setColour(badgeCol);
    g.drawText(synced ? "Synced" : "Plain", badgeRect.toNearestInt(), juce::Justification::centred, false);

    int rightLimit = static_cast<int>(badgeRect.getX()) - 8;
    if (cand.hasDuration)
    {
        g.setFont(theme::makeMonoFont(11.0f, false));
        g.setColour(owner.palette.textDim);
        g.drawText(lyrics::formatTime(cand.duration),
                   juce::Rectangle<int>(rightLimit - 44, 0, 44, height),
                   juce::Justification::centredRight, false);
        rightLimit -= 50;
    }

    g.setFont(theme::makeUiFont(12.5f, true));
    g.setColour(owner.palette.text);
    const juce::String title = cand.displayLabel().isNotEmpty() ? cand.displayLabel() : "Untitled";
    g.drawText(title, juce::Rectangle<int>(12, 0, std::max(60, rightLimit - 12), height),
               juce::Justification::centredLeft, true);
}

void LyricsDisplayContent::CandidatesListModel::listBoxItemClicked(int row, const juce::MouseEvent&)
{
    if (row < 0 || row >= static_cast<int>(owner.candidates.size()))
        return;

    auto chosen = owner.candidates[static_cast<size_t>(row)];
    chosen.offsetSec = 0.0;
    owner.showCandidates = false;
    owner.applySelectedLyrics(chosen);
    owner.setStatusMessage("Loaded: " + chosen.displayLabel());
    owner.resized();
    owner.repaint();
}

//==============================================================================
// LyricsDisplayContent
//==============================================================================

LyricsDisplayContent::LyricsDisplayContent(LyricsDisplayAudioProcessor& p)
    : processor(p)
{
    setLookAndFeel(&lookAndFeel);

    // Header
    brandLabel.setText("LYRICS DISPLAY", juce::dontSendNotification);
    brandLabel.setFont(theme::makeUiFont(14.0f, true));
    addAndMakeVisible(brandLabel);

    tabLyricsBtn.setToggleState(true, juce::dontSendNotification);
    tabLyricsBtn.onClick = [this] { setActiveTab(ActiveTab::Lyrics); };
    addAndMakeVisible(tabLyricsBtn);

    tabInfoBtn.setToggleState(false, juce::dontSendNotification);
    tabInfoBtn.onClick = [this] { setActiveTab(ActiveTab::Info); };
    addAndMakeVisible(tabInfoBtn);

    stayOpenBtn.getProperties().set("pinIcon", true);
    stayOpenBtn.setClickingTogglesState(true);
    auto updatePinTooltip = [this](bool isPinned) {
        stayOpenBtn.setTooltip(isPinned
            ? "Pinned: stays open across all tracks (click to unpin)"
            : "Unpinned: click to keep lyrics visible across all tracks");
    };
    stayOpenBtn.setToggleState(processor.isStayOpen(), juce::dontSendNotification);
    updatePinTooltip(processor.isStayOpen());
    stayOpenBtn.onClick = [this, updatePinTooltip] {
        const bool next = stayOpenBtn.getToggleState();
        updatePinTooltip(next);
        processor.setStayOpen(next);
        if (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
        {
            if (auto* top = getTopLevelComponent())
                top->setAlwaysOnTop(next);
        }
    };
    addAndMakeVisible(stayOpenBtn);

    fontMinusBtn.setTooltip("Decrease lyrics font size");
    fontMinusBtn.onClick = [this] {
        const float next = juce::jlimit(13.0f, 38.0f, processor.getFontSize() - 2.0f);
        processor.setFontSize(next);
        lyricsViewport.setFontSize(next);
    };
    addAndMakeVisible(fontMinusBtn);

    fontPlusBtn.setTooltip("Increase lyrics font size");
    fontPlusBtn.onClick = [this] {
        const float next = juce::jlimit(13.0f, 38.0f, processor.getFontSize() + 2.0f);
        processor.setFontSize(next);
        lyricsViewport.setFontSize(next);
    };
    addAndMakeVisible(fontPlusBtn);

    themeToggleBtn.setTooltip("Switch Dark / Light theme");
    themeToggleBtn.onClick = [this] {
        processor.setDarkTheme(!processor.isDarkTheme());
        applyTheme();
    };
    addAndMakeVisible(themeToggleBtn);

    // Search & action bar
    searchInput.setFont(theme::makeUiFont(13.5f, false));
    searchInput.setTextToShowWhenEmpty("Search song title or artist on LRCLIB...", palette.textDim);
    searchInput.setReturnKeyStartsNewLine(false);
    searchInput.onReturnKey = [this] {
        const auto q = searchInput.getText().trim();
        if (q.isNotEmpty())
            startLrclibSearch(q, processor.getLyricsRecord().duration, false);
    };
    addAndMakeVisible(searchInput);

    findBtn.getProperties().set("primary", true);
    findBtn.onClick = [this] {
        const auto q = searchInput.getText().trim();
        if (q.isNotEmpty())
            startLrclibSearch(q, processor.getLyricsRecord().duration, false);
    };
    addAndMakeVisible(findBtn);

    importBtn.setTooltip("Import a local .lrc, .txt, or lyrics.json file");
    importBtn.onClick = [this] { importLyricsFile(); };
    addAndMakeVisible(importBtn);

    exportBtn.setTooltip("Export current lyrics as .lrc or .txt");
    exportBtn.onClick = [this] { exportLyricsFile(); };
    addAndMakeVisible(exportBtn);

    editBtn.setTooltip("Edit raw LRC / plain lyrics (or double-click lyrics viewport)");
    editBtn.onClick = [this] {
        if (isEditing)
            exitEditMode(false);
        else
            enterEditMode();
    };
    addAndMakeVisible(editBtn);

    offsetMinusBtn.setTooltip("Shift lyrics 0.5s earlier");
    offsetMinusBtn.onClick = [this] { nudgeOffset(-0.5); };
    addAndMakeVisible(offsetMinusBtn);

    offsetResetBtn.setTooltip("Click to reset timing offset to 0.0s");
    offsetResetBtn.onClick = [this] { resetOffset(); };
    addAndMakeVisible(offsetResetBtn);

    offsetPlusBtn.setTooltip("Shift lyrics 0.5s later");
    offsetPlusBtn.onClick = [this] { nudgeOffset(0.5); };
    addAndMakeVisible(offsetPlusBtn);

    // Candidates list
    candidatesList.setRowHeight(32);
    candidatesList.setOutlineThickness(1);
    addChildComponent(candidatesList);

    closeCandidatesBtn.onClick = [this] {
        showCandidates = false;
        resized();
        repaint();
    };
    addChildComponent(closeCandidatesBtn);

    // Karaoke viewport
    lyricsViewport.setFontSize(processor.getFontSize());
    lyricsViewport.onLineClicked = [this](double timeSec) {
        const auto transport = processor.getTransportSnapshot();
        if (!transport.hasHostTransport || !transport.isPlaying)
        {
            const double off = processor.getLyricsRecord().offsetSec;
            previewTimeSec = std::max(0.0, timeSec - off);
            previewSlider.setValue(previewTimeSec, juce::dontSendNotification);
            lyricsViewport.updatePlaybackTime(previewTimeSec, off, true);
        }
    };
    lyricsViewport.onRequestEdit = [this] { enterEditMode(); };
    addAndMakeVisible(lyricsViewport);

    // Raw lyrics editor
    rawEditor.setMultiLine(true, true);
    rawEditor.setReturnKeyStartsNewLine(true);
    rawEditor.setScrollbarsShown(true);
    rawEditor.setFont(theme::makeUiFont(14.0f, false));
    rawEditor.setTextToShowWhenEmpty("Paste or edit lyrics here (supports [mm:ss.xx] LRC timestamps or plain text)...", palette.textDim);
    rawEditor.onEscapeKey = [this] { exitEditMode(false); };
    addChildComponent(rawEditor);

    rawSaveBtn.getProperties().set("primary", true);
    rawSaveBtn.onClick = [this] { exitEditMode(true); };
    addChildComponent(rawSaveBtn);

    rawCancelBtn.onClick = [this] { exitEditMode(false); };
    addChildComponent(rawCancelBtn);

    // Standalone / preview transport bar
    previewPlayBtn.onClick = [this] {
        previewPlaying = !previewPlaying;
        previewPlayBtn.setButtonText(previewPlaying ? "Pause" : "Play");
        lastTickMs = juce::Time::getMillisecondCounterHiRes();
    };
    addChildComponent(previewPlayBtn);

    previewRestartBtn.setTooltip("Restart preview to 0:00");
    previewRestartBtn.onClick = [this] {
        previewTimeSec = 0.0;
        previewSlider.setValue(0.0, juce::dontSendNotification);
        lyricsViewport.updatePlaybackTime(0.0, processor.getLyricsRecord().offsetSec, true);
    };
    addChildComponent(previewRestartBtn);

    previewSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    previewSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    previewSlider.setRange(0.0, 300.0, 0.05);
    previewSlider.onValueChange = [this] {
        previewTimeSec = previewSlider.getValue();
        lyricsViewport.updatePlaybackTime(previewTimeSec, processor.getLyricsRecord().offsetSec, true);
    };
    addChildComponent(previewSlider);

    // Info tab controls
    openGithubBtn.setTooltip(lyrics_service::kGithubRepoUrl);
    openGithubBtn.onClick = [] {
        juce::URL(lyrics_service::kGithubRepoUrl).launchInDefaultBrowser();
    };
    addChildComponent(openGithubBtn);

    openReleasesBtn.setTooltip("Open GitHub Releases page in your browser");
    openReleasesBtn.onClick = [this] {
        const juce::String target = (hasCheckedRelease && latestRelease.htmlUrl.isNotEmpty())
            ? latestRelease.htmlUrl
            : (juce::String(lyrics_service::kGithubRepoUrl) + "/releases");
        juce::URL(target).launchInDefaultBrowser();
    };
    addChildComponent(openReleasesBtn);

    checkUpdateBtn.setTooltip("Check GitHub for the latest release version");
    checkUpdateBtn.onClick = [this] { checkForGithubUpdate(false); };
    addChildComponent(checkUpdateBtn);

    pullReleaseBtn.getProperties().set("primary", true);
    pullReleaseBtn.setTooltip("Download the latest release from GitHub into your Downloads folder");
    pullReleaseBtn.onClick = [this] {
        if (hasCheckedRelease && latestRelease.ok && latestRelease.downloadUrl.isNotEmpty())
            pullLatestGithubRelease();
        else
            checkForGithubUpdate(true);
    };
    addChildComponent(pullReleaseBtn);

    openDownloadDirBtn.setEnabled(false);
    openDownloadDirBtn.setTooltip("Reveal the downloaded release folder in File Explorer");
    openDownloadDirBtn.onClick = [this] {
        if (lastDownloadedReleaseDir.isDirectory())
            lastDownloadedReleaseDir.revealToUser();
    };
    addChildComponent(openDownloadDirBtn);

    releaseNotesBox.setMultiLine(true, true);
    releaseNotesBox.setReadOnly(true);
    releaseNotesBox.setScrollbarsShown(true);
    releaseNotesBox.setCaretVisible(false);
    releaseNotesBox.setFont(theme::makeUiFont(13.0f, false));
    releaseNotesBox.setText("Repository: " + juce::String(lyrics_service::kGithubRepoUrl)
                            + "\n\nClick \"Check Updates\" to fetch the latest GitHub release notes, or \"Pull Latest Release\" to download the latest build directly into your Downloads folder.",
                            false);
    addChildComponent(releaseNotesBox);

    applyTheme();
    syncFromProcessorState();
    setActiveTab(ActiveTab::Lyrics);

    lastTickMs = juce::Time::getMillisecondCounterHiRes();
    startTimerHz(60);
}

LyricsDisplayContent::~LyricsDisplayContent()
{
    stopTimer();
    workerPool.removeAllJobs(true, 1000);
    setLookAndFeel(nullptr);
}

void LyricsDisplayContent::applyTheme()
{
    const bool dark = processor.isDarkTheme();
    lastAppliedDarkTheme = dark;
    palette = dark ? theme::darkPalette() : theme::lightPalette();
    lookAndFeel.setPalette(palette);
    lyricsViewport.setPalette(palette);

    themeToggleBtn.setButtonText(dark ? "Light" : "Dark");
    brandLabel.setColour(juce::Label::textColourId, palette.text);

    auto restyleEditor = [this](juce::TextEditor& ed) {
        ed.setColour(juce::TextEditor::backgroundColourId, palette.surfaceAlt);
        ed.setColour(juce::TextEditor::textColourId, palette.text);
        ed.setColour(juce::TextEditor::outlineColourId, palette.border);
        ed.setColour(juce::TextEditor::focusedOutlineColourId, palette.accent);
        ed.applyColourToAllText(palette.text);
    };
    restyleEditor(searchInput);
    restyleEditor(rawEditor);
    restyleEditor(releaseNotesBox);

    candidatesList.setColour(juce::ListBox::backgroundColourId, palette.surface);
    candidatesList.setColour(juce::ListBox::outlineColourId, palette.border);

    repaint();
    candidatesList.repaint();
}

void LyricsDisplayContent::setActiveTab(ActiveTab tab)
{
    activeTab = tab;
    const bool showLyricsTab = (activeTab == ActiveTab::Lyrics);
    const bool showInfoTab = !showLyricsTab;

    tabLyricsBtn.setToggleState(showLyricsTab, juce::dontSendNotification);
    tabInfoBtn.setToggleState(showInfoTab, juce::dontSendNotification);

    // Lyrics controls
    searchInput.setVisible(showLyricsTab);
    findBtn.setVisible(showLyricsTab);
    importBtn.setVisible(showLyricsTab);
    exportBtn.setVisible(showLyricsTab);
    editBtn.setVisible(showLyricsTab);
    offsetMinusBtn.setVisible(showLyricsTab);
    offsetResetBtn.setVisible(showLyricsTab);
    offsetPlusBtn.setVisible(showLyricsTab);

    lyricsViewport.setVisible(showLyricsTab && !isEditing);
    rawEditor.setVisible(showLyricsTab && isEditing);
    rawSaveBtn.setVisible(showLyricsTab && isEditing);
    rawCancelBtn.setVisible(showLyricsTab && isEditing);

    const bool showCands = showLyricsTab && showCandidates && !candidates.empty();
    candidatesList.setVisible(showCands);
    closeCandidatesBtn.setVisible(showCands);

    const bool showPreview = showLyricsTab && !processor.getTransportSnapshot().hasHostTransport;
    previewPlayBtn.setVisible(showPreview);
    previewRestartBtn.setVisible(showPreview);
    previewSlider.setVisible(showPreview);

    // Info controls
    openGithubBtn.setVisible(showInfoTab);
    openReleasesBtn.setVisible(showInfoTab);
    checkUpdateBtn.setVisible(showInfoTab);
    pullReleaseBtn.setVisible(showInfoTab);
    openDownloadDirBtn.setVisible(showInfoTab);
    releaseNotesBox.setVisible(showInfoTab);

    resized();
    repaint();
}

void LyricsDisplayContent::setUpdateStatus(const juce::String& text, bool isError, double progress)
{
    updateStatusText = text;
    updateStatusIsError = isError;
    updateProgress01 = progress;
    repaint(infoUpdateCardRect);
}

void LyricsDisplayContent::updateInfoButtonsState()
{
    checkUpdateBtn.setEnabled(!isUpdatingRelease);
    pullReleaseBtn.setEnabled(!isUpdatingRelease);
    openDownloadDirBtn.setEnabled(lastDownloadedReleaseDir.isDirectory());
}

void LyricsDisplayContent::checkForGithubUpdate(bool pullAfterCheck)
{
    if (isUpdatingRelease)
        return;

    isUpdatingRelease = true;
    const auto token = ++updateToken;
    updateInfoButtonsState();
    setUpdateStatus("Checking GitHub for the latest release...", false, -1.0);

    juce::Component::SafePointer<LyricsDisplayContent> safeThis(this);
    workerPool.addJob([safeThis, token, pullAfterCheck] {
        auto info = lyrics_service::fetchLatestGithubRelease();

        juce::MessageManager::callAsync([safeThis, token, info = std::move(info), pullAfterCheck]() mutable {
            if (safeThis == nullptr || token != safeThis->updateToken)
                return;

            safeThis->isUpdatingRelease = false;
            safeThis->hasCheckedRelease = true;
            safeThis->latestRelease = std::move(info);
            safeThis->updateInfoButtonsState();

            if (!safeThis->latestRelease.ok)
            {
                safeThis->setUpdateStatus(safeThis->latestRelease.errorMessage,
                                          !safeThis->latestRelease.noReleasesYet, -1.0);
                safeThis->releaseNotesBox.setText(
                    safeThis->latestRelease.errorMessage
                        + "\n\nRepository: " + juce::String(lyrics_service::kGithubRepoUrl),
                    false);
                safeThis->repaint();
                return;
            }

            const juce::String currentVer = JucePlugin_VersionString;
            const bool newer = lyrics::isNewerVersion(safeThis->latestRelease.tagName, currentVer);

            juce::String summary;
            summary << "Release: " << safeThis->latestRelease.releaseName
                    << " (" << safeThis->latestRelease.tagName << ")\n";
            if (safeThis->latestRelease.publishedAt.isNotEmpty())
                summary << "Published: " << safeThis->latestRelease.publishedAt.upToFirstOccurrenceOf("T", false, false) << "\n";
            if (safeThis->latestRelease.assetName.isNotEmpty())
            {
                summary << "Package: " << safeThis->latestRelease.assetName;
                if (safeThis->latestRelease.assetSizeBytes > 0)
                    summary << juce::String::formatted(" (%.2f MB)", static_cast<double>(safeThis->latestRelease.assetSizeBytes) / 1048576.0);
                summary << "\n";
            }
            if (safeThis->latestRelease.body.isNotEmpty())
                summary << "\n" << safeThis->latestRelease.body;
            safeThis->releaseNotesBox.setText(summary, false);

            if (pullAfterCheck)
            {
                safeThis->pullLatestGithubRelease();
                return;
            }

            if (newer)
                safeThis->setUpdateStatus("Update available: " + safeThis->latestRelease.tagName
                                          + " (installed: v" + currentVer + ")", false, -1.0);
            else
                safeThis->setUpdateStatus("You are up to date (latest: " + safeThis->latestRelease.tagName + ").", false, -1.0);

            safeThis->repaint();
        });
    });
}

void LyricsDisplayContent::pullLatestGithubRelease()
{
    if (isUpdatingRelease || !latestRelease.ok)
        return;

    isUpdatingRelease = true;
    const auto token = ++updateToken;
    const auto releaseCopy = latestRelease;
    updateInfoButtonsState();
    setUpdateStatus("Starting download of " + releaseCopy.tagName + "...", false, 0.02);

    juce::Component::SafePointer<LyricsDisplayContent> safeThis(this);
    workerPool.addJob([safeThis, token, releaseCopy] {
        auto dlRes = lyrics_service::downloadLatestGithubRelease(
            releaseCopy,
            [safeThis, token](double pct, const juce::String& stage) {
                juce::MessageManager::callAsync([safeThis, token, pct, stage] {
                    if (safeThis == nullptr || token != safeThis->updateToken)
                        return;
                    safeThis->setUpdateStatus(stage, false, pct);
                });
            });

        juce::MessageManager::callAsync([safeThis, token, dlRes = std::move(dlRes), releaseCopy]() mutable {
            if (safeThis == nullptr || token != safeThis->updateToken)
                return;

            safeThis->isUpdatingRelease = false;
            if (!dlRes.ok)
            {
                safeThis->updateInfoButtonsState();
                safeThis->setUpdateStatus(dlRes.errorMessage, true, -1.0);
                return;
            }

            safeThis->lastDownloadedReleaseDir = dlRes.outputDir;
            safeThis->updateInfoButtonsState();
            safeThis->setUpdateStatus(
                "Pulled " + releaseCopy.tagName + (dlRes.extracted ? " (extracted)" : "")
                    + " — close Ableton Live, replace Lyrics Display.vst3, and reopen Live.",
                false, 1.0);

            if (dlRes.outputDir.isDirectory())
                dlRes.outputDir.revealToUser();
        });
    });
}

void LyricsDisplayContent::syncFromProcessorState()
{
    lastSyncedStateVersion = processor.getStateVersion();

    if (processor.isDarkTheme() != lastAppliedDarkTheme)
        applyTheme();

    const bool pinned = processor.isStayOpen();
    stayOpenBtn.setToggleState(pinned, juce::dontSendNotification);
    stayOpenBtn.setTooltip(pinned
        ? "Pinned: stays open across all tracks (click to unpin)"
        : "Unpinned: click to keep lyrics visible across all tracks");
    if (std::abs(lyricsViewport.getFontSize() - processor.getFontSize()) > 0.1f)
        lyricsViewport.setFontSize(processor.getFontSize());

    const auto rec = processor.getLyricsRecord();
    const auto currentLyricsVer = processor.getLyricsVersion();
    if (currentLyricsVer != lastSyncedLyricsVersion)
    {
        lastSyncedLyricsVersion = currentLyricsVer;
        const auto lines = processor.getParsedLines();

        if (!isEditing)
            lyricsViewport.setLyrics(rec, lines);

        double maxDur = 240.0;
        if (rec.hasDuration && rec.duration > 10.0)
            maxDur = rec.duration;
        else if (!lines.empty())
            maxDur = std::max(60.0, lines.back().timeSec + 15.0);
        previewSlider.setRange(0.0, maxDur, 0.05);

        lyricsViewport.updatePlaybackTime(getEffectivePlaybackTimeSec(), rec.offsetSec, true);
    }

    const auto lastQ = processor.getLastSearchQuery();
    const auto hostName = processor.getHostTrackName();
    if (searchInput.getText().isEmpty())
    {
        if (lastQ.isNotEmpty())
            searchInput.setText(lastQ, false);
        else if (rec.trackName.isNotEmpty())
            searchInput.setText(rec.trackName, false);
        else if (hostName.isNotEmpty())
            searchInput.setText(lyrics::cleanTrackQuery(hostName), false);
    }

    if (hostName.isNotEmpty() && hostName != lastKnownHostTrackName)
    {
        lastKnownHostTrackName = hostName;
        if (rec.isEmpty() && searchInput.getText().isEmpty())
            searchInput.setText(lyrics::cleanTrackQuery(hostName), false);
    }

    updateMetadataUi();
    updateOffsetButtonText();
}

void LyricsDisplayContent::updateMetadataUi()
{
    const auto rec = processor.getLyricsRecord();
    const bool hasLyrics = !rec.isEmpty();
    exportBtn.setEnabled(hasLyrics);
    offsetMinusBtn.setEnabled(rec.hasSynced());
    offsetResetBtn.setEnabled(rec.hasSynced());
    offsetPlusBtn.setEnabled(rec.hasSynced());
    repaint(metaStripArea);
}

void LyricsDisplayContent::updateOffsetButtonText()
{
    const double off = processor.getLyricsRecord().offsetSec;
    const juce::String sign = off > 0.001 ? "+" : "";
    offsetResetBtn.setButtonText(sign + juce::String(off, 1) + "s");
}

void LyricsDisplayContent::setStatusMessage(const juce::String& text, bool isError)
{
    statusMessage = text;
    statusIsError = isError;
    repaint(metaStripArea);
}

double LyricsDisplayContent::getEffectivePlaybackTimeSec() const noexcept
{
    const auto transport = processor.getTransportSnapshot();
    if (transport.hasHostTransport)
        return transport.timeSec;
    return previewTimeSec;
}

void LyricsDisplayContent::timerCallback()
{
    if (processor.getStateVersion() != lastSyncedStateVersion)
        syncFromProcessorState();

    const double nowMs = juce::Time::getMillisecondCounterHiRes();
    const double dtSec = juce::jlimit(0.0, 0.2, (nowMs - lastTickMs) / 1000.0);
    lastTickMs = nowMs;

    const auto transport = processor.getTransportSnapshot();

    // Show preview bar only when running Standalone (no host transport) and on Lyrics tab
    const bool needPreviewBar = (activeTab == ActiveTab::Lyrics) && !transport.hasHostTransport;
    if (previewPlayBtn.isVisible() != needPreviewBar)
    {
        previewPlayBtn.setVisible(needPreviewBar);
        previewRestartBtn.setVisible(needPreviewBar);
        previewSlider.setVisible(needPreviewBar);
        resized();
    }

    if (!transport.hasHostTransport && previewPlaying)
    {
        previewTimeSec += dtSec;
        if (previewTimeSec > previewSlider.getMaximum())
        {
            previewTimeSec = 0.0;
            previewPlaying = false;
            previewPlayBtn.setButtonText("Play");
        }
        previewSlider.setValue(previewTimeSec, juce::dontSendNotification);
    }

    const double currentTime = getEffectivePlaybackTimeSec();
    const double offsetSec = processor.getLyricsRecord().offsetSec;
    if (!isEditing)
    {
        lyricsViewport.updatePlaybackTime(currentTime, offsetSec, false);
        lyricsViewport.tickAnimation();
    }

    repaint(transportPillRect);
}

void LyricsDisplayContent::startLrclibSearch(const juce::String& query, double expectedDurationSec, bool autoPickBest)
{
    const auto cleaned = query.trim();
    if (cleaned.isEmpty())
        return;

    processor.setLastSearchQuery(cleaned);
    const auto token = ++searchToken;
    setStatusMessage("Searching LRCLIB for \"" + cleaned + "\"...");
    if (autoPickBest)
        lyricsViewport.setPlaceholderMessage("Searching LRCLIB...", cleaned);

    juce::Component::SafePointer<LyricsDisplayContent> safeThis(this);
    workerPool.addJob([safeThis, token, cleaned, expectedDurationSec, autoPickBest] {
        auto result = lyrics_service::searchLrclib(cleaned, expectedDurationSec);

        juce::MessageManager::callAsync([safeThis, token, result = std::move(result), autoPickBest]() mutable {
            if (safeThis == nullptr || token != safeThis->searchToken)
                return;

            if (!result.ok)
            {
                safeThis->candidates.clear();
                safeThis->candidatesList.updateContent();
                safeThis->setStatusMessage(result.errorMessage, true);
                if (autoPickBest)
                    safeThis->lyricsViewport.setPlaceholderMessage("Search failed", result.errorMessage);
                return;
            }

            safeThis->candidates = std::move(result.candidates);
            safeThis->candidatesList.updateContent();

            if (safeThis->candidates.empty())
            {
                safeThis->showCandidates = false;
                safeThis->setStatusMessage("No LRCLIB matches found — try editing the title/artist or importing .lrc", true);
                if (autoPickBest)
                    safeThis->lyricsViewport.setPlaceholderMessage("No matches found",
                                                                   "Try searching with a different title/artist, import an .lrc file, or double-click to edit.");
                safeThis->resized();
                safeThis->repaint();
                return;
            }

            if (autoPickBest)
            {
                auto best = safeThis->candidates.front();
                best.offsetSec = 0.0;
                safeThis->showCandidates = false;
                safeThis->applySelectedLyrics(best);
                safeThis->setStatusMessage("Matched " + best.displayLabel()
                                           + " (" + juce::String(static_cast<int>(safeThis->candidates.size())) + " candidates)");
            }
            else
            {
                safeThis->showCandidates = true;
                safeThis->setStatusMessage("Found " + juce::String(static_cast<int>(safeThis->candidates.size()))
                                           + " results on LRCLIB — click one to load");
            }
            safeThis->resized();
            safeThis->repaint();
        });
    });
}

void LyricsDisplayContent::applySelectedLyrics(const LyricsRecord& record)
{
    isEditing = false;
    rawEditor.setVisible(false);
    rawSaveBtn.setVisible(false);
    rawCancelBtn.setVisible(false);
    lyricsViewport.setVisible(activeTab == ActiveTab::Lyrics);
    editBtn.setButtonText("Edit");

    processor.setLyricsRecord(record);
    syncFromProcessorState();

    resized();
    repaint();
}

void LyricsDisplayContent::nudgeOffset(double deltaSec)
{
    auto rec = processor.getLyricsRecord();
    if (!rec.hasSynced())
        return;

    const double next = std::round((rec.offsetSec + deltaSec) * 10.0) / 10.0;
    processor.setLyricsOffset(next);
    updateOffsetButtonText();
    lyricsViewport.updatePlaybackTime(getEffectivePlaybackTimeSec(), next, true);
    setStatusMessage("Timing offset: " + offsetResetBtn.getButtonText());
}

void LyricsDisplayContent::resetOffset()
{
    auto rec = processor.getLyricsRecord();
    if (!rec.hasSynced())
        return;

    processor.setLyricsOffset(0.0);
    updateOffsetButtonText();
    lyricsViewport.updatePlaybackTime(getEffectivePlaybackTimeSec(), 0.0, true);
    setStatusMessage("Timing offset reset to 0.0s");
}

void LyricsDisplayContent::enterEditMode()
{
    isEditing = true;
    const auto rec = processor.getLyricsRecord();
    rawEditor.setText(rec.hasSynced() ? rec.syncedLyrics : rec.plainLyrics, false);
    lyricsViewport.setVisible(false);
    rawEditor.setVisible(true);
    rawSaveBtn.setVisible(true);
    rawCancelBtn.setVisible(true);
    editBtn.setButtonText("Done");
    resized();
    rawEditor.grabKeyboardFocus();
}

void LyricsDisplayContent::exitEditMode(bool saveChanges)
{
    if (!isEditing)
        return;

    if (saveChanges)
    {
        const auto raw = rawEditor.getText().trim();
        const auto parsed = lyrics::parseLrc(raw);

        auto updated = processor.getLyricsRecord();
        if (updated.id.isEmpty())
            updated.id = "manual:" + juce::String(juce::Time::currentTimeMillis());
        if (updated.trackName.isEmpty())
            updated.trackName = parsed.title.isNotEmpty() ? parsed.title : searchInput.getText().trim();
        if (updated.artistName.isEmpty() && parsed.artist.isNotEmpty())
            updated.artistName = parsed.artist;
        if (updated.source.isEmpty())
            updated.source = "Manual";
        else if (!updated.source.containsIgnoreCase("Edited"))
            updated.source += " (Edited)";

        if (!parsed.lines.empty())
        {
            updated.syncedLyrics = raw;
            juce::StringArray plain;
            for (const auto& l : parsed.lines)
                plain.add(l.text);
            updated.plainLyrics = plain.joinIntoString("\n");
        }
        else
        {
            updated.syncedLyrics.clear();
            updated.plainLyrics = raw;
        }

        applySelectedLyrics(updated);
        setStatusMessage("Lyrics saved");
        return;
    }

    isEditing = false;
    rawEditor.setVisible(false);
    rawSaveBtn.setVisible(false);
    rawCancelBtn.setVisible(false);
    lyricsViewport.setVisible(true);
    editBtn.setButtonText("Edit");
    resized();
}

void LyricsDisplayContent::importLyricsFile()
{
    const auto startDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    activeFileChooser = std::make_unique<juce::FileChooser>(
        "Import Lyrics (.lrc, .txt, or lyrics.json)", startDir, "*.lrc;*.txt;*.json");

    const auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    activeFileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser) {
        const auto file = chooser.getResult();
        if (!file.existsAsFile())
            return;

        LyricsRecord rec;
        if (lyrics_service::loadLyricsFromFile(file, rec))
        {
            searchInput.setText(rec.trackName, false);
            applySelectedLyrics(rec);
            setStatusMessage("Imported " + file.getFileName());
        }
        else
        {
            setStatusMessage("Could not parse lyrics from " + file.getFileName(), true);
        }
    });
}

void LyricsDisplayContent::exportLyricsFile()
{
    const auto rec = processor.getLyricsRecord();
    if (rec.isEmpty())
        return;

    const juce::String ext = rec.hasSynced() ? ".lrc" : ".txt";
    const juce::String defaultName = (rec.trackName.isNotEmpty() ? rec.trackName : "lyrics") + ext;
    juce::File defaultTarget = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                                   .getChildFile(juce::File::createLegalFileName(defaultName));

    activeFileChooser = std::make_unique<juce::FileChooser>(
        "Export Lyrics", defaultTarget, "*.lrc;*.txt;*.json");

    const auto chooserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting;
    activeFileChooser->launchAsync(chooserFlags, [this, rec](const juce::FileChooser& chooser) {
        const auto file = chooser.getResult();
        if (file == juce::File())
            return;

        if (lyrics_service::exportLyricsToFile(file, rec))
            setStatusMessage("Exported to " + file.getFileName());
        else
            setStatusMessage("Failed to write " + file.getFileName(), true);
    });
}

void LyricsDisplayContent::mouseDown(const juce::MouseEvent&)
{
    // Release text editor focus so Ableton Live Spacebar play/pause works smoothly
    unfocusAllComponents();
}

void LyricsDisplayContent::resized()
{
    auto area = getLocalBounds().reduced(12);

    // Header bar
    headerArea = area.removeFromTop(34);
    auto hdr = headerArea;
    brandLabel.setBounds(hdr.removeFromLeft(126));
    hdr.removeFromLeft(6);
    tabLyricsBtn.setBounds(hdr.removeFromLeft(56).reduced(0, 3));
    hdr.removeFromLeft(4);
    tabInfoBtn.setBounds(hdr.removeFromLeft(48).reduced(0, 3));
    hdr.removeFromLeft(8);

    themeToggleBtn.setBounds(hdr.removeFromRight(56).reduced(0, 3));
    hdr.removeFromRight(5);
    fontPlusBtn.setBounds(hdr.removeFromRight(34).reduced(0, 3));
    hdr.removeFromRight(4);
    fontMinusBtn.setBounds(hdr.removeFromRight(34).reduced(0, 3));
    hdr.removeFromRight(6);
    stayOpenBtn.setBounds(hdr.removeFromRight(32).reduced(0, 3));
    hdr.removeFromRight(8);

    transportPillRect = hdr.reduced(0, 3);

    area.removeFromTop(8);

    if (activeTab == ActiveTab::Info)
    {
        metaStripArea = {};
        candidatesHeaderRect = {};
        previewBarArea = {};

        infoAboutCardRect = area.removeFromTop(104);
        auto aboutInner = infoAboutCardRect.reduced(16, 14);
        auto aboutRight = aboutInner.removeFromRight(120);
        openGithubBtn.setBounds(aboutRight.withSizeKeepingCentre(116, 32));

        area.removeFromTop(10);

        infoUpdateCardRect = area;
        auto updInner = infoUpdateCardRect.reduced(16, 14);
        updInner.removeFromTop(28); // card heading + version badges
        updInner.removeFromTop(8);

        auto btnRow = updInner.removeFromTop(32);
        pullReleaseBtn.setBounds(btnRow.removeFromLeft(152));
        btnRow.removeFromLeft(8);
        checkUpdateBtn.setBounds(btnRow.removeFromLeft(116));
        btnRow.removeFromLeft(8);
        openReleasesBtn.setBounds(btnRow.removeFromLeft(112));
        btnRow.removeFromLeft(8);
        openDownloadDirBtn.setBounds(btnRow.removeFromLeft(102));

        updInner.removeFromTop(10);
        infoProgressRect = updInner.removeFromTop(26);
        updInner.removeFromTop(8);
        releaseNotesBox.setBounds(updInner);
        return;
    }

    infoAboutCardRect = {};
    infoUpdateCardRect = {};
    infoProgressRect = {};

    // Search & action bar
    auto searchRow = area.removeFromTop(32);
    offsetPlusBtn.setBounds(searchRow.removeFromRight(44).reduced(0, 1));
    searchRow.removeFromRight(3);
    offsetResetBtn.setBounds(searchRow.removeFromRight(46).reduced(0, 1));
    searchRow.removeFromRight(3);
    offsetMinusBtn.setBounds(searchRow.removeFromRight(44).reduced(0, 1));
    searchRow.removeFromRight(8);

    editBtn.setBounds(searchRow.removeFromRight(48).reduced(0, 1));
    searchRow.removeFromRight(4);
    exportBtn.setBounds(searchRow.removeFromRight(56).reduced(0, 1));
    searchRow.removeFromRight(4);
    importBtn.setBounds(searchRow.removeFromRight(56).reduced(0, 1));
    searchRow.removeFromRight(8);

    findBtn.setBounds(searchRow.removeFromRight(86).reduced(0, 1));
    searchRow.removeFromRight(6);
    searchInput.setBounds(searchRow);

    area.removeFromTop(6);

    // Metadata & status strip
    metaStripArea = area.removeFromTop(24);
    area.removeFromTop(6);

    // Optional candidates list drawer
    if (showCandidates && !candidates.empty())
    {
        candidatesHeaderRect = area.removeFromTop(24);
        closeCandidatesBtn.setBounds(candidatesHeaderRect.withLeft(candidatesHeaderRect.getRight() - 56).reduced(0, 1));
        closeCandidatesBtn.setVisible(true);

        const int listH = std::min(168, std::max(68, static_cast<int>(candidates.size()) * 32 + 4));
        candidatesList.setBounds(area.removeFromTop(listH));
        candidatesList.setVisible(true);
        area.removeFromTop(8);
    }
    else
    {
        candidatesHeaderRect = {};
        closeCandidatesBtn.setVisible(false);
        candidatesList.setVisible(false);
    }

    // Optional bottom preview transport (Standalone only)
    if (previewPlayBtn.isVisible())
    {
        previewBarArea = area.removeFromBottom(36);
        area.removeFromBottom(6);
        auto pb = previewBarArea.reduced(8, 4);
        previewRestartBtn.setBounds(pb.removeFromLeft(48));
        pb.removeFromLeft(6);
        previewPlayBtn.setBounds(pb.removeFromLeft(62));
        pb.removeFromLeft(8);
        previewSlider.setBounds(pb);
    }
    else
    {
        previewBarArea = {};
    }

    // Main lyrics viewport OR raw editor
    if (isEditing)
    {
        auto edArea = area;
        auto actionRow = edArea.removeFromBottom(32);
        edArea.removeFromBottom(6);
        rawSaveBtn.setBounds(actionRow.removeFromRight(104));
        actionRow.removeFromRight(8);
        rawCancelBtn.setBounds(actionRow.removeFromRight(76));
        rawEditor.setBounds(edArea);
    }
    else
    {
        lyricsViewport.setBounds(area);
    }
}

void LyricsDisplayContent::paint(juce::Graphics& g)
{
    g.fillAll(palette.bg);

    // Transport pill in header
    if (!transportPillRect.isEmpty() && transportPillRect.getWidth() >= 90)
    {
        const auto tr = processor.getTransportSnapshot();
        const double timeSec = getEffectivePlaybackTimeSec();
        const bool activePlay = tr.hasHostTransport ? tr.isPlaying : previewPlaying;

        const juce::Colour dotCol = activePlay ? palette.success
                                               : (tr.hasHostTransport ? palette.accent : palette.textDim);
        const juce::String modeTxt = tr.hasHostTransport
            ? (tr.isPlaying ? "LIVE SYNC" : "LIVE PAUSED")
            : (previewPlaying ? "PREVIEW" : "STANDALONE");

        juce::String pillTxt = modeTxt + juce::String::fromUTF8("  •  ") + lyrics::formatTimePrecise(timeSec);
        if (tr.hasHostTransport && tr.bpm > 1.0 && transportPillRect.getWidth() >= 230)
            pillTxt += juce::String::fromUTF8("  •  ") + juce::String(tr.bpm, 1) + " BPM";

        auto pill = transportPillRect.toFloat();
        const float maxW = std::min(pill.getWidth(), 300.0f);
        auto drawPill = pill.withWidth(maxW);

        g.setColour(palette.surface);
        g.fillRoundedRectangle(drawPill, 14.0f);
        g.setColour(activePlay ? palette.success.withAlpha(0.55f) : palette.border);
        g.drawRoundedRectangle(drawPill, 14.0f, 1.0f);

        g.setColour(dotCol);
        g.fillEllipse(drawPill.getX() + 11.0f, drawPill.getCentreY() - 3.5f, 7.0f, 7.0f);

        g.setFont(theme::makeMonoFont(11.5f, true));
        g.setColour(palette.text);
        g.drawText(pillTxt, drawPill.withTrimmedLeft(24.0f).withTrimmedRight(10.0f).toNearestInt(),
                   juce::Justification::centredLeft, true);
    }

    // Metadata & status strip
    if (!metaStripArea.isEmpty())
    {
        auto strip = metaStripArea;
        const auto rec = processor.getLyricsRecord();

        if (!rec.isEmpty())
        {
            const bool synced = rec.hasSynced();
            const juce::String badgeText = (synced ? "Synced" : "Plain")
                + (rec.source.isNotEmpty() ? (juce::String::fromUTF8(" · ") + rec.source) : juce::String());
            const juce::Colour badgeCol = synced ? palette.success : palette.warning;

            const auto badgeFont = theme::makeUiFont(11.0f, true);
            const int badgeW = juce::jlimit(64, 180, static_cast<int>(std::ceil(juce::GlyphArrangement::getStringWidth(badgeFont, badgeText))) + 18);
            auto badgeRect = strip.removeFromLeft(badgeW).reduced(0, 2).toFloat();

            g.setColour(badgeCol.withAlpha(0.18f));
            g.fillRoundedRectangle(badgeRect, 5.0f);
            g.setFont(badgeFont);
            g.setColour(badgeCol);
            g.drawText(badgeText, badgeRect.toNearestInt(), juce::Justification::centred, true);

            strip.removeFromLeft(8);

            const juce::String label = rec.displayLabel();
            if (label.isNotEmpty())
            {
                g.setFont(theme::makeUiFont(12.5f, true));
                g.setColour(palette.text);
                const int labelW = std::min(strip.getWidth() / 2 + 40, 280);
                g.drawText(label, strip.removeFromLeft(labelW), juce::Justification::centredLeft, true);
                strip.removeFromLeft(8);
            }
        }

        if (statusMessage.isNotEmpty())
        {
            g.setFont(theme::makeUiFont(11.5f, false));
            g.setColour(statusIsError ? palette.danger : palette.textMuted);
            g.drawText(statusMessage, strip, juce::Justification::centredRight, true);
        }
    }

    // Candidates drawer header
    if (!candidatesHeaderRect.isEmpty())
    {
        g.setFont(theme::makeUiFont(11.5f, true));
        g.setColour(palette.textMuted);
        g.drawText("LRCLIB Matches (" + juce::String(static_cast<int>(candidates.size())) + ") — click to apply",
                   candidatesHeaderRect.withTrimmedRight(62), juce::Justification::centredLeft, true);
    }

    // Preview transport background
    if (!previewBarArea.isEmpty())
    {
        auto pb = previewBarArea.toFloat();
        g.setColour(palette.surface);
        g.fillRoundedRectangle(pb, 8.0f);
        g.setColour(palette.border);
        g.drawRoundedRectangle(pb.reduced(0.5f), 8.0f, 1.0f);
    }

    // Info tab cards
    if (activeTab == ActiveTab::Info)
    {
        if (!infoAboutCardRect.isEmpty())
        {
            auto card = infoAboutCardRect.toFloat();
            g.setColour(palette.surface);
            g.fillRoundedRectangle(card, 10.0f);
            g.setColour(palette.border);
            g.drawRoundedRectangle(card.reduced(0.5f), 10.0f, 1.0f);

            auto inner = infoAboutCardRect.reduced(16, 14).withTrimmedRight(130);
            auto titleRow = inner.removeFromTop(26);
            g.setFont(theme::makeUiFont(16.0f, true));
            g.setColour(palette.text);
            g.drawText("Lyrics Display", titleRow.removeFromLeft(130), juce::Justification::centredLeft, false);

            const juce::String verText = "v" + juce::String(JucePlugin_VersionString);
            auto verBadge = titleRow.removeFromLeft(58).reduced(0, 3).toFloat();
            g.setColour(palette.accentSoft);
            g.fillRoundedRectangle(verBadge, 5.0f);
            g.setFont(theme::makeMonoFont(11.0f, true));
            g.setColour(palette.accent);
            g.drawText(verText, verBadge.toNearestInt(), juce::Justification::centred, false);

            inner.removeFromTop(4);
            g.setFont(theme::makeUiFont(12.5f, false));
            g.setColour(palette.textMuted);
            g.drawText("Synchronized karaoke lyrics VST3 plugin for Ableton Live 11 & VST3 hosts.",
                       inner.removeFromTop(20), juce::Justification::centredLeft, true);

            inner.removeFromTop(4);
            g.setFont(theme::makeMonoFont(11.5f, false));
            g.setColour(palette.accent);
            g.drawText(lyrics_service::kGithubRepoUrl, inner.removeFromTop(18),
                       juce::Justification::centredLeft, true);
        }

        if (!infoUpdateCardRect.isEmpty())
        {
            auto card = infoUpdateCardRect.toFloat();
            g.setColour(palette.surface);
            g.fillRoundedRectangle(card, 10.0f);
            g.setColour(palette.border);
            g.drawRoundedRectangle(card.reduced(0.5f), 10.0f, 1.0f);

            auto headingRow = infoUpdateCardRect.reduced(16, 14).removeFromTop(24);
            g.setFont(theme::makeUiFont(14.5f, true));
            g.setColour(palette.text);
            g.drawText("GitHub Releases & Updates", headingRow.removeFromLeft(210),
                       juce::Justification::centredLeft, false);

            if (hasCheckedRelease && latestRelease.ok)
            {
                const bool newer = lyrics::isNewerVersion(latestRelease.tagName, JucePlugin_VersionString);
                const juce::String pillTxt = newer
                    ? ("Update: " + latestRelease.tagName)
                    : (juce::String::fromUTF8("Up to date · ") + latestRelease.tagName);
                const juce::Colour pillCol = newer ? palette.accent : palette.success;
                const auto pillFont = theme::makeUiFont(11.0f, true);
                const int pillW = juce::jlimit(90, 200, static_cast<int>(std::ceil(juce::GlyphArrangement::getStringWidth(pillFont, pillTxt))) + 20);
                auto pill = headingRow.removeFromRight(pillW).reduced(0, 2).toFloat();
                g.setColour(pillCol.withAlpha(0.18f));
                g.fillRoundedRectangle(pill, 5.0f);
                g.setFont(pillFont);
                g.setColour(pillCol);
                g.drawText(pillTxt, pill.toNearestInt(), juce::Justification::centred, true);
            }

            if (!infoProgressRect.isEmpty())
            {
                auto progArea = infoProgressRect;
                auto statusRow = progArea.removeFromTop(18);
                g.setFont(theme::makeUiFont(12.0f, false));
                g.setColour(updateStatusIsError ? palette.danger : palette.textMuted);
                g.drawText(updateStatusText, statusRow, juce::Justification::centredLeft, true);

                if (updateProgress01 >= 0.0)
                {
                    auto track = progArea.reduced(0, 1).toFloat();
                    track = track.withHeight(std::max(4.0f, track.getHeight()));
                    g.setColour(palette.surfaceAlt);
                    g.fillRoundedRectangle(track, 3.0f);
                    g.setColour(updateProgress01 >= 0.999 ? palette.success : palette.accent);
                    g.fillRoundedRectangle(track.withWidth(track.getWidth() * static_cast<float>(juce::jlimit(0.0, 1.0, updateProgress01))), 3.0f);
                }
            }
        }
    }
}

//==============================================================================
// LyricsDisplayAudioProcessorEditor (single panel instance with host pin hook)
//==============================================================================

#if JUCE_WINDOWS
namespace
{
struct HostPinHookContext
{
    LyricsDisplayAudioProcessor* processor = nullptr;
    bool* allowHostHide = nullptr;
    void** subclassedHostHwnd = nullptr;
};

LRESULT CALLBACK hostWindowSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                        UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    auto* ctx = reinterpret_cast<HostPinHookContext*>(dwRefData);

    if (msg == WM_NCDESTROY)
    {
        ::RemoveWindowSubclass(hwnd, &hostWindowSubclassProc, uIdSubclass);
        if (ctx != nullptr)
        {
            if (ctx->subclassedHostHwnd != nullptr && *ctx->subclassedHostHwnd == hwnd)
                *ctx->subclassedHostHwnd = nullptr;
            delete ctx;
        }
        return ::DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    if (ctx != nullptr && ctx->processor != nullptr && ctx->allowHostHide != nullptr)
    {
        if (msg == WM_CLOSE || msg == WM_DESTROY)
        {
            *ctx->allowHostHide = true;
        }
        else if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_NCLBUTTONDOWN)
        {
            // Allow clicking the host window's own title-bar close ('x') button to close normally
            POINT pt { static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam)) };
            if (msg != WM_NCLBUTTONDOWN)
                ::ClientToScreen(hwnd, &pt);

            RECT wr {};
            if (::GetWindowRect(hwnd, &wr))
            {
                const bool inTopBar = (pt.y >= wr.top && pt.y <= wr.top + 34);
                const bool inCloseBox = (pt.x >= wr.right - 38 && pt.x <= wr.right);
                if (inTopBar && inCloseBox)
                    *ctx->allowHostHide = true;
            }
        }
        else if (msg == WM_WINDOWPOSCHANGING && lParam != 0)
        {
            auto* wp = reinterpret_cast<WINDOWPOS*>(lParam);
            if ((wp->flags & SWP_HIDEWINDOW) != 0)
            {
                const bool pinOn = ctx->processor->isStayOpen();
                HWND owner = ::GetWindow(hwnd, GW_OWNER);
                const bool ownerMinimized = (owner != nullptr && ::IsIconic(owner) != FALSE);

                if (pinOn && !(*ctx->allowHostHide) && !ownerMinimized)
                {
                    // Prevent DAW Auto-Hide on track switch from hiding this single panel
                    wp->flags &= ~static_cast<UINT>(SWP_HIDEWINDOW);
                    wp->flags |= SWP_NOACTIVATE;
                }
                else
                {
                    *ctx->allowHostHide = false;
                }
            }
            else if ((wp->flags & SWP_SHOWWINDOW) != 0)
            {
                *ctx->allowHostHide = false;
            }
        }
        else if (msg == WM_SHOWWINDOW && wParam == FALSE && lParam == 0)
        {
            const bool pinOn = ctx->processor->isStayOpen();
            HWND owner = ::GetWindow(hwnd, GW_OWNER);
            const bool ownerMinimized = (owner != nullptr && ::IsIconic(owner) != FALSE);
            if (pinOn && !(*ctx->allowHostHide) && !ownerMinimized)
                return 0;
        }
    }

    return ::DefSubclassProc(hwnd, msg, wParam, lParam);
}
} // namespace
#endif

LyricsDisplayAudioProcessorEditor::LyricsDisplayAudioProcessorEditor(LyricsDisplayAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), content(p)
{
    setResizable(true, true);
    setResizeLimits(520, 380, 1920, 1200);
    setSize(740, 600);

    addAndMakeVisible(content);
    attachHostPinHook();
    startTimer(100);
}

LyricsDisplayAudioProcessorEditor::~LyricsDisplayAudioProcessorEditor()
{
    stopTimer();
    detachHostPinHook();
}

void LyricsDisplayAudioProcessorEditor::resized()
{
    content.setBounds(getLocalBounds());
}

void LyricsDisplayAudioProcessorEditor::parentHierarchyChanged()
{
    attachHostPinHook();
}

void LyricsDisplayAudioProcessorEditor::timerCallback()
{
    if (subclassedHostHwnd != nullptr)
    {
        stopTimer();
        return;
    }

    attachHostPinHook();
    if (subclassedHostHwnd != nullptr)
        stopTimer();
}

void LyricsDisplayAudioProcessorEditor::attachHostPinHook()
{
    if (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
    {
        if (auto* top = getTopLevelComponent())
            top->setAlwaysOnTop(processor.isStayOpen());
        stopTimer();
        return;
    }

#if JUCE_WINDOWS
    if (subclassedHostHwnd != nullptr)
        return;

    auto* peer = getPeer();
    if (peer == nullptr)
        return;

    auto* childHwnd = static_cast<HWND>(peer->getNativeHandle());
    if (childHwnd == nullptr)
        return;

    HWND rootHwnd = ::GetAncestor(childHwnd, GA_ROOT);
    if (rootHwnd == nullptr || rootHwnd == childHwnd)
        return;

    auto* ctx = new HostPinHookContext { &processor, &allowHostHide, &subclassedHostHwnd };
    if (::SetWindowSubclass(rootHwnd, &hostWindowSubclassProc, kHostPinSubclassId, reinterpret_cast<DWORD_PTR>(ctx)))
    {
        subclassedHostHwnd = rootHwnd;
    }
    else
    {
        delete ctx;
    }
#endif
}

void LyricsDisplayAudioProcessorEditor::detachHostPinHook()
{
#if JUCE_WINDOWS
    if (subclassedHostHwnd != nullptr)
    {
        auto* hwnd = static_cast<HWND>(subclassedHostHwnd);
        DWORD_PTR refData = 0;
        if (::GetWindowSubclass(hwnd, &hostWindowSubclassProc, kHostPinSubclassId, &refData))
        {
            ::RemoveWindowSubclass(hwnd, &hostWindowSubclassProc, kHostPinSubclassId);
            delete reinterpret_cast<HostPinHookContext*>(refData);
        }
        subclassedHostHwnd = nullptr;
    }
#endif
}

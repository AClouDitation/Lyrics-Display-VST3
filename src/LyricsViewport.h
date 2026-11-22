#pragma once

#include "LyricsModel.h"
#include "Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>

class LyricsViewport : public juce::Component
{
public:
    LyricsViewport();

    void setPalette(const theme::Palette& newPalette);
    void setFontSize(float newFontSize);
    float getFontSize() const noexcept { return baseFontSize; }

    void setLyrics(const LyricsRecord& record, const std::vector<LyricLine>& parsedLines);
    void setPlaceholderMessage(const juce::String& title, const juce::String& subtitle);
    void updatePlaybackTime(double playbackTimeSec, double offsetSec, bool forceSnap = false);
    void tickAnimation();
    void snapToActive();

    std::function<void(double)> onLineClicked;
    std::function<void()> onRequestEdit;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

private:
    struct LayoutRow
    {
        float y = 0.0f;
        float height = 0.0f;
        juce::TextLayout layout;
        juce::TextLayout activeLayout;
        juce::String timeLabel;
        double timeSec = 0.0;
    };

    void rebuildLayout();
    void updateScrollTarget(bool immediate);
    int hitTestRow(juce::Point<float> pos) const;

    theme::Palette palette = theme::darkPalette();
    float baseFontSize = 21.0f;

    bool hasSynced = false;
    bool hasPlain = false;
    std::vector<LyricLine> lines;
    juce::String plainText;
    juce::String placeholderTitle = "No lyrics loaded";
    juce::String placeholderSubtitle = "Search LRCLIB above, import an .lrc file, or double-click here to enter lyrics.";

    std::vector<LayoutRow> rows;
    juce::TextLayout plainLayout;
    float contentHeight = 0.0f;

    int activeIndex = -1;
    int hoveredIndex = -1;

    float scrollY = 0.0f;
    float targetScrollY = 0.0f;
    double manualScrollHoldUntilMs = 0.0;
    bool autoScrollEnabled = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LyricsViewport)
};

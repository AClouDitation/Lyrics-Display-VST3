#include "LyricsViewport.h"

#include <algorithm>
#include <cmath>

LyricsViewport::LyricsViewport()
{
    setOpaque(false);
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void LyricsViewport::setPalette(const theme::Palette& newPalette)
{
    palette = newPalette;
    rebuildLayout();
    repaint();
}

void LyricsViewport::setFontSize(float newFontSize)
{
    baseFontSize = juce::jlimit(13.0f, 38.0f, newFontSize);
    rebuildLayout();
    updateScrollTarget(true);
    repaint();
}

void LyricsViewport::setLyrics(const LyricsRecord& record, const std::vector<LyricLine>& parsedLines)
{
    lines = parsedLines;
    hasSynced = !lines.empty();
    hasPlain = !hasSynced && record.hasPlain();
    plainText = record.plainLyrics;
    activeIndex = -1;
    hoveredIndex = -1;
    scrollY = 0.0f;
    targetScrollY = 0.0f;
    manualScrollHoldUntilMs = 0.0;

    if (!hasSynced && !hasPlain)
    {
        placeholderTitle = "No lyrics loaded";
        placeholderSubtitle = "Search LRCLIB above, import an .lrc file, or double-click here to enter lyrics.";
    }

    rebuildLayout();
    repaint();
}

void LyricsViewport::setPlaceholderMessage(const juce::String& title, const juce::String& subtitle)
{
    lines.clear();
    hasSynced = false;
    hasPlain = false;
    plainText.clear();
    activeIndex = -1;
    hoveredIndex = -1;
    scrollY = 0.0f;
    targetScrollY = 0.0f;
    placeholderTitle = title;
    placeholderSubtitle = subtitle;
    rebuildLayout();
    repaint();
}

void LyricsViewport::updatePlaybackTime(double playbackTimeSec, double offsetSec, bool forceSnap)
{
    if (!hasSynced)
        return;

    const int nextIdx = lyrics::findActiveLineIndex(lines, playbackTimeSec + offsetSec);
    if (nextIdx != activeIndex || forceSnap)
    {
        activeIndex = nextIdx;
        if (forceSnap)
            manualScrollHoldUntilMs = 0.0;
        updateScrollTarget(forceSnap);
        repaint();
    }
}

void LyricsViewport::snapToActive()
{
    manualScrollHoldUntilMs = 0.0;
    autoScrollEnabled = true;
    updateScrollTarget(false);
    repaint();
}

void LyricsViewport::tickAnimation()
{
    const float diff = targetScrollY - scrollY;
    if (std::abs(diff) > 0.35f)
    {
        scrollY += diff * 0.16f;
        repaint();
    }
    else if (std::abs(diff) > 0.001f)
    {
        scrollY = targetScrollY;
        repaint();
    }
}

void LyricsViewport::rebuildLayout()
{
    rows.clear();
    contentHeight = 0.0f;

    const float viewW = static_cast<float>(getWidth());
    if (viewW < 60.0f)
        return;

    constexpr float padX = 24.0f;
    constexpr float timeColW = 58.0f;
    const float textW = std::max(120.0f, viewW - padX * 2.0f - timeColW - 24.0f);

    if (hasSynced)
    {
        const float topSpacer = std::max(48.0f, static_cast<float>(getHeight()) * 0.28f);
        float y = topSpacer;

        const auto normalFont = theme::makeUiFont(baseFontSize, false);
        const auto activeFont = theme::makeUiFont(baseFontSize * 1.16f, true);

        rows.reserve(lines.size());
        for (const auto& line : lines)
        {
            LayoutRow row;
            row.timeSec = line.timeSec;
            row.timeLabel = lyrics::formatTime(line.timeSec);

            juce::AttributedString asNormal;
            asNormal.setJustification(juce::Justification::centredLeft);
            asNormal.setWordWrap(juce::AttributedString::byWord);
            asNormal.setLineSpacing(baseFontSize * 0.28f);
            asNormal.append(line.text, normalFont, palette.text);
            row.layout.createLayout(asNormal, textW);

            juce::AttributedString asActive;
            asActive.setJustification(juce::Justification::centredLeft);
            asActive.setWordWrap(juce::AttributedString::byWord);
            asActive.setLineSpacing(baseFontSize * 0.28f);
            asActive.append(line.text, activeFont, palette.text);
            row.activeLayout.createLayout(asActive, textW);

            const float h = std::max(row.layout.getHeight(), row.activeLayout.getHeight()) + 22.0f;
            row.y = y;
            row.height = h;
            y += h + 6.0f;
            rows.push_back(std::move(row));
        }

        contentHeight = y + std::max(80.0f, static_cast<float>(getHeight()) * 0.45f);
    }
    else if (hasPlain)
    {
        juce::AttributedString as;
        as.setJustification(juce::Justification::centredLeft);
        as.setWordWrap(juce::AttributedString::byWord);
        as.setLineSpacing(baseFontSize * 0.36f);
        as.append(plainText, theme::makeUiFont(baseFontSize * 0.92f, false), palette.text);
        plainLayout.createLayout(as, std::max(120.0f, viewW - padX * 2.0f));
        contentHeight = plainLayout.getHeight() + 64.0f;
    }

    const float maxScroll = std::max(0.0f, contentHeight - static_cast<float>(getHeight()));
    scrollY = juce::jlimit(0.0f, maxScroll, scrollY);
    targetScrollY = juce::jlimit(0.0f, maxScroll, targetScrollY);
}

void LyricsViewport::updateScrollTarget(bool immediate)
{
    if (!hasSynced || rows.empty())
        return;

    const double nowMs = juce::Time::getMillisecondCounterHiRes();
    if (!immediate && (!autoScrollEnabled || nowMs < manualScrollHoldUntilMs))
        return;

    if (activeIndex < 0 || activeIndex >= static_cast<int>(rows.size()))
        return;

    const auto& row = rows[static_cast<size_t>(activeIndex)];
    const float viewH = static_cast<float>(getHeight());
    const float desired = row.y + row.height * 0.5f - viewH * 0.44f;
    const float maxScroll = std::max(0.0f, contentHeight - viewH);
    targetScrollY = juce::jlimit(0.0f, maxScroll, desired);

    if (immediate)
        scrollY = targetScrollY;
}

int LyricsViewport::hitTestRow(juce::Point<float> pos) const
{
    if (!hasSynced)
        return -1;

    const float contentY = pos.y + scrollY;
    for (size_t i = 0; i < rows.size(); ++i)
    {
        if (contentY >= rows[i].y && contentY <= rows[i].y + rows[i].height)
            return static_cast<int>(i);
    }
    return -1;
}

void LyricsViewport::resized()
{
    rebuildLayout();
    updateScrollTarget(true);
}

void LyricsViewport::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // Background card
    g.setColour(palette.surface);
    g.fillRoundedRectangle(bounds, 10.0f);
    g.setColour(palette.border);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 10.0f, 1.0f);

    juce::Graphics::ScopedSaveState clipState(g);
    g.reduceClipRegion(getLocalBounds().reduced(2));

    if (!hasSynced && !hasPlain)
    {
        auto center = bounds.reduced(36.0f);
        g.setFont(theme::makeUiFont(18.0f, true));
        g.setColour(palette.textMuted);
        g.drawFittedText(placeholderTitle,
                         center.withHeight(30.0f).withCentre(center.getCentre().translated(0.0f, -20.0f)).toNearestInt(),
                         juce::Justification::centred, 1);

        g.setFont(theme::makeUiFont(13.5f, false));
        g.setColour(palette.textDim);
        g.drawFittedText(placeholderSubtitle,
                         center.withHeight(54.0f).withCentre(center.getCentre().translated(0.0f, 22.0f)).toNearestInt(),
                         juce::Justification::centred, 3);
        return;
    }

    const float viewH = bounds.getHeight();
    constexpr float padX = 24.0f;
    constexpr float timeColW = 58.0f;

    if (hasSynced)
    {
        for (size_t i = 0; i < rows.size(); ++i)
        {
            const auto& row = rows[i];
            const float drawY = row.y - scrollY;
            if (drawY + row.height < -20.0f || drawY > viewH + 20.0f)
                continue;

            const bool isActive = static_cast<int>(i) == activeIndex;
            const bool isPast = activeIndex >= 0 && static_cast<int>(i) < activeIndex;
            const bool isHover = static_cast<int>(i) == hoveredIndex;

            const juce::Rectangle<float> rowRect(12.0f, drawY, bounds.getWidth() - 24.0f, row.height);

            if (isActive)
            {
                g.setColour(palette.accentSoft);
                g.fillRoundedRectangle(rowRect, 8.0f);

                g.setColour(palette.accent);
                g.fillRoundedRectangle(juce::Rectangle<float>(rowRect.getX() + 4.0f, rowRect.getY() + 8.0f, 3.5f, rowRect.getHeight() - 16.0f), 2.0f);
            }
            else if (isHover)
            {
                g.setColour(palette.surfaceHover.withAlpha(0.55f));
                g.fillRoundedRectangle(rowRect, 8.0f);
            }

            // Timestamp pill on left
            g.setFont(theme::makeMonoFont(12.0f, isActive));
            g.setColour(isActive ? palette.accent : (isPast ? palette.textDim.withAlpha(0.45f) : palette.textDim.withAlpha(0.8f)));
            g.drawText(row.timeLabel,
                       juce::Rectangle<float>(padX, drawY + 10.0f, timeColW - 8.0f, 22.0f),
                       juce::Justification::centredLeft, false);

            // Lyric text
            const float textX = padX + timeColW;
            const float textW = std::max(100.0f, bounds.getWidth() - textX - padX);
            const auto& layoutToDraw = isActive ? row.activeLayout : row.layout;
            const float textY = drawY + (row.height - layoutToDraw.getHeight()) * 0.5f;

            const float alpha = isActive ? 1.0f : (isPast ? 0.38f : 0.78f);
            g.setOpacity(alpha);
            layoutToDraw.draw(g, juce::Rectangle<float>(textX, textY, textW, layoutToDraw.getHeight()));
            g.setOpacity(1.0f);
        }
    }
    else if (hasPlain)
    {
        const float textW = std::max(120.0f, bounds.getWidth() - padX * 2.0f);
        plainLayout.draw(g, juce::Rectangle<float>(padX, 24.0f - scrollY, textW, plainLayout.getHeight()));
    }

    // Subtle top/bottom fade masks for karaoke polish
    if (hasSynced && contentHeight > viewH)
    {
        juce::ColourGradient topFade(palette.surface, 0.0f, 0.0f,
                                     palette.surface.withAlpha(0.0f), 0.0f, 36.0f, false);
        g.setGradientFill(topFade);
        g.fillRect(juce::Rectangle<float>(2.0f, 2.0f, bounds.getWidth() - 4.0f, 36.0f));

        juce::ColourGradient botFade(palette.surface.withAlpha(0.0f), 0.0f, viewH - 36.0f,
                                     palette.surface, 0.0f, viewH, false);
        g.setGradientFill(botFade);
        g.fillRect(juce::Rectangle<float>(2.0f, viewH - 36.0f, bounds.getWidth() - 4.0f, 36.0f));
    }

    // Minimal scrollbar thumb
    if (contentHeight > viewH + 2.0f)
    {
        const float ratio = viewH / contentHeight;
        const float thumbH = std::max(28.0f, viewH * ratio);
        const float maxScroll = contentHeight - viewH;
        const float thumbY = 8.0f + (scrollY / maxScroll) * (viewH - thumbH - 16.0f);
        g.setColour(palette.textDim.withAlpha(0.35f));
        g.fillRoundedRectangle(juce::Rectangle<float>(bounds.getWidth() - 8.0f, thumbY, 4.0f, thumbH), 2.0f);
    }
}

void LyricsViewport::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    const float viewH = static_cast<float>(getHeight());
    const float maxScroll = std::max(0.0f, contentHeight - viewH);
    if (maxScroll <= 0.0f)
        return;

    const float delta = -wheel.deltaY * 140.0f;
    targetScrollY = juce::jlimit(0.0f, maxScroll, targetScrollY + delta);
    scrollY = juce::jlimit(0.0f, maxScroll, scrollY + delta * 0.45f);
    manualScrollHoldUntilMs = juce::Time::getMillisecondCounterHiRes() + 3500.0;
    repaint();
}

void LyricsViewport::mouseMove(const juce::MouseEvent& e)
{
    const int idx = hitTestRow(e.position);
    if (idx != hoveredIndex)
    {
        hoveredIndex = idx;
        setMouseCursor(idx >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void LyricsViewport::mouseExit(const juce::MouseEvent&)
{
    if (hoveredIndex != -1)
    {
        hoveredIndex = -1;
        setMouseCursor(juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void LyricsViewport::mouseDown(const juce::MouseEvent& e)
{
    if (!e.mods.isLeftButtonDown())
        return;

    const int idx = hitTestRow(e.position);
    if (idx >= 0 && onLineClicked)
        onLineClicked(rows[static_cast<size_t>(idx)].timeSec);
}

void LyricsViewport::mouseDoubleClick(const juce::MouseEvent&)
{
    if (onRequestEdit)
        onRequestEdit();
}

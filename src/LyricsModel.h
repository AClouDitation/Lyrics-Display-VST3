#pragma once

#include <juce_core/juce_core.h>
#include <vector>

struct LyricLine
{
    double timeSec = 0.0;
    juce::String text;
};

struct LyricsRecord
{
    juce::String id;
    juce::String trackName;
    juce::String artistName;
    juce::String albumName;
    double duration = 0.0;
    bool hasDuration = false;
    juce::String syncedLyrics;
    juce::String plainLyrics;
    juce::String source;
    double offsetSec = 0.0;

    bool hasSynced() const noexcept { return syncedLyrics.trim().isNotEmpty(); }
    bool hasPlain() const noexcept { return plainLyrics.trim().isNotEmpty(); }
    bool isEmpty() const noexcept { return !hasSynced() && !hasPlain(); }

    juce::String displayLabel() const;
    juce::var toVar() const;
    static LyricsRecord fromVar(const juce::var& v);
};

struct ParsedLrc
{
    std::vector<LyricLine> lines;
    double embeddedOffsetSec = 0.0;
    juce::String title;
    juce::String artist;
    juce::String album;
};

namespace lyrics
{
    // Parse LRC formatted text into chronologically sorted lines
    ParsedLrc parseLrc(const juce::String& lrcText);

    // Serialize lyric lines back to LRC text
    juce::String serializeLrc(const std::vector<LyricLine>& lines);

    // Format seconds as [mm:ss.xx]
    juce::String formatLrcTimestamp(double sec);

    // Format seconds as m:ss
    juce::String formatTime(double sec);

    // Format seconds as m:ss.d
    juce::String formatTimePrecise(double sec);

    // Clean raw filenames/track titles for better lyrics search matching
    juce::String cleanTrackQuery(const juce::String& rawTrackName);

    // Find active lyric line index for (playbackSec + offsetSec) with 0.05s lookahead
    int findActiveLineIndex(const std::vector<LyricLine>& lines, double effectiveTimeSec);

    // Compare semantic version strings like "v1.2.3" and "1.0.0" (-1, 0, +1)
    int compareSemver(const juce::String& a, const juce::String& b);
    bool isNewerVersion(const juce::String& candidate, const juce::String& current);
} // namespace lyrics

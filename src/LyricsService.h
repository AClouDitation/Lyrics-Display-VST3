#pragma once

#include "LyricsModel.h"
#include <juce_core/juce_core.h>
#include <vector>

struct LrclibSearchResult
{
    bool ok = false;
    juce::String errorMessage;
    std::vector<LyricsRecord> candidates;
};

struct AppSettings
{
    bool darkTheme = true;
    float fontSize = 21.0f;
};

namespace lyrics_service
{
    // Synchronous HTTPS lookup against https://lrclib.net/api/search (call from background thread)
    LrclibSearchResult searchLrclib(const juce::String& query, double expectedDurationSec = 0.0);

    // Load lyrics from an arbitrary .json, .lrc, or .txt file
    bool loadLyricsFromFile(const juce::File& file, LyricsRecord& outRecord);

    // Export lyrics to .lrc, .txt, or .json
    bool exportLyricsToFile(const juce::File& targetFile, const LyricsRecord& record);

    // Global user preferences (%APPDATA%/LyricsDisplay/settings.json)
    AppSettings loadAppSettings();
    void saveAppSettings(const AppSettings& settings);
} // namespace lyrics_service

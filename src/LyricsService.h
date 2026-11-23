#pragma once

#include "LyricsModel.h"
#include <juce_core/juce_core.h>
#include <functional>
#include <vector>

struct LrclibSearchResult
{
    bool ok = false;
    juce::String errorMessage;
    std::vector<LyricsRecord> candidates;
};

struct GithubReleaseInfo
{
    bool ok = false;
    bool noReleasesYet = false;
    juce::String errorMessage;
    juce::String tagName;
    juce::String releaseName;
    juce::String body;
    juce::String htmlUrl;
    juce::String publishedAt;
    juce::String assetName;
    juce::String downloadUrl;
    juce::int64 assetSizeBytes = 0;
};

struct ReleaseUpdateResult
{
    bool ok = false;
    bool usedElevation = false;
    bool isStandalone = false;
    juce::String errorMessage;
    juce::File installedBinary;
};

struct AppSettings
{
    bool darkTheme = true;
    float fontSize = 21.0f;
    bool stayOpen = true;
};

namespace lyrics_service
{
    constexpr const char* kGithubRepoUrl = "https://github.com/AClouDitation/Lyrics-Display-VST3";
    constexpr const char* kGithubLatestReleaseApiUrl = "https://api.github.com/repos/AClouDitation/Lyrics-Display-VST3/releases/latest";

    // Synchronous HTTPS lookup against https://lrclib.net/api/search (call from background thread)
    LrclibSearchResult searchLrclib(const juce::String& query, double expectedDurationSec = 0.0);

    // Fetch latest release metadata from GitHub Releases API (call from background thread)
    GithubReleaseInfo fetchLatestGithubRelease();

    // Download latest GitHub release and replace the installed VST3/Standalone binary in place
    ReleaseUpdateResult updatePluginInPlace(
        const GithubReleaseInfo& release,
        std::function<void(double progress01, const juce::String& status)> onProgress = nullptr);

    // Remove any leftover .old backup binary left behind by a prior in-place update
    void cleanupOldUpdateBackups();

    // Return the path of the currently loaded plugin DLL (.vst3) or Standalone .exe
    juce::File getCurrentModuleFile();

    // Load lyrics from an arbitrary .json, .lrc, or .txt file
    bool loadLyricsFromFile(const juce::File& file, LyricsRecord& outRecord);

    // Export lyrics to .lrc, .txt, or .json
    bool exportLyricsToFile(const juce::File& targetFile, const LyricsRecord& record);

    // Global user preferences (%APPDATA%/LyricsDisplay/settings.json)
    AppSettings loadAppSettings();
    void saveAppSettings(const AppSettings& settings);
} // namespace lyrics_service

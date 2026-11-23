#include "LyricsModel.h"
#include <iostream>
#include <cmath>

namespace
{
int failures = 0;

void expect(bool cond, const char* msg)
{
    if (!cond)
    {
        std::cerr << "[FAIL] " << msg << "\n";
        ++failures;
    }
}

void testParseLrc()
{
    const juce::String sample = juce::String::fromUTF8(
        "[ti:永不失联的爱]\n"
        "[ar:周兴哲]\n"
        "[offset:500]\n"
        "[00:12.34] 亲爱的你躲在哪里发呆\n"
        "[00:25.100][01:10.50] 我给你最后的疼爱是手放开\n"
        "[00:05.00] Intro line\n");

    auto parsed = lyrics::parseLrc(sample);
    expect(parsed.title == juce::String::fromUTF8("永不失联的爱"), "LRC title parsed");
    expect(parsed.artist == juce::String::fromUTF8("周兴哲"), "LRC artist parsed");
    expect(std::abs(parsed.embeddedOffsetSec - 0.5) < 1e-6, "LRC offset parsed in seconds");
    expect(parsed.lines.size() == 4, "Multi-timestamp line expands into 4 sorted lines");
    expect(std::abs(parsed.lines[0].timeSec - 5.0) < 1e-6, "Sorted first line is 5.00s");
    expect(std::abs(parsed.lines[1].timeSec - 12.34) < 1e-6, "Second line is 12.34s");
    expect(std::abs(parsed.lines[2].timeSec - 25.1) < 1e-6, "Millisecond tag parsed");
    expect(std::abs(parsed.lines[3].timeSec - 70.5) < 1e-6, "Second timestamp of multi-tag parsed");
}

void testActiveLineLookup()
{
    const auto parsed = lyrics::parseLrc(
        "[00:10.00] Line 1\n"
        "[00:20.00] Line 2\n"
        "[00:30.00] Line 3\n");

    expect(lyrics::findActiveLineIndex(parsed.lines, 0.0) == -1, "Before first line returns -1");
    expect(lyrics::findActiveLineIndex(parsed.lines, 9.96) == 0, "0.05s lookahead highlights line 0");
    expect(lyrics::findActiveLineIndex(parsed.lines, 15.0) == 0, "Mid line 0");
    expect(lyrics::findActiveLineIndex(parsed.lines, 20.0) == 1, "Exact line 1");
    expect(lyrics::findActiveLineIndex(parsed.lines, 45.0) == 2, "After last line stays on last line");
}

void testCleanTrackQuery()
{
    expect(lyrics::cleanTrackQuery("YTDown.com_YouTube_Media_abc1234567_Always_Online_(Official_MV)_320_320k") == "Always Online (Official MV)",
           "Strips YTDown prefix and bitrate suffix");
    expect(lyrics::cleanTrackQuery("The-Biggest-Dreamer [Official Video]") == "The Biggest Dreamer",
           "Strips bracketed Official Video tag and normalizes hyphens");
    expect(lyrics::cleanTrackQuery(juce::String::fromUTF8("雨下一整晚 (Official Music Video)")) == juce::String::fromUTF8("雨下一整晚"),
           "Preserves Chinese characters while stripping MV tag");
}

void testJsonRoundTrip()
{
    LyricsRecord orig;
    orig.id = "12345";
    orig.trackName = juce::String::fromUTF8("红豆");
    orig.artistName = juce::String::fromUTF8("王菲");
    orig.albumName = juce::String::fromUTF8("唱游");
    orig.duration = 259.5;
    orig.hasDuration = true;
    orig.syncedLyrics = juce::String::fromUTF8("[00:15.20] 还没好好的感受");
    orig.plainLyrics = juce::String::fromUTF8("还没好好的感受");
    orig.source = "LRCLIB";
    orig.offsetSec = -0.5;

    const auto json = juce::JSON::toString(orig.toVar(), false);
    juce::var parsed;
    expect(juce::JSON::parse(json, parsed).wasOk(), "JSON serializes and parses cleanly");

    const auto restored = LyricsRecord::fromVar(parsed);
    expect(restored.trackName == orig.trackName, "Round-trip UTF-8 trackName");
    expect(restored.artistName == orig.artistName, "Round-trip UTF-8 artistName");
    expect(std::abs(restored.duration - orig.duration) < 1e-6, "Round-trip duration");
    expect(std::abs(restored.offsetSec - orig.offsetSec) < 1e-6, "Round-trip offset");
    expect(restored.syncedLyrics == orig.syncedLyrics, "Round-trip syncedLyrics");
}

void testSemverComparison()
{
    expect(lyrics::compareSemver("v1.0.0", "1.0.0") == 0, "v prefix ignored in semver");
    expect(lyrics::isNewerVersion("v1.2.0", "1.0.0"), "1.2.0 > 1.0.0");
    expect(lyrics::isNewerVersion("v2.0.0", "1.9.9"), "2.0.0 > 1.9.9");
    expect(!lyrics::isNewerVersion("v1.0.0", "1.0.0"), "same version is not newer");
    expect(!lyrics::isNewerVersion("v0.9.5", "1.0.0"), "older version is not newer");
    expect(lyrics::isNewerVersion("v1.0.1-beta", "1.0.0"), "prerelease tag stripped cleanly");
}
} // namespace

int main()
{
    testParseLrc();
    testActiveLineLookup();
    testCleanTrackQuery();
    testJsonRoundTrip();
    testSemverComparison();

    if (failures == 0)
    {
        std::cout << "All LyricsModelTests passed!\n";
        return 0;
    }

    std::cerr << failures << " test(s) failed.\n";
    return 1;
}

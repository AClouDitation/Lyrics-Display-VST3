#include "LyricsService.h"

#include <algorithm>
#include <cmath>
#include <tuple>

namespace lyrics_service
{

namespace
{
    juce::String urlEncodeUtf8(const juce::String& text)
    {
        auto utf8 = text.toRawUTF8();
        juce::String out;
        for (size_t i = 0; utf8[i] != '\0'; ++i)
        {
            const auto c = static_cast<unsigned char>(utf8[i]);
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')
                || c == '-' || c == '_' || c == '.' || c == '~')
            {
                out += static_cast<char>(c);
            }
            else if (c == ' ')
            {
                out += "%20";
            }
            else
            {
                out += juce::String::formatted("%%%02X", static_cast<unsigned int>(c));
            }
        }
        return out;
    }

    juce::File getSettingsFile()
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("LyricsDisplay")
            .getChildFile("settings.json");
    }
} // namespace

LrclibSearchResult searchLrclib(const juce::String& query, double expectedDurationSec)
{
    LrclibSearchResult result;
    const auto trimmed = query.trim();
    if (trimmed.isEmpty())
    {
        result.errorMessage = "Enter a song title or artist to search.";
        return result;
    }

    const juce::String fullUrl = "https://lrclib.net/api/search?q=" + urlEncodeUtf8(trimmed);
    juce::URL url(fullUrl);

    int statusCode = 0;
    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                       .withConnectionTimeoutMs(10000)
                       .withNumRedirectsToFollow(5)
                       .withStatusCode(&statusCode)
                       .withExtraHeaders("User-Agent: LyricsDisplay/1.0\r\n"
                                         "Accept: application/json\r\n");

    auto stream = url.createInputStream(options);
    if (!stream)
    {
        result.errorMessage = "Could not connect to LRCLIB (check internet access).";
        return result;
    }

    const juce::String body = stream->readEntireStreamAsString();
    if (statusCode != 0 && (statusCode < 200 || statusCode >= 300))
    {
        result.errorMessage = "LRCLIB returned HTTP " + juce::String(statusCode) + ".";
        return result;
    }

    juce::var parsed;
    auto parseRes = juce::JSON::parse(body, parsed);
    if (parseRes.failed() || !parsed.isArray())
    {
        result.errorMessage = "Unexpected response from LRCLIB.";
        return result;
    }

    struct ScoredCandidate
    {
        LyricsRecord rec;
        int syncedRank = 1;
        int durBucket = 2;
        double durDiff = 999.0;
    };

    std::vector<ScoredCandidate> scored;
    if (auto* arr = parsed.getArray())
    {
        scored.reserve(static_cast<size_t>(arr->size()));
        for (const auto& item : *arr)
        {
            LyricsRecord rec = LyricsRecord::fromVar(item);
            if (rec.isEmpty())
                continue;
            rec.source = "LRCLIB";
            rec.offsetSec = 0.0;

            double durDiff = 999.0;
            if (expectedDurationSec > 0.0 && rec.hasDuration)
                durDiff = std::abs(rec.duration - expectedDurationSec);

            int durBucket = 2;
            if (durDiff <= 5.0)
                durBucket = 0;
            else if (durDiff <= 15.0)
                durBucket = 1;

            scored.push_back({ std::move(rec), rec.hasSynced() ? 0 : 1, durBucket, durDiff });
        }
    }

    std::stable_sort(scored.begin(), scored.end(), [](const ScoredCandidate& a, const ScoredCandidate& b) {
        return std::tie(a.syncedRank, a.durBucket, a.durDiff) < std::tie(b.syncedRank, b.durBucket, b.durDiff);
    });

    result.candidates.reserve(scored.size());
    for (auto& s : scored)
        result.candidates.push_back(std::move(s.rec));

    result.ok = true;
    return result;
}

bool loadLyricsFromFile(const juce::File& file, LyricsRecord& outRecord)
{
    if (!file.existsAsFile())
        return false;

    const auto text = file.loadFileAsString();
    if (text.trim().isEmpty())
        return false;

    const auto ext = file.getFileExtension().toLowerCase();
    if (ext == ".json")
    {
        juce::var parsed;
        if (juce::JSON::parse(text, parsed).wasOk() && parsed.isObject())
        {
            outRecord = LyricsRecord::fromVar(parsed);
            if (outRecord.source.isEmpty())
                outRecord.source = "Local File";
            return !outRecord.isEmpty();
        }
        return false;
    }

    auto parsedLrc = lyrics::parseLrc(text);
    outRecord = {};
    outRecord.id = "file:" + file.getFileName();
    outRecord.trackName = parsedLrc.title.isNotEmpty() ? parsedLrc.title : file.getFileNameWithoutExtension();
    outRecord.artistName = parsedLrc.artist;
    outRecord.albumName = parsedLrc.album;
    outRecord.offsetSec = parsedLrc.embeddedOffsetSec;

    if (!parsedLrc.lines.empty())
    {
        outRecord.syncedLyrics = text.trim();
        juce::StringArray plain;
        for (const auto& l : parsedLrc.lines)
            plain.add(l.text);
        outRecord.plainLyrics = plain.joinIntoString("\n");
        outRecord.source = "Local .lrc";
    }
    else
    {
        outRecord.plainLyrics = text.trim();
        outRecord.source = "Local .txt";
    }
    return !outRecord.isEmpty();
}

bool exportLyricsToFile(const juce::File& targetFile, const LyricsRecord& record)
{
    const auto ext = targetFile.getFileExtension().toLowerCase();
    if (ext == ".json")
    {
        const auto jsonText = juce::JSON::toString(record.toVar(), false);
        return targetFile.replaceWithText(jsonText, false, false, "\n");
    }

    const juce::String content = record.hasSynced() ? record.syncedLyrics : record.plainLyrics;
    return targetFile.replaceWithText(content, false, false, "\n");
}

AppSettings loadAppSettings()
{
    AppSettings settings;
    auto file = getSettingsFile();
    if (file.existsAsFile())
    {
        juce::var parsed;
        if (juce::JSON::parse(file.loadFileAsString(), parsed).wasOk())
        {
            if (auto* obj = parsed.getDynamicObject())
            {
                auto dark = obj->getProperty("darkTheme");
                if (dark.isBool())
                    settings.darkTheme = static_cast<bool>(dark);

                auto fs = obj->getProperty("fontSize");
                if (fs.isDouble() || fs.isInt())
                    settings.fontSize = juce::jlimit(13.0f, 38.0f, static_cast<float>(static_cast<double>(fs)));
            }
        }
    }

    return settings;
}

void saveAppSettings(const AppSettings& settings)
{
    auto file = getSettingsFile();
    file.getParentDirectory().createDirectory();

    auto* obj = new juce::DynamicObject();
    obj->setProperty("darkTheme", settings.darkTheme);
    obj->setProperty("fontSize", static_cast<double>(settings.fontSize));

    file.replaceWithText(juce::JSON::toString(juce::var(obj), false), false, false, "\n");
}

} // namespace lyrics_service

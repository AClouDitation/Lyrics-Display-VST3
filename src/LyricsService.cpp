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

GithubReleaseInfo fetchLatestGithubRelease()
{
    GithubReleaseInfo info;
    juce::URL url(kGithubLatestReleaseApiUrl);

    int statusCode = 0;
    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                       .withConnectionTimeoutMs(12000)
                       .withNumRedirectsToFollow(5)
                       .withStatusCode(&statusCode)
                       .withExtraHeaders("User-Agent: LyricsDisplay/1.0\r\n"
                                         "Accept: application/vnd.github+json\r\n");

    auto stream = url.createInputStream(options);
    if (!stream)
    {
        if (statusCode == 404)
        {
            info.noReleasesYet = true;
            info.errorMessage = "No releases have been published on GitHub yet.";
        }
        else
        {
            info.errorMessage = "Could not reach GitHub (check internet connection).";
        }
        return info;
    }

    const juce::String body = stream->readEntireStreamAsString();
    if (statusCode == 404)
    {
        info.noReleasesYet = true;
        info.errorMessage = "No releases have been published on GitHub yet.";
        return info;
    }
    if (statusCode != 0 && (statusCode < 200 || statusCode >= 300))
    {
        info.errorMessage = "GitHub returned HTTP " + juce::String(statusCode) + ".";
        return info;
    }

    juce::var parsed;
    if (juce::JSON::parse(body, parsed).failed() || !parsed.isObject())
    {
        info.errorMessage = "Unexpected response from GitHub Releases API.";
        return info;
    }

    auto* obj = parsed.getDynamicObject();
    auto strProp = [obj](const char* key) -> juce::String {
        auto v = obj->getProperty(key);
        return (v.isVoid() || v.isUndefined()) ? juce::String() : v.toString().trim();
    };

    info.tagName = strProp("tag_name");
    info.releaseName = strProp("name");
    if (info.releaseName.isEmpty())
        info.releaseName = info.tagName;
    info.body = strProp("body");
    info.htmlUrl = strProp("html_url");
    if (info.htmlUrl.isEmpty())
        info.htmlUrl = juce::String(kGithubRepoUrl) + "/releases";
    info.publishedAt = strProp("published_at");

    // Pick best release asset for Windows (prefer .zip / .vst3 / .exe), fallback to source zipball
    int bestScore = -1;
    if (auto* assets = obj->getProperty("assets").getArray())
    {
        for (const auto& assetVar : *assets)
        {
            auto* aObj = assetVar.getDynamicObject();
            if (aObj == nullptr)
                continue;
            const auto name = aObj->getProperty("name").toString().trim();
            const auto dlUrl = aObj->getProperty("browser_download_url").toString().trim();
            if (name.isEmpty() || !dlUrl.startsWithIgnoreCase("https://"))
                continue;

            const auto lower = name.toLowerCase();
            int score = 1;
            if (lower.contains("win") || lower.contains("x64") || lower.contains("vst3"))
                score += 10;
            if (lower.endsWith(".zip"))
                score += 6;
            else if (lower.endsWith(".vst3") || lower.endsWith(".exe") || lower.endsWith(".msi"))
                score += 5;

            if (score > bestScore)
            {
                bestScore = score;
                info.assetName = name;
                info.downloadUrl = dlUrl;
                info.assetSizeBytes = static_cast<juce::int64>(aObj->getProperty("size"));
            }
        }
    }

    if (info.downloadUrl.isEmpty())
    {
        const auto zipball = strProp("zipball_url");
        if (zipball.startsWithIgnoreCase("https://"))
        {
            info.downloadUrl = zipball;
            info.assetName = "Lyrics-Display-" + juce::File::createLegalFileName(info.tagName.isNotEmpty() ? info.tagName : "latest") + ".zip";
        }
    }

    info.ok = info.tagName.isNotEmpty();
    if (!info.ok && info.errorMessage.isEmpty())
        info.errorMessage = "Latest GitHub release metadata was incomplete.";
    return info;
}

ReleaseDownloadResult downloadLatestGithubRelease(
    const GithubReleaseInfo& release,
    std::function<void(double, const juce::String&)> onProgress)
{
    ReleaseDownloadResult res;
    if (!release.ok || !release.downloadUrl.startsWithIgnoreCase("https://"))
    {
        res.errorMessage = "No downloadable HTTPS release asset available.";
        return res;
    }

    auto baseDownloads = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("Downloads");
    if (!baseDownloads.isDirectory())
        baseDownloads = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);

    const juce::String folderName = "Lyrics-Display-"
        + juce::File::createLegalFileName(release.tagName.isNotEmpty() ? release.tagName : "latest");
    juce::File targetDir = baseDownloads.getChildFile(folderName);
    if (!targetDir.createDirectory().wasOk())
    {
        res.errorMessage = "Could not create download folder: " + targetDir.getFullPathName();
        return res;
    }

    const juce::String safeAssetName = juce::File::createLegalFileName(
        release.assetName.isNotEmpty() ? release.assetName : "release.zip");
    juce::File targetFile = targetDir.getChildFile(safeAssetName);

    if (onProgress)
        onProgress(0.05, "Downloading " + safeAssetName + "...");

    int statusCode = 0;
    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                       .withConnectionTimeoutMs(20000)
                       .withNumRedirectsToFollow(8)
                       .withStatusCode(&statusCode)
                       .withExtraHeaders("User-Agent: LyricsDisplay/1.0\r\n");

    auto stream = juce::URL(release.downloadUrl).createInputStream(options);
    if (!stream)
    {
        res.errorMessage = "Failed to download release asset from GitHub.";
        return res;
    }
    if (statusCode != 0 && (statusCode < 200 || statusCode >= 300))
    {
        res.errorMessage = "Download failed with HTTP " + juce::String(statusCode) + ".";
        return res;
    }

    targetFile.deleteFile();
    {
        juce::FileOutputStream out(targetFile);
        if (!out.openedOk())
        {
            res.errorMessage = "Could not open file for writing: " + targetFile.getFullPathName();
            return res;
        }

        const juce::int64 expectedBytes = release.assetSizeBytes > 0
            ? release.assetSizeBytes
            : stream->getTotalLength();

        juce::HeapBlock<char> buf(32768);
        juce::int64 totalWritten = 0;
        while (!stream->isExhausted())
        {
            const int bytesRead = stream->read(buf.getData(), 32768);
            if (bytesRead <= 0)
                break;
            if (!out.write(buf.getData(), static_cast<size_t>(bytesRead)))
            {
                res.errorMessage = "Disk write error while saving release.";
                return res;
            }
            totalWritten += bytesRead;
            if (onProgress)
            {
                if (expectedBytes > 0)
                {
                    const double pct = juce::jlimit(0.05, 0.85, 0.05 + 0.80 * (static_cast<double>(totalWritten) / static_cast<double>(expectedBytes)));
                    onProgress(pct, juce::String::formatted("Downloading %.1f / %.1f MB...",
                                                            static_cast<double>(totalWritten) / 1048576.0,
                                                            static_cast<double>(expectedBytes) / 1048576.0));
                }
                else
                {
                    onProgress(0.5, juce::String::formatted("Downloading %.1f MB...",
                                                            static_cast<double>(totalWritten) / 1048576.0));
                }
            }
        }
        out.flush();
    }

    res.outputDir = targetDir;
    res.downloadedFile = targetFile;

    // Safely extract .zip archives (guarding against zip-slip path traversal)
    if (targetFile.getFileExtension().equalsIgnoreCase(".zip"))
    {
        if (onProgress)
            onProgress(0.90, "Extracting release archive...");

        juce::ZipFile zip(targetFile);
        const int numEntries = zip.getNumEntries();
        if (numEntries > 0)
        {
            juce::File extractRoot = targetDir.getChildFile("extracted");
            extractRoot.createDirectory();
            bool allSafe = true;
            for (int i = 0; i < numEntries; ++i)
            {
                const auto* entry = zip.getEntry(i);
                if (entry == nullptr)
                    continue;
                const auto entryPath = entry->filename.replaceCharacter('\\', '/');
                if (entryPath.startsWith("/") || entryPath.contains(".."))
                {
                    allSafe = false;
                    continue;
                }
                const auto dest = extractRoot.getChildFile(entryPath);
                if (!dest.isAChildOf(extractRoot))
                {
                    allSafe = false;
                    continue;
                }
                zip.uncompressEntry(i, extractRoot, true);
            }
            res.extracted = allSafe;
            if (allSafe)
                res.outputDir = extractRoot;
        }
    }

    if (onProgress)
        onProgress(1.0, "Release ready in " + res.outputDir.getFileName());

    res.ok = true;
    return res;
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

                auto stay = obj->getProperty("stayOpen");
                if (stay.isBool())
                    settings.stayOpen = static_cast<bool>(stay);
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
    obj->setProperty("stayOpen", settings.stayOpen);

    file.replaceWithText(juce::JSON::toString(juce::var(obj), false), false, false, "\n");
}

} // namespace lyrics_service

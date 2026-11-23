#include "LyricsService.h"

#include <algorithm>
#include <cmath>
#include <tuple>

#if JUCE_WINDOWS
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
 #include <shellapi.h>
#endif

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

juce::File getCurrentModuleFile()
{
#if JUCE_WINDOWS
    HMODULE hMod = nullptr;
    if (::GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&getCurrentModuleFile),
            &hMod) != FALSE && hMod != nullptr)
    {
        std::wstring buf(32768, L'\0');
        const DWORD len = ::GetModuleFileNameW(hMod, buf.data(), static_cast<DWORD>(buf.size()));
        if (len > 0 && len < buf.size())
            return juce::File(juce::String(buf.c_str(), static_cast<size_t>(len)));
    }
#endif
    return juce::File::getSpecialLocation(juce::File::currentExecutableFile);
}

void cleanupOldUpdateBackups()
{
    const auto modFile = getCurrentModuleFile();
    if (!modFile.existsAsFile())
        return;

    const auto backupFile = juce::File(modFile.getFullPathName() + ".old");
    if (backupFile.existsAsFile())
        backupFile.deleteFile();
}

namespace
{
#if JUCE_WINDOWS
    juce::String escapePsSingleQuoted(const juce::String& s)
    {
        return s.replace("'", "''");
    }

    bool findExtractedPayload(const juce::File& extractRoot, bool wantVst3,
                              juce::File& outBinary, juce::File& outModuleInfo)
    {
        auto allFiles = extractRoot.findChildFiles(juce::File::findFiles, true);
        for (const auto& f : allFiles)
        {
            const auto name = f.getFileName();
            if (wantVst3)
            {
                if (name.equalsIgnoreCase("Lyrics Display.vst3")
                    && f.getParentDirectory().getFileName().equalsIgnoreCase("x86_64-win"))
                {
                    outBinary = f;
                }
                else if (name.equalsIgnoreCase("moduleinfo.json"))
                {
                    outModuleInfo = f;
                }
            }
            else
            {
                if (name.equalsIgnoreCase("Lyrics Display.exe"))
                    outBinary = f;
            }
        }
        return outBinary.existsAsFile();
    }

    bool applyElevatedReplace(const juce::File& stagingDir,
                              const juce::File& targetBinary,
                              const juce::File& newBinary,
                              const juce::File& targetModuleInfo,
                              const juce::File& newModuleInfo,
                              juce::String& outError)
    {
        const auto scriptFile = stagingDir.getChildFile("apply_update.ps1");
        const auto backupPath = targetBinary.getFullPathName() + ".old";

        juce::String ps;
        ps << "$ErrorActionPreference = 'Stop'\n";
        ps << "$target = '" << escapePsSingleQuoted(targetBinary.getFullPathName()) << "'\n";
        ps << "$backup = '" << escapePsSingleQuoted(backupPath) << "'\n";
        ps << "$newBin = '" << escapePsSingleQuoted(newBinary.getFullPathName()) << "'\n";
        ps << "if (Test-Path $backup) { Remove-Item -LiteralPath $backup -Force -ErrorAction SilentlyContinue }\n";
        ps << "Move-Item -LiteralPath $target -Destination $backup -Force\n";
        ps << "try {\n";
        ps << "    Copy-Item -LiteralPath $newBin -Destination $target -Force\n";
        ps << "} catch {\n";
        ps << "    Move-Item -LiteralPath $backup -Destination $target -Force -ErrorAction SilentlyContinue\n";
        ps << "    throw\n";
        ps << "}\n";
        if (targetModuleInfo != juce::File() && newModuleInfo.existsAsFile())
        {
            ps << "$targetInfo = '" << escapePsSingleQuoted(targetModuleInfo.getFullPathName()) << "'\n";
            ps << "$newInfo = '" << escapePsSingleQuoted(newModuleInfo.getFullPathName()) << "'\n";
            ps << "New-Item -ItemType Directory -Path (Split-Path -Parent $targetInfo) -Force | Out-Null\n";
            ps << "Copy-Item -LiteralPath $newInfo -Destination $targetInfo -Force\n";
        }
        ps << "Remove-Item -LiteralPath $backup -Force -ErrorAction SilentlyContinue\n";

        if (!scriptFile.replaceWithText(ps, false, false, "\n"))
        {
            outError = "Could not create elevated update helper script.";
            return false;
        }

        const juce::String params = "-NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File \""
                                    + scriptFile.getFullPathName() + "\"";
        const std::wstring wParams = params.toWideCharPointer();

        SHELLEXECUTEINFOW sei {};
        sei.cbSize = sizeof(sei);
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        sei.lpVerb = L"runas";
        sei.lpFile = L"powershell.exe";
        sei.lpParameters = wParams.c_str();
        sei.nShow = SW_HIDE;

        if (::ShellExecuteExW(&sei) == FALSE)
        {
            const DWORD err = ::GetLastError();
            outError = (err == ERROR_CANCELLED)
                ? "Administrator permission prompt was cancelled."
                : ("Could not start elevated installer (Win32 error " + juce::String(static_cast<int>(err)) + ").");
            return false;
        }

        DWORD exitCode = 1;
        if (sei.hProcess != nullptr)
        {
            ::WaitForSingleObject(sei.hProcess, 60000);
            ::GetExitCodeProcess(sei.hProcess, &exitCode);
            ::CloseHandle(sei.hProcess);
        }

        if (exitCode != 0)
        {
            outError = "Elevated update failed (exit code " + juce::String(static_cast<int>(exitCode)) + ").";
            return false;
        }
        return true;
    }
#endif
} // namespace

ReleaseUpdateResult updatePluginInPlace(
    const GithubReleaseInfo& release,
    std::function<void(double, const juce::String&)> onProgress)
{
    ReleaseUpdateResult res;
    if (!release.ok || !release.downloadUrl.startsWithIgnoreCase("https://"))
    {
        res.errorMessage = "No downloadable HTTPS release asset available.";
        return res;
    }

#if !JUCE_WINDOWS
    res.errorMessage = "In-place self-update is currently supported on Windows only.";
    return res;
#else
    const juce::File currentBin = getCurrentModuleFile();
    if (!currentBin.existsAsFile())
    {
        res.errorMessage = "Could not locate the currently installed plugin binary.";
        return res;
    }

    const bool isVst3 = currentBin.getFileExtension().equalsIgnoreCase(".vst3");
    const bool isStandaloneExe = currentBin.getFileExtension().equalsIgnoreCase(".exe");
    res.isStandalone = isStandaloneExe;
    res.installedBinary = currentBin;

    if (!isVst3 && !isStandaloneExe)
    {
        res.errorMessage = "Unsupported module type for in-place update: " + currentBin.getFileName();
        return res;
    }

    const juce::File stagingDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                      .getChildFile("LyricsDisplayUpdate_" + juce::String(juce::Time::currentTimeMillis()));
    if (!stagingDir.createDirectory().wasOk())
    {
        res.errorMessage = "Could not create temporary staging directory.";
        return res;
    }

    struct ScopedDirCleanup
    {
        juce::File dir;
        ~ScopedDirCleanup() { dir.deleteRecursively(); }
    } cleanup { stagingDir };

    const juce::String safeAssetName = juce::File::createLegalFileName(
        release.assetName.isNotEmpty() ? release.assetName : "release.zip");
    const juce::File archiveFile = stagingDir.getChildFile(safeAssetName);

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

    {
        juce::FileOutputStream out(archiveFile);
        if (!out.openedOk())
        {
            res.errorMessage = "Could not write temporary release archive.";
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
                res.errorMessage = "Disk write error while downloading update.";
                return res;
            }
            totalWritten += bytesRead;
            if (onProgress)
            {
                if (expectedBytes > 0)
                {
                    const double pct = juce::jlimit(0.05, 0.78, 0.05 + 0.73 * (static_cast<double>(totalWritten) / static_cast<double>(expectedBytes)));
                    onProgress(pct, juce::String::formatted("Downloading %.1f / %.1f MB...",
                                                            static_cast<double>(totalWritten) / 1048576.0,
                                                            static_cast<double>(expectedBytes) / 1048576.0));
                }
                else
                {
                    onProgress(0.45, juce::String::formatted("Downloading %.1f MB...",
                                                             static_cast<double>(totalWritten) / 1048576.0));
                }
            }
        }
        out.flush();
    }

    juce::File newBinary;
    juce::File newModuleInfo;

    if (archiveFile.getFileExtension().equalsIgnoreCase(".zip"))
    {
        if (onProgress)
            onProgress(0.82, "Extracting release archive...");

        juce::ZipFile zip(archiveFile);
        const int numEntries = zip.getNumEntries();
        if (numEntries <= 0)
        {
            res.errorMessage = "Downloaded archive is empty or corrupted.";
            return res;
        }

        juce::File extractRoot = stagingDir.getChildFile("extracted");
        extractRoot.createDirectory();
        for (int i = 0; i < numEntries; ++i)
        {
            const auto* entry = zip.getEntry(i);
            if (entry == nullptr)
                continue;
            const auto entryPath = entry->filename.replaceCharacter('\\', '/');
            if (entryPath.startsWith("/") || entryPath.contains(".."))
                continue;
            const auto dest = extractRoot.getChildFile(entryPath);
            if (!dest.isAChildOf(extractRoot))
                continue;
            zip.uncompressEntry(i, extractRoot, true);
        }

        if (!findExtractedPayload(extractRoot, isVst3, newBinary, newModuleInfo))
        {
            res.errorMessage = isVst3
                ? "Release archive does not contain a Windows VST3 binary (Lyrics Display.vst3)."
                : "Release archive does not contain Lyrics Display.exe.";
            return res;
        }
    }
    else
    {
        newBinary = archiveFile;
    }

    juce::File targetModuleInfo;
    if (isVst3)
    {
        const auto contentsDir = currentBin.getParentDirectory().getParentDirectory();
        if (contentsDir.getFileName().equalsIgnoreCase("Contents"))
            targetModuleInfo = contentsDir.getChildFile("Resources").getChildFile("moduleinfo.json");
    }

    if (onProgress)
        onProgress(0.92, "Replacing installed binary in place...");

    // Attempt direct NTFS rename-while-loaded + copy-new (no UAC needed for user-writable folders)
    const juce::File backupBin(currentBin.getFullPathName() + ".old");
    backupBin.deleteFile();

    const std::wstring wTarget = currentBin.getFullPathName().toWideCharPointer();
    const std::wstring wBackup = backupBin.getFullPathName().toWideCharPointer();
    const std::wstring wNew = newBinary.getFullPathName().toWideCharPointer();

    if (::MoveFileExW(wTarget.c_str(), wBackup.c_str(), MOVEFILE_REPLACE_EXISTING) != FALSE)
    {
        if (::CopyFileW(wNew.c_str(), wTarget.c_str(), FALSE) != FALSE)
        {
            if (targetModuleInfo != juce::File() && newModuleInfo.existsAsFile())
            {
                targetModuleInfo.getParentDirectory().createDirectory();
                newModuleInfo.copyFileTo(targetModuleInfo);
            }
            ::MoveFileExW(wBackup.c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
            if (onProgress)
                onProgress(1.0, "Updated in place!");
            res.ok = true;
            return res;
        }

        // Roll back if copy failed
        ::MoveFileExW(wBackup.c_str(), wTarget.c_str(), MOVEFILE_REPLACE_EXISTING);
        res.errorMessage = "Failed to copy updated binary into place.";
        return res;
    }

    const DWORD moveErr = ::GetLastError();
    if (moveErr == ERROR_ACCESS_DENIED)
    {
        if (onProgress)
            onProgress(0.94, "Requesting administrator permission to update Program Files...");

        juce::String elevErr;
        if (!applyElevatedReplace(stagingDir, currentBin, newBinary, targetModuleInfo, newModuleInfo, elevErr))
        {
            res.errorMessage = elevErr;
            return res;
        }
        res.usedElevation = true;
        if (onProgress)
            onProgress(1.0, "Updated in place!");
        res.ok = true;
        return res;
    }

    res.errorMessage = "Could not replace binary in place (Win32 error " + juce::String(static_cast<int>(moveErr)) + ").";
    return res;
#endif
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

#include "LyricsModel.h"

#include <algorithm>
#include <cmath>
#include <regex>

juce::String LyricsRecord::displayLabel() const
{
    if (trackName.isNotEmpty() && artistName.isNotEmpty())
        return trackName + juce::String::fromUTF8(" — ") + artistName;
    if (trackName.isNotEmpty())
        return trackName;
    return artistName;
}

juce::var LyricsRecord::toVar() const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("id", id);
    obj->setProperty("trackName", trackName);
    obj->setProperty("artistName", artistName);
    obj->setProperty("albumName", albumName);
    if (hasDuration)
        obj->setProperty("duration", duration);
    else
        obj->setProperty("duration", juce::var());
    obj->setProperty("syncedLyrics", syncedLyrics.isNotEmpty() ? juce::var(syncedLyrics) : juce::var());
    obj->setProperty("plainLyrics", plainLyrics.isNotEmpty() ? juce::var(plainLyrics) : juce::var());
    obj->setProperty("source", source.isNotEmpty() ? source : juce::String("LRCLIB"));
    obj->setProperty("offset", offsetSec);
    return juce::var(obj);
}

LyricsRecord LyricsRecord::fromVar(const juce::var& v)
{
    LyricsRecord rec;
    if (auto* obj = v.getDynamicObject())
    {
        auto strProp = [&](const char* key) -> juce::String {
            auto val = obj->getProperty(key);
            return val.isVoid() || val.isUndefined() ? juce::String() : val.toString();
        };

        rec.id = strProp("id");
        rec.trackName = strProp("trackName");
        rec.artistName = strProp("artistName");
        rec.albumName = strProp("albumName");
        rec.syncedLyrics = strProp("syncedLyrics").trim();
        rec.plainLyrics = strProp("plainLyrics").trim();
        rec.source = strProp("source");

        auto durVal = obj->getProperty("duration");
        if (durVal.isDouble() || durVal.isInt() || durVal.isInt64())
        {
            rec.duration = static_cast<double>(durVal);
            rec.hasDuration = rec.duration > 0.0;
        }

        auto offVal = obj->getProperty("offset");
        if (offVal.isDouble() || offVal.isInt() || offVal.isInt64())
            rec.offsetSec = static_cast<double>(offVal);
    }
    return rec;
}

namespace lyrics
{

ParsedLrc parseLrc(const juce::String& lrcText)
{
    ParsedLrc parsed;
    if (lrcText.trim().isEmpty())
        return parsed;

    juce::StringArray rawLines;
    rawLines.addLines(lrcText);

    static const std::regex timeTagRe(R"(\[(\d{1,3}):(\d{2})(?:[\.:](\d{1,3}))?\])");
    static const std::regex metaTagRe(R"(^\s*\[(ti|ar|al|offset):([^\]]*)\]\s*$)", std::regex::icase);

    for (const auto& rawLine : rawLines)
    {
        std::string utf8 = rawLine.toStdString();

        std::smatch metaMatch;
        if (std::regex_match(utf8, metaMatch, metaTagRe))
        {
            juce::String key = juce::String(metaMatch[1].str()).toLowerCase();
            juce::String val = juce::String::fromUTF8(metaMatch[2].str().c_str()).trim();
            if (key == "ti")
                parsed.title = val;
            else if (key == "ar")
                parsed.artist = val;
            else if (key == "al")
                parsed.album = val;
            else if (key == "offset")
                parsed.embeddedOffsetSec = val.getDoubleValue() / 1000.0;
            continue;
        }

        std::vector<double> times;
        auto begin = std::sregex_iterator(utf8.begin(), utf8.end(), timeTagRe);
        auto end = std::sregex_iterator();

        for (auto it = begin; it != end; ++it)
        {
            const std::smatch& m = *it;
            const int mins = std::stoi(m[1].str());
            const int secs = std::stoi(m[2].str());
            double frac = 0.0;
            if (m[3].matched)
            {
                const std::string fracStr = m[3].str();
                frac = std::stod(fracStr) / std::pow(10.0, static_cast<double>(fracStr.size()));
            }
            times.push_back(static_cast<double>(mins * 60 + secs) + frac);
        }

        if (times.empty())
            continue;

        std::string stripped = std::regex_replace(utf8, timeTagRe, "");
        juce::String text = juce::String::fromUTF8(stripped.c_str()).trim();
        if (text.isEmpty())
            continue;

        for (double t : times)
            parsed.lines.push_back({ t, text });
    }

    std::stable_sort(parsed.lines.begin(), parsed.lines.end(),
                     [](const LyricLine& a, const LyricLine& b) { return a.timeSec < b.timeSec; });

    return parsed;
}

juce::String serializeLrc(const std::vector<LyricLine>& lines)
{
    juce::StringArray out;
    for (const auto& line : lines)
    {
        auto t = line.text.trim();
        if (t.isNotEmpty())
            out.add(formatLrcTimestamp(line.timeSec) + " " + t);
    }
    return out.joinIntoString("\n");
}

juce::String formatLrcTimestamp(double sec)
{
    const double clamped = std::max(0.0, sec);
    const int mins = static_cast<int>(clamped / 60.0);
    const double rem = clamped - static_cast<double>(mins * 60);
    return juce::String::formatted("[%02d:%05.2f]", mins, rem);
}

juce::String formatTime(double sec)
{
    if (!std::isfinite(sec) || sec < 0.0)
        return "0:00";
    const int total = static_cast<int>(std::floor(sec));
    const int m = total / 60;
    const int s = total % 60;
    return juce::String::formatted("%d:%02d", m, s);
}

juce::String formatTimePrecise(double sec)
{
    if (!std::isfinite(sec) || sec < 0.0)
        return "0:00.0";
    const int m = static_cast<int>(sec / 60.0);
    const double rem = sec - static_cast<double>(m * 60);
    return juce::String::formatted("%d:%04.1f", m, rem);
}

juce::String cleanTrackQuery(const juce::String& rawTrackName)
{
    std::string s = rawTrackName.trim().toStdString();
    if (s.empty())
        return {};

    static const std::regex ytPrefixRe(R"(^YTDown\.com_YouTube_Media_[A-Za-z0-9_-]{8,15}_)", std::regex::icase);
    static const std::regex bitrateSuffixRe(R"(_\d{2,4}_\d{2,4}k$)", std::regex::icase);
    static const std::regex bracketTagRe(
        R"([\(\[\{]\s*(?:official\s*(?:music\s*)?(?:video|mv|audio|lyric(?:s)?)|lyric(?:s)?|hd|4k|hq|mv|audio)\s*[\)\]\}])",
        std::regex::icase);
    static const std::regex sepRe(R"([_-]+)");
    static const std::regex multiSpaceRe(R"(\s+)");

    s = std::regex_replace(s, ytPrefixRe, "");
    s = std::regex_replace(s, bitrateSuffixRe, "");
    s = std::regex_replace(s, bracketTagRe, " ");
    s = std::regex_replace(s, sepRe, " ");
    s = std::regex_replace(s, multiSpaceRe, " ");

    juce::String cleaned = juce::String::fromUTF8(s.c_str()).trim();
    return cleaned.isNotEmpty() ? cleaned : rawTrackName.trim();
}

int findActiveLineIndex(const std::vector<LyricLine>& lines, double effectiveTimeSec)
{
    if (lines.empty())
        return -1;

    const double probe = effectiveTimeSec + 0.05;
    auto it = std::upper_bound(lines.begin(), lines.end(), probe,
                               [](double val, const LyricLine& line) { return val < line.timeSec; });

    if (it == lines.begin())
        return -1;

    return static_cast<int>(std::distance(lines.begin(), it)) - 1;
}

int compareSemver(const juce::String& a, const juce::String& b)
{
    auto parseParts = [](const juce::String& raw) {
        juce::String s = raw.trim();
        if (s.startsWithIgnoreCase("v"))
            s = s.substring(1);
        const int dashIdx = s.indexOfChar('-');
        if (dashIdx >= 0)
            s = s.substring(0, dashIdx);
        const int plusIdx = s.indexOfChar('+');
        if (plusIdx >= 0)
            s = s.substring(0, plusIdx);

        std::vector<int> nums;
        juce::StringArray tokens;
        tokens.addTokens(s, ".", "");
        for (const auto& tok : tokens)
            nums.push_back(tok.getIntValue());
        while (nums.size() < 3)
            nums.push_back(0);
        return nums;
    };

    const auto pa = parseParts(a);
    const auto pb = parseParts(b);
    const size_t n = std::max(pa.size(), pb.size());
    for (size_t i = 0; i < n; ++i)
    {
        const int va = i < pa.size() ? pa[i] : 0;
        const int vb = i < pb.size() ? pb[i] : 0;
        if (va < vb)
            return -1;
        if (va > vb)
            return 1;
    }
    return 0;
}

bool isNewerVersion(const juce::String& candidate, const juce::String& current)
{
    return compareSemver(candidate, current) > 0;
}

} // namespace lyrics

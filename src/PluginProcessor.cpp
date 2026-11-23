#include "PluginProcessor.h"
#include "PluginEditor.h"

LyricsDisplayAudioProcessor::LyricsDisplayAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    auto settings = lyrics_service::loadAppSettings();
    darkTheme.store(settings.darkTheme);
    fontSize.store(settings.fontSize);
    stayOpen.store(settings.stayOpen);
}

void LyricsDisplayAudioProcessor::prepareToPlay(double, int) {}
void LyricsDisplayAudioProcessor::releaseResources() {}

bool LyricsDisplayAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mainOut = layouts.getMainOutputChannelSet();
    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return mainOut == layouts.getMainInputChannelSet();
}

void LyricsDisplayAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Pure transparent pass-through: clear any extra output channels if present
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    if (auto* hostPlayHead = getPlayHead())
    {
        if (const auto pos = hostPlayHead->getPosition())
        {
            hostHasTransport.store(true, std::memory_order_relaxed);
            hostIsPlaying.store(pos->getIsPlaying(), std::memory_order_relaxed);

            if (const auto t = pos->getTimeInSeconds())
                hostTimeSec.store(std::max(0.0, *t), std::memory_order_relaxed);

            if (const auto bpm = pos->getBpm())
                hostBpm.store(*bpm, std::memory_order_relaxed);
        }
    }
}

void LyricsDisplayAudioProcessor::updateTrackProperties(const TrackProperties& properties)
{
    if (properties.name.has_value() && properties.name->trim().isNotEmpty())
    {
        const juce::ScopedLock sl(stateLock);
        if (hostTrackName != *properties.name)
        {
            hostTrackName = properties.name->trim();
            stateVersion.fetch_add(1, std::memory_order_release);
        }
    }
}

LyricsDisplayAudioProcessor::TransportSnapshot LyricsDisplayAudioProcessor::getTransportSnapshot() const noexcept
{
    return {
        hostTimeSec.load(std::memory_order_relaxed),
        hostBpm.load(std::memory_order_relaxed),
        hostIsPlaying.load(std::memory_order_relaxed),
        hostHasTransport.load(std::memory_order_relaxed)
    };
}

LyricsRecord LyricsDisplayAudioProcessor::getLyricsRecord() const
{
    const juce::ScopedLock sl(stateLock);
    return currentLyrics;
}

std::vector<LyricLine> LyricsDisplayAudioProcessor::getParsedLines() const
{
    const juce::ScopedLock sl(stateLock);
    return parsedLines;
}

void LyricsDisplayAudioProcessor::setLyricsRecord(const LyricsRecord& record)
{
    auto parsed = record.hasSynced() ? lyrics::parseLrc(record.syncedLyrics) : ParsedLrc {};
    {
        const juce::ScopedLock sl(stateLock);
        currentLyrics = record;
        parsedLines = std::move(parsed.lines);
    }
    lyricsVersion.fetch_add(1, std::memory_order_release);
    stateVersion.fetch_add(1, std::memory_order_release);
}

void LyricsDisplayAudioProcessor::setLyricsOffset(double newOffsetSec)
{
    {
        const juce::ScopedLock sl(stateLock);
        currentLyrics.offsetSec = newOffsetSec;
    }
    stateVersion.fetch_add(1, std::memory_order_release);
}

juce::String LyricsDisplayAudioProcessor::getHostTrackName() const
{
    const juce::ScopedLock sl(stateLock);
    return hostTrackName;
}

juce::String LyricsDisplayAudioProcessor::getLastSearchQuery() const
{
    const juce::ScopedLock sl(stateLock);
    return lastSearchQuery;
}

void LyricsDisplayAudioProcessor::setLastSearchQuery(const juce::String& query)
{
    const juce::ScopedLock sl(stateLock);
    lastSearchQuery = query;
}

void LyricsDisplayAudioProcessor::setDarkTheme(bool dark)
{
    darkTheme.store(dark);
    lyrics_service::saveAppSettings({ dark, fontSize.load(), stayOpen.load() });
    stateVersion.fetch_add(1, std::memory_order_release);
}

void LyricsDisplayAudioProcessor::setFontSize(float size)
{
    const float clamped = juce::jlimit(13.0f, 38.0f, size);
    fontSize.store(clamped);
    lyrics_service::saveAppSettings({ darkTheme.load(), clamped, stayOpen.load() });
    stateVersion.fetch_add(1, std::memory_order_release);
}

void LyricsDisplayAudioProcessor::setStayOpen(bool enabled)
{
    stayOpen.store(enabled);
    lyrics_service::saveAppSettings({ darkTheme.load(), fontSize.load(), enabled });
    stateVersion.fetch_add(1, std::memory_order_release);
}

void LyricsDisplayAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto* root = new juce::DynamicObject();
    root->setProperty("schemaVersion", 1);

    {
        const juce::ScopedLock sl(stateLock);
        root->setProperty("lyrics", currentLyrics.toVar());
        root->setProperty("lastSearchQuery", lastSearchQuery);
    }

    root->setProperty("darkTheme", darkTheme.load());
    root->setProperty("fontSize", static_cast<double>(fontSize.load()));
    root->setProperty("stayOpen", stayOpen.load());

    const juce::String json = juce::JSON::toString(juce::var(root), false);
    const auto utf8 = json.toRawUTF8();
    destData.replaceAll(utf8, std::strlen(utf8));
}

void LyricsDisplayAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0)
        return;

    const juce::String json = juce::String::fromUTF8(static_cast<const char*>(data), sizeInBytes);
    juce::var parsed;
    if (juce::JSON::parse(json, parsed).failed())
        return;

    if (auto* obj = parsed.getDynamicObject())
    {
        auto rec = LyricsRecord::fromVar(obj->getProperty("lyrics"));
        auto lrcParsed = rec.hasSynced() ? lyrics::parseLrc(rec.syncedLyrics) : ParsedLrc {};

        {
            const juce::ScopedLock sl(stateLock);
            currentLyrics = std::move(rec);
            parsedLines = std::move(lrcParsed.lines);

            auto lastQ = obj->getProperty("lastSearchQuery");
            if (lastQ.isString())
                lastSearchQuery = lastQ.toString();
        }

        auto dark = obj->getProperty("darkTheme");
        if (dark.isBool())
            darkTheme.store(static_cast<bool>(dark));

        auto fs = obj->getProperty("fontSize");
        if (fs.isDouble() || fs.isInt())
            fontSize.store(juce::jlimit(13.0f, 38.0f, static_cast<float>(static_cast<double>(fs))));

        auto stay = obj->getProperty("stayOpen");
        if (stay.isBool())
            stayOpen.store(static_cast<bool>(stay));

        lyricsVersion.fetch_add(1, std::memory_order_release);
        stateVersion.fetch_add(1, std::memory_order_release);
    }
}

juce::AudioProcessorEditor* LyricsDisplayAudioProcessor::createEditor()
{
    return new LyricsDisplayAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LyricsDisplayAudioProcessor();
}

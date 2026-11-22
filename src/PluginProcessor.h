#pragma once

#include "LyricsModel.h"
#include "LyricsService.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <vector>

class LyricsDisplayAudioProcessor : public juce::AudioProcessor
{
public:
    LyricsDisplayAudioProcessor();
    ~LyricsDisplayAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void updateTrackProperties(const TrackProperties& properties) override;

    // Transport telemetry read by editor (lock-free)
    struct TransportSnapshot
    {
        double timeSec = 0.0;
        double bpm = 120.0;
        bool isPlaying = false;
        bool hasHostTransport = false;
    };

    TransportSnapshot getTransportSnapshot() const noexcept;

    // Shared lyrics & UI state persisted with the DAW project
    LyricsRecord getLyricsRecord() const;
    std::vector<LyricLine> getParsedLines() const;
    void setLyricsRecord(const LyricsRecord& record);
    void setLyricsOffset(double newOffsetSec);

    juce::String getHostTrackName() const;
    juce::String getLastSearchQuery() const;
    void setLastSearchQuery(const juce::String& query);

    bool isDarkTheme() const noexcept { return darkTheme.load(); }
    void setDarkTheme(bool dark);

    float getFontSize() const noexcept { return fontSize.load(); }
    void setFontSize(float size);

    // Monotonic counter bumped whenever state changes (so editor can sync cleanly)
    juce::uint64 getStateVersion() const noexcept { return stateVersion.load(); }

private:
    std::atomic<double> hostTimeSec { 0.0 };
    std::atomic<double> hostBpm { 120.0 };
    std::atomic<bool> hostIsPlaying { false };
    std::atomic<bool> hostHasTransport { false };

    mutable juce::CriticalSection stateLock;
    LyricsRecord currentLyrics;
    std::vector<LyricLine> parsedLines;
    juce::String hostTrackName;
    juce::String lastSearchQuery;

    std::atomic<bool> darkTheme { true };
    std::atomic<float> fontSize { 21.0f };
    std::atomic<juce::uint64> stateVersion { 1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LyricsDisplayAudioProcessor)
};

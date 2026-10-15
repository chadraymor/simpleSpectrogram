#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>

class SimpleSpectrogramAudioProcessor final : public juce::AudioProcessor
{
public:
    static constexpr int fftOrder = 12;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int analysisHopSize = fftSize / 4;
    static constexpr int spectrumBinCount = fftSize / 2 + 1;
    static constexpr int frameQueueSize = 64;

    struct SpectrumFrame
    {
        std::array<std::array<float, spectrumBinCount>, 2> magnitudes {};
    };

    SimpleSpectrogramAudioProcessor();
    ~SimpleSpectrogramAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "simpleSpectrogram"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    bool popSpectrumFrame(SpectrumFrame& frame) noexcept;
    double getAnalysisSampleRate() const noexcept;
    float getInputPeak(int channel) const noexcept;

private:
    void analyseWindow() noexcept;
    void publishSpectrum() noexcept;

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window {
        fftSize, juce::dsp::WindowingFunction<float>::hann
    };
    std::array<std::array<float, fftSize>, 2> sampleHistory {};
    std::array<std::array<float, fftSize * 2>, 2> fftData {};
    std::array<SpectrumFrame, frameQueueSize> frameQueue {};
    std::atomic<unsigned int> frameWriteIndex { 0 };
    std::atomic<unsigned int> frameReadIndex { 0 };
    std::atomic<double> analysisSampleRate { 44100.0 };
    std::array<std::atomic<float>, 2> inputPeaks {};
    int historyWritePosition = 0;
    int samplesSinceAnalysis = 0;
    int samplesInHistory = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleSpectrogramAudioProcessor)
};

#include "PluginProcessor.h"
#include "PluginEditor.h"

SimpleSpectrogramAudioProcessor::SimpleSpectrogramAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

void SimpleSpectrogramAudioProcessor::prepareToPlay(double sampleRate, int)
{
    analysisSampleRate.store(sampleRate, std::memory_order_relaxed);
    historyWritePosition = 0;
    samplesSinceAnalysis = 0;
    samplesInHistory = 0;

    for (auto& channel : sampleHistory)
        channel.fill(0.0f);

    for (auto& channel : fftData)
        channel.fill(0.0f);

    for (auto& peak : inputPeaks)
        peak.store(0.0f, std::memory_order_relaxed);

    frameReadIndex.store(0, std::memory_order_relaxed);
    frameWriteIndex.store(0, std::memory_order_relaxed);
}

void SimpleSpectrogramAudioProcessor::releaseResources()
{
}

bool SimpleSpectrogramAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();

    return input == output
        && (input == juce::AudioChannelSet::mono()
            || input == juce::AudioChannelSet::stereo());
}

void SimpleSpectrogramAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto inputChannels = getTotalNumInputChannels();
    const auto outputChannels = getTotalNumOutputChannels();
    const auto sampleCount = buffer.getNumSamples();

    for (int channel = inputChannels; channel < outputChannels; ++channel)
        buffer.clear(channel, 0, sampleCount);

    const auto* left = inputChannels > 0 ? buffer.getReadPointer(0) : nullptr;
    const auto* right = inputChannels > 1 ? buffer.getReadPointer(1) : nullptr;
    std::array<float, 2> blockPeaks {};

    for (int sample = 0; sample < sampleCount; ++sample)
    {
        const auto leftSample = left != nullptr ? left[sample] : 0.0f;
        const auto rightSample = right != nullptr ? right[sample] : 0.0f;
        sampleHistory[0][static_cast<size_t>(historyWritePosition)] =
            leftSample;
        sampleHistory[1][static_cast<size_t>(historyWritePosition)] =
            rightSample;
        blockPeaks[0] = juce::jmax(blockPeaks[0], std::abs(leftSample));
        blockPeaks[1] = juce::jmax(blockPeaks[1], std::abs(rightSample));

        historyWritePosition = (historyWritePosition + 1) % fftSize;
        samplesInHistory = juce::jmin(samplesInHistory + 1, fftSize);

        if (++samplesSinceAnalysis >= analysisHopSize)
        {
            samplesSinceAnalysis = 0;
            if (samplesInHistory == fftSize)
                analyseWindow();
        }
    }

    for (int channel = 0; channel < 2; ++channel)
        inputPeaks[static_cast<size_t>(channel)].store(
            blockPeaks[static_cast<size_t>(channel)], std::memory_order_relaxed);
}

void SimpleSpectrogramAudioProcessor::analyseWindow() noexcept
{
    for (int channel = 0; channel < 2; ++channel)
    {
        auto& data = fftData[static_cast<size_t>(channel)];
        const auto& history = sampleHistory[static_cast<size_t>(channel)];

        for (int sample = 0; sample < fftSize; ++sample)
        {
            const auto index = (historyWritePosition + sample) % fftSize;
            data[static_cast<size_t>(sample)] = history[static_cast<size_t>(index)];
        }

        window.multiplyWithWindowingTable(data.data(), fftSize);
        std::fill(data.begin() + fftSize, data.end(), 0.0f);
        fft.performFrequencyOnlyForwardTransform(data.data(), true);
    }

    publishSpectrum();
}

void SimpleSpectrogramAudioProcessor::publishSpectrum() noexcept
{
    const auto write = frameWriteIndex.load(std::memory_order_relaxed);
    const auto nextWrite = (write + 1U) % static_cast<unsigned int>(frameQueueSize);
    const auto read = frameReadIndex.load(std::memory_order_acquire);

    if (nextWrite == read)
        return;

    auto& frame = frameQueue[write];
    for (int channel = 0; channel < 2; ++channel)
    {
        const auto& data = fftData[static_cast<size_t>(channel)];
        auto& magnitudes = frame.magnitudes[static_cast<size_t>(channel)];
        for (int bin = 0; bin < spectrumBinCount; ++bin)
            magnitudes[static_cast<size_t>(bin)] = data[static_cast<size_t>(bin)];
    }

    frameWriteIndex.store(nextWrite, std::memory_order_release);
}

bool SimpleSpectrogramAudioProcessor::popSpectrumFrame(SpectrumFrame& frame) noexcept
{
    const auto read = frameReadIndex.load(std::memory_order_relaxed);
    const auto write = frameWriteIndex.load(std::memory_order_acquire);

    if (read == write)
        return false;

    frame = frameQueue[read];
    frameReadIndex.store(
        (read + 1U) % static_cast<unsigned int>(frameQueueSize),
        std::memory_order_release);
    return true;
}

double SimpleSpectrogramAudioProcessor::getAnalysisSampleRate() const noexcept
{
    return analysisSampleRate.load(std::memory_order_relaxed);
}

float SimpleSpectrogramAudioProcessor::getInputPeak(int channel) const noexcept
{
    return inputPeaks[static_cast<size_t>(juce::jlimit(0, 1, channel))]
        .load(std::memory_order_relaxed);
}

juce::AudioProcessorEditor* SimpleSpectrogramAudioProcessor::createEditor()
{
    return new SimpleSpectrogramAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleSpectrogramAudioProcessor();
}

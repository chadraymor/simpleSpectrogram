#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class SimpleSpectrogramAudioProcessorEditor final
    : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SimpleSpectrogramAudioProcessorEditor(SimpleSpectrogramAudioProcessor&);
    ~SimpleSpectrogramAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void appendSpectrum(const SimpleSpectrogramAudioProcessor::SpectrumFrame&);
    void resizeSpectrogramImages();
    void drawSpectrumColumn(juce::Image::BitmapData&, int x,
                            const SimpleSpectrogramAudioProcessor::SpectrumFrame&,
                            int channel);
    void drawChannel(juce::Graphics&, int channel, juce::Rectangle<int> area);
    float magnitudeToIntensity(float magnitude) const noexcept;
    float getInterpolatedMagnitude(
        const SimpleSpectrogramAudioProcessor::SpectrumFrame&,
        int channel, int y, int height) const noexcept;

    SimpleSpectrogramAudioProcessor& processor;
    std::array<juce::Image, 2> spectrograms;
    std::array<bool, 2> hasSeededSpectrogram {};
    int imageWidth = 1;
    int imageHeight = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleSpectrogramAudioProcessorEditor)
};

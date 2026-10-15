#include "PluginEditor.h"
#include <algorithm>
#include <cstring>

SimpleSpectrogramAudioProcessorEditor::SimpleSpectrogramAudioProcessorEditor(
    SimpleSpectrogramAudioProcessor& audioProcessor)
    : AudioProcessorEditor(&audioProcessor), processor(audioProcessor)
{
    setResizable(true, true);
    setResizeLimits(500, 360, 1800, 1200);
    setSize(900, 600);
    startTimerHz(30);
}

void SimpleSpectrogramAudioProcessorEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour(0xff050505));
    graphics.setColour(juce::Colour(0xffdddddd));
    graphics.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    graphics.drawText("STEREO SPECTROGRAM", 16, 8, getWidth() - 32, 20,
                      juce::Justification::centredLeft);
    graphics.setFont(juce::Font(juce::FontOptions(11.0f)));
    const auto leftInput = processor.getInputPeak(0);
    const auto rightInput = processor.getInputPeak(1);
    const auto inputStatus = leftInput > 1.0e-7f || rightInput > 1.0e-7f
        ? "IN L " + juce::String(juce::Decibels::gainToDecibels(leftInput), 1)
            + " dB  R " + juce::String(juce::Decibels::gainToDecibels(rightInput), 1)
            + " dB"
        : "NO INPUT";
    graphics.drawText(inputStatus, getWidth() / 2, 8, getWidth() / 2 - 16, 20,
                      juce::Justification::centredRight);

    auto bounds = getLocalBounds().reduced(12);
    bounds.removeFromTop(27);
    const auto gap = 12;
    const auto panelHeight = (bounds.getHeight() - gap) / 2;

    drawChannel(graphics, 0, bounds.removeFromTop(panelHeight));
    bounds.removeFromTop(gap);
    drawChannel(graphics, 1, bounds);
}

void SimpleSpectrogramAudioProcessorEditor::drawChannel(
    juce::Graphics& graphics, int channel, juce::Rectangle<int> area)
{
    graphics.setColour(juce::Colour(0xff101010));
    graphics.fillRect(area);

    auto content = area.reduced(1);
    auto labelArea = content.removeFromTop(18);
    graphics.setColour(juce::Colour(0xff9a9a9a));
    graphics.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    graphics.drawText(channel == 0 ? "LEFT" : "RIGHT",
                      labelArea.reduced(7, 0), juce::Justification::centredLeft);

    graphics.setColour(juce::Colour(0xff292929));
    graphics.drawRect(area);
    graphics.drawImageAt(spectrograms[static_cast<size_t>(channel)],
                         content.getX(), content.getY());
}

void SimpleSpectrogramAudioProcessorEditor::resized()
{
    resizeSpectrogramImages();
    repaint();
}

void SimpleSpectrogramAudioProcessorEditor::resizeSpectrogramImages()
{
    const auto bounds = getLocalBounds().reduced(12);
    const auto panelHeight = (bounds.getHeight() - 27 - 12) / 2;
    const auto newWidth = juce::jmax(1, bounds.getWidth() - 2);
    const auto newHeight = juce::jmax(1, panelHeight - 20);

    if (newWidth == imageWidth && newHeight == imageHeight
        && !spectrograms[0].isNull())
        return;

    imageWidth = newWidth;
    imageHeight = newHeight;

    for (auto& image : spectrograms)
    {
        const auto previous = image;
        image = juce::Image(juce::Image::RGB, imageWidth, imageHeight, true);

        juce::Graphics imageGraphics(image);
        imageGraphics.fillAll(juce::Colours::black);
        if (!previous.isNull())
            imageGraphics.drawImage(previous,
                                    juce::Rectangle<int>(imageWidth, imageHeight).toFloat(),
                                    juce::RectanglePlacement::stretchToFit);
    }
}

void SimpleSpectrogramAudioProcessorEditor::timerCallback()
{
    SimpleSpectrogramAudioProcessor::SpectrumFrame frame;
    bool changed = false;

    while (processor.popSpectrumFrame(frame))
    {
        appendSpectrum(frame);
        changed = true;
    }

    if (changed || processor.getInputPeak(0) > 1.0e-7f
        || processor.getInputPeak(1) > 1.0e-7f)
        repaint();
}

void SimpleSpectrogramAudioProcessorEditor::appendSpectrum(
    const SimpleSpectrogramAudioProcessor::SpectrumFrame& frame)
{
    for (int channel = 0; channel < 2; ++channel)
    {
        auto& image = spectrograms[static_cast<size_t>(channel)];
        const auto& magnitudes = frame.magnitudes[static_cast<size_t>(channel)];
        const auto hasSignal = std::any_of(
            magnitudes.begin() + 1, magnitudes.end(),
            [](float magnitude) { return magnitude > 1.0e-7f; });

        if (!hasSeededSpectrogram[static_cast<size_t>(channel)]
            && hasSignal)
        {
            juce::Image::BitmapData pixels(image, juce::Image::BitmapData::writeOnly);
            for (int x = 0; x < imageWidth; ++x)
                drawSpectrumColumn(pixels, x, frame, channel);

            hasSeededSpectrogram[static_cast<size_t>(channel)] = true;
            continue;
        }

        juce::Image::BitmapData pixels(image, juce::Image::BitmapData::readWrite);
        const auto rowBytes = static_cast<size_t>(imageWidth - 1)
            * static_cast<size_t>(pixels.pixelStride);
        for (int y = 0; y < imageHeight; ++y)
        {
            auto* row = pixels.getLinePointer(y);
            std::memmove(row, row + pixels.pixelStride, rowBytes);
        }

        drawSpectrumColumn(pixels, imageWidth - 1, frame, channel);
    }
}

void SimpleSpectrogramAudioProcessorEditor::drawSpectrumColumn(
    juce::Image::BitmapData& pixels, int x,
    const SimpleSpectrogramAudioProcessor::SpectrumFrame& frame,
    int channel)
{
    for (int y = 0; y < imageHeight; ++y)
    {
        const auto intensity = magnitudeToIntensity(
            getInterpolatedMagnitude(frame, channel, y, imageHeight));
        const auto level = juce::roundToInt(intensity * 255.0f);
        pixels.setPixelColour(x, y,
                              juce::Colour(static_cast<juce::uint8>(level),
                                           static_cast<juce::uint8>(level),
                                           static_cast<juce::uint8>(level)));
    }
}

float SimpleSpectrogramAudioProcessorEditor::magnitudeToIntensity(
    float magnitude) const noexcept
{
    if (magnitude <= 0.0f)
        return 0.0f;

    const auto amplitude = magnitude
        * (4.0f / static_cast<float>(SimpleSpectrogramAudioProcessor::fftSize));
    const auto decibels = juce::Decibels::gainToDecibels(amplitude, -96.0f);
    const auto scaled = juce::jlimit(0.0f, 1.0f, (decibels + 96.0f) / 96.0f);
    return std::pow(scaled, 2.0f);
}

float SimpleSpectrogramAudioProcessorEditor::getInterpolatedMagnitude(
    const SimpleSpectrogramAudioProcessor::SpectrumFrame& frame,
    int channel, int y, int height) const noexcept
{
    constexpr float minimumFrequency = 20.0f;
    constexpr float maximumDisplayFrequency = 18000.0f;
    const auto nyquist = static_cast<float>(processor.getAnalysisSampleRate() * 0.5);
    const auto maximumFrequency = juce::jmax(minimumFrequency * 1.01f,
        juce::jmin(maximumDisplayFrequency, nyquist));
    const auto safeHeight = static_cast<float>(juce::jmax(1, height));
    const auto logRange = std::log(maximumFrequency / minimumFrequency);
    const auto position = 1.0f - (static_cast<float>(y) + 0.5f) / safeHeight;
    const auto frequency = minimumFrequency * std::exp(logRange * position);
    const auto binScale = static_cast<float>(SimpleSpectrogramAudioProcessor::fftSize)
        / nyquist;
    const auto fractionalBin = juce::jlimit(
        1.0f, static_cast<float>(SimpleSpectrogramAudioProcessor::fftSize / 2 - 1),
        frequency * binScale);
    const auto firstBin = static_cast<int>(std::floor(fractionalBin));
    const auto secondBin = juce::jmin(
        firstBin + 1, SimpleSpectrogramAudioProcessor::fftSize / 2 - 1);
    const auto fraction = fractionalBin - static_cast<float>(firstBin);
    const auto& magnitudes = frame.magnitudes[static_cast<size_t>(channel)];
    const auto firstMagnitude = magnitudes[static_cast<size_t>(firstBin)];
    const auto secondMagnitude = magnitudes[static_cast<size_t>(secondBin)];
    return firstMagnitude + fraction * (secondMagnitude - firstMagnitude);
}

# simpleSpectrogram

A Windows VST3 audio-effect plugin that recreates the two stacked, monochrome
scrolling spectrograms in the supplied reference. It analyzes the
left and right input independently, uses a 4096-point Hann-windowed FFT with
75% overlap, and maps 20 Hz–18 kHz logarithmically from bottom to top. Audio
passes through unchanged.

## Build

Requirements: Windows, CMake 3.22 or later, and Visual Studio with the C++
workload installed. CMake downloads JUCE 8.0.10 from its official repository
on the first configure.

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release --target simpleSpectrogram_VST3
```

The resulting plugin bundle is under
`build/simpleSpectrogram_artefacts/Release/VST3/simpleSpectrogram.vst3`.
Copy that bundle to `%CommonProgramFiles%\VST3`, then rescan plug-ins in Ableton.

## Display

Each channel scrolls from right to left, with new time at the right edge.
Frequency is logarithmic; brighter pixels indicate stronger energy. Brightness
uses a consistent -96 dBFS to 0 dBFS scale rather than being normalized to each
frame's loudest FFT bin, preserving contrast between the bass and high
frequencies. Display rows interpolate directly between FFT bins without
additional smoothing. The image matches the display's pixel dimensions to
avoid blurring from scaling. New spectra are drawn within an 18 kHz
upper-frequency limit. When a channel first receives audio, its initial
spectrum fills the display; subsequent FFT frames scroll from right to left.
The header shows each input peak; use `NO INPUT` to check that audio is routed
through the track containing the plugin. The plugin is an analyzer/effect and
does not alter the audio.

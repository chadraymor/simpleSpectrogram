cmake_minimum_required(VERSION 3.22)
project(simpleSpectrogram VERSION 1.0.0 LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.10
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(simpleSpectrogram
    COMPANY_NAME "simpleSpectrogram"
    BUNDLE_ID "com.simplespectrogram.analyzer"
    PLUGIN_MANUFACTURER_CODE Fbar
    PLUGIN_CODE Fspc
    PRODUCT_NAME "simpleSpectrogram"
    FORMATS VST3
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(simpleSpectrogram PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

juce_generate_juce_header(simpleSpectrogram)

target_compile_features(simpleSpectrogram PRIVATE cxx_std_17)
target_compile_definitions(simpleSpectrogram PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)
target_link_libraries(simpleSpectrogram PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

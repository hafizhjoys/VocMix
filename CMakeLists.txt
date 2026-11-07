cmake_minimum_required(VERSION 3.22)
project(VocMix VERSION 0.1.0)

# Letakkan folder JUCE di sebelah file ini:  git clone https://github.com/juce-framework/JUCE.git
add_subdirectory(JUCE)

juce_add_plugin(VocMix
    COMPANY_NAME "YourName"
    PLUGIN_MANUFACTURER_CODE Yrnm
    PLUGIN_CODE Mxfv
    FORMATS VST3 Standalone
    PRODUCT_NAME "VocMix"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(VocMix)

target_sources(VocMix PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h)

target_compile_definitions(VocMix PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(VocMix
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC  juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)

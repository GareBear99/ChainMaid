cmake_minimum_required(VERSION 3.22)
project(ChainMaid VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE setup:
# Provide JUCE_DIR (folder containing JUCEConfig.cmake) OR JUCE_PATH
# (path to a JUCE source checkout).
if (DEFINED JUCE_DIR)
    list(APPEND CMAKE_PREFIX_PATH "${JUCE_DIR}")
    find_package(JUCE CONFIG REQUIRED)
elseif(DEFINED JUCE_PATH)
    add_subdirectory("${JUCE_PATH}" JUCE)
else()
    message(FATAL_ERROR "Please set JUCE_DIR or JUCE_PATH to your local JUCE folder.")
endif()

juce_add_plugin(ChainMaid
    COMPANY_NAME "TizWildin Entertainment"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE TWE1
    PLUGIN_CODE ChMd
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "ChainMaid")

target_sources(ChainMaid PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/SideChainEngine.h)

target_compile_definitions(ChainMaid PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(ChainMaid PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra)

juce_generate_juce_header(ChainMaid)

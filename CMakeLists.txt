cmake_minimum_required(VERSION 3.22)

project(HardTune VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ------------------------------------------------------------------
# JUCE (fetched at configure time, pinned for reproducible builds)
# ------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.8
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ------------------------------------------------------------------
# Plugin target
# ------------------------------------------------------------------
juce_add_plugin(HardTune
    PRODUCT_NAME                "HardTune"
    COMPANY_NAME                "Belize2026"
    BUNDLE_ID                   "com.belize2026.hardtune"
    PLUGIN_MANUFACTURER_CODE    Blze
    PLUGIN_CODE                 Htun
    FORMATS                     VST3 AU Standalone
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     TRUE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT  "HardTune needs audio input for the standalone app."
    VST3_CATEGORIES             "Fx" "Pitch Shift"
    AU_MAIN_TYPE                "kAudioUnitType_Effect"
)

target_sources(HardTune PRIVATE
    source/PluginProcessor.cpp
    source/PluginEditor.cpp
)

target_include_directories(HardTune PRIVATE source)

target_compile_definitions(HardTune PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(HardTune
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ------------------------------------------------------------------
# Headless DSP tests (pure C++, no JUCE dependency)
# ------------------------------------------------------------------
enable_testing()
add_executable(HardTuneDspTests tests/DspTests.cpp)
target_include_directories(HardTuneDspTests PRIVATE source)
target_compile_features(HardTuneDspTests PRIVATE cxx_std_17)
add_test(NAME DspTests COMMAND HardTuneDspTests)

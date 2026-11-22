cmake_minimum_required(VERSION 3.22)

project(LyricsDisplay VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8 /W3)
endif()

add_subdirectory(third_party/JUCE)

juce_add_plugin(LyricsDisplay
    COMPANY_NAME "AClouD"
    BUNDLE_ID "com.acloud.lyricsdisplay"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Acld
    PLUGIN_CODE LyrD
    FORMATS VST3 Standalone
    PRODUCT_NAME "Lyrics Display"
    VST3_CATEGORIES "Fx" "Tools"
)

target_sources(LyricsDisplay
    PRIVATE
        src/LyricsModel.h
        src/LyricsModel.cpp
        src/LyricsService.h
        src/LyricsService.cpp
        src/Theme.h
        src/LyricsViewport.h
        src/LyricsViewport.cpp
        src/PluginProcessor.h
        src/PluginProcessor.cpp
        src/PluginEditor.h
        src/PluginEditor.cpp
)

target_compile_definitions(LyricsDisplay
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_MODAL_LOOPS_PERMITTED=0
)

target_link_libraries(LyricsDisplay
    PRIVATE
        juce::juce_audio_utils
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Standalone unit test executable for core lyrics model & LRC parser
add_executable(LyricsModelTests
    tests/LyricsModelTests.cpp
    src/LyricsModel.h
    src/LyricsModel.cpp
)

target_include_directories(LyricsModelTests PRIVATE src)
target_link_libraries(LyricsModelTests
    PRIVATE
        juce::juce_core
)
target_compile_definitions(LyricsModelTests
    PRIVATE
        JUCE_STANDALONE_APPLICATION=1
)

cmake_minimum_required(VERSION 3.22)

# macOS builds target Apple silicon and macOS 14.7 unless the cache already says otherwise.
if(APPLE)
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "14.7" CACHE STRING "Minimum macOS version" FORCE)
    endif()
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS architectures" FORCE)
    endif()
endif()

project(ORBIT VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8)
endif()

option(ORBIT_BUILD_PLUGIN "Build the JUCE plugin targets" ON)

# Engine tests do not link JUCE. They are the sound contract.
add_executable(orbit_render_test tests/render_test.cpp)
add_executable(orbit_loudness_test tests/loudness_test.cpp)
add_executable(orbit_bloom_ab tests/bloom_ab.cpp)
add_executable(orbit_bloom_zone_ab tests/bloom_zone_ab.cpp)

foreach(test_target orbit_render_test orbit_loudness_test orbit_bloom_ab orbit_bloom_zone_ab)
    target_include_directories(${test_target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    target_compile_features(${test_target} PRIVATE cxx_std_17)
endforeach()

enable_testing()
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/renders)
add_test(NAME engine COMMAND orbit_render_test ${CMAKE_CURRENT_BINARY_DIR}/renders)
add_test(NAME loudness COMMAND orbit_loudness_test --check)
set_tests_properties(engine PROPERTIES TIMEOUT 900)
set_tests_properties(loudness PROPERTIES TIMEOUT 600)

if(ORBIT_BUILD_PLUGIN)
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.15
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)

    # AU is a macOS format. Linux and Windows build VST3 and Standalone.
    if(APPLE)
        set(ORBIT_FORMATS VST3 AU Standalone)
    else()
        set(ORBIT_FORMATS VST3 Standalone)
    endif()

    set(ORBIT_PLUGIN_CODE OrGw)
    set(ORBIT_MANUFACTURER_CODE Brfd)

    juce_add_plugin(ORBIT
        COMPANY_NAME "Bearfield"
        BUNDLE_ID com.bearfield.ORBIT
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        NEEDS_CURL FALSE
        NEEDS_WEB_BROWSER FALSE
        PLUGIN_MANUFACTURER_CODE ${ORBIT_MANUFACTURER_CODE}
        PLUGIN_CODE ${ORBIT_PLUGIN_CODE}
        FORMATS ${ORBIT_FORMATS}
        PRODUCT_NAME "ORBIT"
        AU_MAIN_TYPE kAudioUnitType_Effect
        VST3_CATEGORIES Fx Modulation)

    juce_generate_juce_header(ORBIT)

    target_sources(ORBIT
        PRIVATE
            Source/PluginProcessor.cpp
            Source/PluginEditor.cpp
            Source/OrbitLookAndFeel.cpp
            Source/GravityField.cpp)

    target_include_directories(ORBIT PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)

    target_compile_definitions(ORBIT
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_DISPLAY_SPLASH_SCREEN=0
            JUCE_REPORT_APP_USAGE=0
            JUCE_VST3_CAN_REPLACE_VST2=0)

    target_link_libraries(ORBIT
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)

    juce_add_console_app(orbit_gui_snapshot
        PRODUCT_NAME "orbit_gui_snapshot"
        NEEDS_CURL FALSE
        NEEDS_WEB_BROWSER FALSE)
    juce_generate_juce_header(orbit_gui_snapshot)
    target_sources(orbit_gui_snapshot
        PRIVATE
            tests/gui_snapshot.cpp
            Source/PluginProcessor.cpp
            Source/PluginEditor.cpp
            Source/OrbitLookAndFeel.cpp
            Source/GravityField.cpp)
    target_include_directories(orbit_gui_snapshot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)
    target_compile_definitions(orbit_gui_snapshot
        PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_DISPLAY_SPLASH_SCREEN=0
            JUCE_REPORT_APP_USAGE=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            JucePlugin_Name="ORBIT"
            JUCE_STANDALONE_APPLICATION=1)
    target_link_libraries(orbit_gui_snapshot
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)
endif()

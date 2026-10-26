cmake_minimum_required(VERSION 3.22)
project(ORBIT_GravityWarp VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ORBIT_BUILD_PLUGIN "Build JUCE plugin" ON)

# Engine-only offline tests (no JUCE)
add_executable(orbit_render_test tests/render_test.cpp)
add_executable(orbit_bloom_ab tests/bloom_ab.cpp)
target_include_directories(orbit_bloom_ab PRIVATE Source)
target_include_directories(orbit_render_test PRIVATE Source)

if(ORBIT_BUILD_PLUGIN)
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.6 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)

    set(FORMATS VST3 Standalone)
    if(APPLE)
        list(APPEND FORMATS AU)
    endif()

    juce_add_plugin(ORBIT_GravityWarp
        COMPANY_NAME "PLUGIN LAB"
        BUNDLE_ID "com.pluginlab.orbit.gravitywarp"
        PLUGIN_MANUFACTURER_CODE PLab
        PLUGIN_CODE OrGw
        FORMATS ${FORMATS}
        PRODUCT_NAME "ORBIT Gravity Warp"
        PLUGIN_NAME "ORBIT Gravity Warp"
        DESCRIPTION "ORBIT - Gravity Warp: black-hole orbit chorus / tape wow & flutter"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES Fx Modulation
        AU_MAIN_TYPE kAudioUnitType_Effect)

    juce_generate_juce_header(ORBIT_GravityWarp)
    target_sources(ORBIT_GravityWarp PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/OrbitLookAndFeel.cpp Source/GravityField.cpp)
    target_compile_definitions(ORBIT_GravityWarp PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
    target_link_libraries(ORBIT_GravityWarp
        PRIVATE juce::juce_audio_utils juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
endif()

# offscreen GUI snapshot tool (docs/gui_m2.png)
if(TARGET juce::juce_audio_utils)
    juce_add_console_app(orbit_gui_snapshot PRODUCT_NAME "orbit_gui_snapshot")
    juce_generate_juce_header(orbit_gui_snapshot)
    target_sources(orbit_gui_snapshot PRIVATE tests/gui_snapshot.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/OrbitLookAndFeel.cpp Source/GravityField.cpp)
    target_compile_definitions(orbit_gui_snapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="ORBIT Gravity Warp" JUCE_STANDALONE_APPLICATION=1)
    target_link_libraries(orbit_gui_snapshot PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endif()

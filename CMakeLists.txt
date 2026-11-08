cmake_minimum_required(VERSION 3.22)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC runtime library")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(PurpleTuba VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(PURPLETUBA_BUILD_TESTS "Build the Catch2 unit tests" ON)
option(PURPLETUBA_COPY_AFTER_BUILD "Copy plug-ins into the system plug-in folders after building" OFF)

if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libs/JUCE/CMakeLists.txt")
    message(FATAL_ERROR "Submodules are missing. Run: git submodule update --init --recursive")
endif()

add_subdirectory(libs/JUCE)

set(PURPLETUBA_FORMATS VST3 Standalone)
if (APPLE)
    list(APPEND PURPLETUBA_FORMATS AU)
endif()

juce_add_plugin(PurpleTuba
    COMPANY_NAME "Purple Tuba"
    PRODUCT_NAME "Purple Tuba"
    PLUGIN_MANUFACTURER_CODE Prpt
    PLUGIN_CODE PTba
    BUNDLE_ID "com.purpletuba.purpletuba"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES Instrument Synth Mono
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    COPY_PLUGIN_AFTER_BUILD ${PURPLETUBA_COPY_AFTER_BUILD}
    FORMATS ${PURPLETUBA_FORMATS})

file(GLOB_RECURSE PURPLETUBA_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
target_sources(PurpleTuba PRIVATE ${PURPLETUBA_SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source" FILES ${PURPLETUBA_SOURCES})
target_include_directories(PurpleTuba PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_definitions(PurpleTuba
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        # Hosts send MIDI CCs to VST3 plug-ins only through parameters; JUCE turns
        # them back into MIDI messages so MIDI learn sees them. (This is JUCE's default.)
        JUCE_VST3_EMULATE_MIDI_CC_WITH_PARAMETERS=1
        PURPLETUBA_VERSION="${PROJECT_VERSION}")

target_link_libraries(PurpleTuba
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# The announcer raises UI Automation notification events on Windows.
if (WIN32)
    target_link_libraries(PurpleTuba PRIVATE oleaut32)
endif()

# CLAP, through clap-juce-extensions.
add_subdirectory(libs/clap-juce-extensions EXCLUDE_FROM_ALL)
clap_juce_extensions_plugin(TARGET PurpleTuba
    CLAP_ID "com.purpletuba.purpletuba"
    CLAP_FEATURES instrument synthesizer mono)

if (PURPLETUBA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(libs/Catch2 EXCLUDE_FROM_ALL)
    add_subdirectory(tests)
endif()

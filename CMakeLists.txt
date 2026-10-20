cmake_minimum_required(VERSION 3.22)

# BIT ERA by TrapWin - its own project, built from this folder: cmake -S bitera -B build-bitera
# Version lives here only (CI reads it from this line for the installers).
project(BIT_ERA VERSION 0.6.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# --- JUCE (same pin as EVOLVE / CHROME VAULT) ---------------------------------
set(BE_JUCE_DIR "" CACHE PATH "Local JUCE checkout (optional)")
if(BE_JUCE_DIR)
    add_subdirectory(${BE_JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.15
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

# Identity - NEVER change the codes after release, hosts identify saved projects by them.
juce_add_plugin(BitEra
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "TrapWin"
    BUNDLE_ID com.trapwin.bitera
    PLUGIN_MANUFACTURER_CODE Trpw
    PLUGIN_CODE Btea
    PRODUCT_NAME "Bit Era"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    FORMATS VST3 AU Standalone
    COPY_PLUGIN_AFTER_BUILD FALSE)

file(GLOB BE_SOUNDS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/sounds/*.wav)
file(GLOB BE_ERAS   CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/eras/*.jpg)
file(GLOB BE_FONTS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/*.ttf)
juce_add_binary_data(BitEraData HEADER_NAME BitEraData.h NAMESPACE BitEraData SOURCES ${BE_SOUNDS} ${BE_ERAS} ${BE_FONTS})

target_sources(BitEra PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Eras.cpp
    Source/Chips.cpp
    Source/Engine.cpp
    Source/Fx.cpp
    Source/Presets.cpp
    Source/Instruments.cpp)

target_compile_definitions(BitEra PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    BE_VERSION_STRING="${PROJECT_VERSION}")

# Linux is only used for quick local compile checks (no ALSA / Xrandr headers needed)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(BitEra PUBLIC JUCE_ALSA=0 JUCE_JACK=0 JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0)
endif()

target_link_libraries(BitEra
    PRIVATE
        BitEraData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Render test: loads every sound, plays every preset through the processor, checks for silence / NaN / clipping
option(BE_BUILD_TESTS "Build the Bit Era render test" ON)
if(BE_BUILD_TESTS)
    juce_add_console_app(BitEraTests PRODUCT_NAME "BitEraTests")
    target_sources(BitEraTests PRIVATE tests/RenderTest.cpp Source/PluginProcessor.cpp Source/Eras.cpp Source/Chips.cpp Source/Engine.cpp Source/Fx.cpp Source/Presets.cpp Source/Instruments.cpp)
    target_compile_definitions(BitEraTests PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 BE_HEADLESS=1 BE_VERSION_STRING="${PROJECT_VERSION}"
        JucePlugin_Name="Bit Era" JucePlugin_IsSynth=1 JucePlugin_WantsMidiInput=1 JucePlugin_ProducesMidiOutput=0
        JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="${PROJECT_VERSION}")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_compile_definitions(BitEraTests PRIVATE JUCE_ALSA=0 JUCE_JACK=0)
    endif()
    target_link_libraries(BitEraTests PRIVATE BitEraData juce::juce_audio_processors juce::juce_audio_formats juce::juce_dsp
        juce::juce_recommended_config_flags)

    # UI snapshot tool: renders the window of every era to PNG (no window opened) - for design checks
    juce_add_console_app(BitEraSnapshot PRODUCT_NAME "BitEraSnapshot")
    target_sources(BitEraSnapshot PRIVATE tests/Snapshot.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/Eras.cpp Source/Chips.cpp Source/Engine.cpp Source/Fx.cpp Source/Presets.cpp Source/Instruments.cpp)
    target_compile_definitions(BitEraSnapshot PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 BE_VERSION_STRING="${PROJECT_VERSION}"
        JucePlugin_Name="Bit Era" JucePlugin_IsSynth=1 JucePlugin_WantsMidiInput=1 JucePlugin_ProducesMidiOutput=0
        JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="${PROJECT_VERSION}" JucePlugin_Build_Standalone=0)
    target_link_libraries(BitEraSnapshot PRIVATE BitEraData juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)

    juce_add_console_app(BitEraPatchDump PRODUCT_NAME "BitEraPatchDump")
    target_sources(BitEraPatchDump PRIVATE tests/PatchDump.cpp Source/Eras.cpp Source/Chips.cpp Source/Engine.cpp Source/Fx.cpp Source/Presets.cpp Source/Instruments.cpp)
    target_compile_definitions(BitEraPatchDump PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(BitEraPatchDump PRIVATE juce::juce_graphics juce::juce_audio_basics juce::juce_dsp juce::juce_recommended_config_flags)

    # demo renders of chosen presets (for listening without a DAW)
    juce_add_console_app(BitEraRenderDemo PRODUCT_NAME "BitEraRenderDemo")
    target_sources(BitEraRenderDemo PRIVATE tests/RenderDemo.cpp Source/PluginProcessor.cpp Source/Eras.cpp Source/Chips.cpp Source/Engine.cpp Source/Fx.cpp Source/Presets.cpp Source/Instruments.cpp)
    target_compile_definitions(BitEraRenderDemo PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 BE_HEADLESS=1 BE_VERSION_STRING="${PROJECT_VERSION}"
        JucePlugin_Name="Bit Era" JucePlugin_IsSynth=1 JucePlugin_WantsMidiInput=1 JucePlugin_ProducesMidiOutput=0
        JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="${PROJECT_VERSION}")
    target_link_libraries(BitEraRenderDemo PRIVATE BitEraData juce::juce_audio_processors juce::juce_audio_formats juce::juce_dsp juce::juce_recommended_config_flags)
endif()

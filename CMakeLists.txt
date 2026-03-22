cmake_minimum_required(VERSION 3.22)
project(MidiEnvelopePlugin VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Platform detection ─────────────────────────────────────────────────────────
# iOS build: cmake -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DDAN_TEAM_ID=XXXXXXXXXX
# macOS build: cmake -G "Unix Makefiles"   (default)
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(DAN_IOS TRUE)
    set(CMAKE_OSX_ARCHITECTURES     "arm64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0"  CACHE STRING "" FORCE)
    set(_DAN_FORMATS    AUv3 Standalone)
    set(_IS_MIDI_EFFECT TRUE)
    set(_IS_MIDI_EFFECT_NUM 1)
    # Generate .xcscheme files so xcodebuild -scheme and -archivePath work
    set(CMAKE_XCODE_GENERATE_SCHEME YES)
    message(STATUS "Dan: building for iOS — AUv3 + Standalone")
else()
    set(DAN_IOS FALSE)
    set(CMAKE_OSX_ARCHITECTURES     "arm64;x86_64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13"        CACHE STRING "" FORCE)
    set(_DAN_FORMATS    VST3 AU Standalone)
    set(_IS_MIDI_EFFECT FALSE)
    set(_IS_MIDI_EFFECT_NUM 0)
    message(STATUS "Dan: building for macOS — VST3 + AU + CLAP + Standalone")
endif()

# User-settable at configure time
set(DAN_TEAM_ID   ""                   CACHE STRING "Apple Developer Team ID (10-char, required for iOS)")
set(DAN_BUNDLE_ID "com.umaruwaste.dan" CACHE STRING "Base bundle identifier")

# ── Dependencies ───────────────────────────────────────────────────────────────
include(FetchContent)

FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.9
    GIT_SHALLOW    TRUE
)

if(NOT DAN_IOS)
    FetchContent_Declare(clap_juce_extensions
        GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
        GIT_TAG        main
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE clap_juce_extensions)
else()
    FetchContent_MakeAvailable(JUCE)
endif()

# ── Plugin target ──────────────────────────────────────────────────────────────
juce_add_plugin(MidiEnvelopePlugin
    COMPANY_NAME             "Umaru Industrial Waste"
    PLUGIN_MANUFACTURER_CODE UmIW
    PLUGIN_CODE              DnEv
    FORMATS                  ${_DAN_FORMATS}
    PRODUCT_NAME             "Dan"
    IS_SYNTH                 FALSE
    NEEDS_MIDI_INPUT         TRUE
    NEEDS_MIDI_OUTPUT        TRUE
    IS_MIDI_EFFECT           ${_IS_MIDI_EFFECT}
    VST3_CATEGORIES          "Fx" "MIDI"
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD  FALSE
    VERSION                  0.1.0
    BUNDLE_ID                "${DAN_BUNDLE_ID}"
)

# ── CLAP wrapper (macOS only — not supported on iOS) ──────────────────────────
if(NOT DAN_IOS)
    clap_juce_extensions_plugin(
        TARGET        MidiEnvelopePlugin
        CLAP_ID       "${DAN_BUNDLE_ID}"
        CLAP_FEATURES "midi-effect" "utility"
    )
endif()

# ── iOS code signing ───────────────────────────────────────────────────────────
# Pass -DDAN_TEAM_ID=XXXXXXXXXX at cmake configure time.
# Signing style is Automatic so Xcode manages provisioning profiles.
if(DAN_IOS)
    if(NOT DAN_TEAM_ID)
        message(WARNING "DAN_TEAM_ID not set — Xcode signing will be unconfigured. "
                        "Pass -DDAN_TEAM_ID=<10-char-team-id> to cmake.")
    endif()

    foreach(_t MidiEnvelopePlugin_Standalone MidiEnvelopePlugin_AUv3)
        if(TARGET ${_t})
            set_target_properties(${_t} PROPERTIES
                XCODE_ATTRIBUTE_DEVELOPMENT_TEAM             "${DAN_TEAM_ID}"
                XCODE_ATTRIBUTE_CODE_SIGN_STYLE              "Automatic"
                XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY           "iPhone Developer"
                XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY       "1,2"
                XCODE_ATTRIBUTE_ENABLE_BITCODE               "NO"
                XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET   "15.0"
            )
        endif()
    endforeach()
endif()

# ── Sources ────────────────────────────────────────────────────────────────────
target_sources(MidiEnvelopePlugin PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/EnvelopePlayer.cpp
    Source/EnvelopeEditorComponent.cpp
    Source/KeyMapperComponent.cpp
    Source/ScopeComponent.cpp
)

target_include_directories(MidiEnvelopePlugin PRIVATE Source)

juce_generate_juce_header(MidiEnvelopePlugin)

target_compile_definitions(MidiEnvelopePlugin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        # Controls isMidiEffect() and bus layout in PluginProcessor.
        # TRUE on iOS (aumi AUv3 for AUM), FALSE on macOS (stereo buses for Ableton VST3).
        DAN_IS_MIDI_EFFECT=${_IS_MIDI_EFFECT_NUM}
)

target_link_libraries(MidiEnvelopePlugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

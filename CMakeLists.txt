cmake_minimum_required(VERSION 3.22)
project(MidiEnvelopePlugin VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Platform detection ─────────────────────────────────────────────────────────
# iOS build:     cmake -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DDAN_TEAM_ID=XXXXXXXXXX
# macOS build:   cmake -G "Unix Makefiles"   (default)
# Windows build: cmake -B build-win          (auto-detects Visual Studio / Ninja)
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(DAN_IOS TRUE)
    set(DAN_WIN FALSE)
    set(CMAKE_OSX_ARCHITECTURES     "arm64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0"  CACHE STRING "" FORCE)
    set(_DAN_FORMATS    AUv3 Standalone)
    set(_IS_MIDI_EFFECT TRUE)
    set(_IS_MIDI_EFFECT_NUM 1)
    # Generate .xcscheme files so xcodebuild -scheme and -archivePath work
    set(CMAKE_XCODE_GENERATE_SCHEME YES)
    message(STATUS "Dan: building for iOS — AUv3 + Standalone")
elseif(WIN32)
    set(DAN_IOS FALSE)
    set(DAN_WIN TRUE)
    set(_DAN_FORMATS    VST3 Standalone)
    set(_IS_MIDI_EFFECT FALSE)
    set(_IS_MIDI_EFFECT_NUM 0)
    message(STATUS "Dan: building for Windows — VST3 + CLAP + Standalone")
else()
    set(DAN_IOS FALSE)
    set(DAN_WIN FALSE)
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

# ── Embedded icon resources ───────────────────────────────────────────────────
juce_add_binary_data(DanResources
    SOURCES
        Resources/icon_add.svg
        Resources/icon_loop.svg
        Resources/icon_envelope.svg
        Resources/icon_keymap.svg
        Resources/icon_scope.svg
        Resources/icon_sweep.svg
        Resources/icon_midiout.svg
)

# ── App icon (generated from Resources/dan_icon.svg at configure time) ────────
# macOS:   requires rsvg-convert  →  brew install librsvg
# Windows: additionally benefits from ImageMagick  →  winget install ImageMagick
set(_DAN_ICON_BIG_ARG "")
find_program(_RSVG rsvg-convert)
if(_RSVG)
    if(APPLE AND NOT DAN_IOS)
        # Build a proper multi-resolution .icns from an iconset
        set(_ICONSET "${CMAKE_CURRENT_BINARY_DIR}/Dan.iconset")
        set(_ICNS    "${CMAKE_CURRENT_BINARY_DIR}/Dan.icns")
        file(MAKE_DIRECTORY "${_ICONSET}")
        foreach(_PAIR IN ITEMS
            "16,icon_16x16.png"           "32,icon_16x16@2x.png"
            "32,icon_32x32.png"           "64,icon_32x32@2x.png"
            "128,icon_128x128.png"        "256,icon_128x128@2x.png"
            "256,icon_256x256.png"        "512,icon_256x256@2x.png"
            "512,icon_512x512.png"        "1024,icon_512x512@2x.png")
            string(REGEX MATCH "^([0-9]+),(.*)" _ "${_PAIR}")
            execute_process(COMMAND "${_RSVG}"
                -w "${CMAKE_MATCH_1}" -h "${CMAKE_MATCH_1}"
                "${CMAKE_CURRENT_SOURCE_DIR}/Resources/dan_icon.svg"
                -o "${_ICONSET}/${CMAKE_MATCH_2}")
        endforeach()
        execute_process(
            COMMAND iconutil -c icns "${_ICONSET}" -o "${_ICNS}"
            RESULT_VARIABLE _ICONUTIL_RC)
        if(_ICONUTIL_RC EQUAL 0)
            set(_DAN_ICON_BIG_ARG ICON_BIG "${_ICNS}")
            message(STATUS "Dan: icon → ${_ICNS}")
        else()
            message(WARNING "Dan: iconutil failed (RC=${_ICONUTIL_RC}) — building without icon")
        endif()
    elseif(DAN_IOS)
        # iOS: a 1024×1024 PNG is sufficient; Xcode/JUCE scale it for all icon slots
        set(_ICON_PNG "${CMAKE_CURRENT_BINARY_DIR}/dan_icon_1024.png")
        execute_process(COMMAND "${_RSVG}" -w 1024 -h 1024
            "${CMAKE_CURRENT_SOURCE_DIR}/Resources/dan_icon.svg"
            -o "${_ICON_PNG}")
        set(_DAN_ICON_BIG_ARG ICON_BIG "${_ICON_PNG}")
        message(STATUS "Dan: icon → ${_ICON_PNG}")
    elseif(WIN32)
        # Windows needs a .ico; use ImageMagick if available
        find_program(_MAGICK NAMES magick convert)
        if(_MAGICK)
            set(_ICO "${CMAKE_CURRENT_BINARY_DIR}/Dan.ico")
            execute_process(
                COMMAND "${_MAGICK}" convert -background none
                    -define icon:auto-resize=256,128,64,48,32,16
                    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/dan_icon.svg"
                    "${_ICO}"
                RESULT_VARIABLE _MAGICK_RC)
            if(_MAGICK_RC EQUAL 0)
                set(_DAN_ICON_BIG_ARG ICON_BIG "${_ICO}")
                message(STATUS "Dan: icon → ${_ICO}")
            else()
                message(STATUS "Dan: magick failed (RC=${_MAGICK_RC}) — building without icon")
            endif()
        else()
            message(STATUS "Dan: magick not found — building without icon (install ImageMagick to enable)")
        endif()
    endif()
else()
    message(STATUS "Dan: rsvg-convert not found — building without app icon (brew install librsvg to enable)")
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
    ${_DAN_ICON_BIG_ARG}
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
    Source/MidiOutputComponent.cpp
    Source/Icons.h
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
        DanResources
        juce::juce_audio_utils
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

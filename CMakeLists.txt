cmake_minimum_required(VERSION 3.22)
project(MidiEnvelopePlugin VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Universal Binary — visible to Ableton on both native Apple Silicon and Rosetta
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
# Target macOS 10.13+ (JUCE 8 minimum; covers all Macs that run Ableton 11)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "" FORCE)

# ─── Dependencies via FetchContent ───────────────────────────────────────────
include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.9
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(
    clap_juce_extensions
    GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(JUCE clap_juce_extensions)

# ─── Plugin target ─────────────────────────────────────────────────────────
juce_add_plugin(MidiEnvelopePlugin
    COMPANY_NAME            "Umaru Industrial Waste"
    PLUGIN_MANUFACTURER_CODE UmIW
    PLUGIN_CODE             DnEv
    FORMATS                 VST3 AU AUv3 Standalone
    PRODUCT_NAME            "Dan"
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        TRUE
    NEEDS_MIDI_OUTPUT       TRUE
    IS_MIDI_EFFECT          TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VERSION                 0.1.0
    BUNDLE_ID               "com.umaruwaste.dan"
)

# ─── CLAP wrapper ─────────────────────────────────────────────────────────
clap_juce_extensions_plugin(
    TARGET              MidiEnvelopePlugin
    CLAP_ID             "com.umaruwaste.dan"
    CLAP_FEATURES       "midi-effect" "utility"
)

# ─── Sources ──────────────────────────────────────────────────────────────
target_sources(MidiEnvelopePlugin PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/EnvelopePlayer.cpp
    Source/EnvelopeEditorComponent.cpp
    Source/KeyMapperComponent.cpp
)

target_include_directories(MidiEnvelopePlugin PRIVATE Source)

# Generate JuceHeader.h (includes all linked JUCE modules)
juce_generate_juce_header(MidiEnvelopePlugin)

target_compile_definitions(MidiEnvelopePlugin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
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

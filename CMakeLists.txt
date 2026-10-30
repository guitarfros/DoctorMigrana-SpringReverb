# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (C) 2026 GuitarFros

cmake_minimum_required(VERSION 3.22)
project(DoctorMigranaReverb VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Copia o plugin ao cartafol de VST3 do sistema despois de compilar:
#   Linux ~/.vst3 | macOS ~/Library/Audio/Plug-Ins/VST3 | Windows C:/Program Files/Common Files/VST3
# En Windows esa copia precisa permisos de administrador: usa -DMIGRANA_COPY_AFTER_BUILD=OFF
option(MIGRANA_COPY_AFTER_BUILD "Copy plugin to the system plugin folder after build" ON)

include(FetchContent)
# Mesma versión de JUCE que o proxecto do tremolo (descárgase de GitHub na primeira compilación)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.11
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

# PLUGIN_NAME  = nome visible no host (co guión longo)
# PRODUCT_NAME = nome dos ficheiros (ASCII)
juce_add_plugin(DoctorMigranaReverb
    COMPANY_NAME "GuitarFros"
    PLUGIN_MANUFACTURER_CODE Gfrs
    PLUGIN_CODE Mrev
    BUNDLE_ID com.guitarfros.doctormigranareverb
    FORMATS VST3 Standalone
    VST3_CATEGORIES Fx Reverb
    PLUGIN_NAME "Doctor Migrana – Reverb"
    PRODUCT_NAME "Doctor Migrana - Reverb"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD ${MIGRANA_COPY_AFTER_BUILD})

juce_add_binary_data(DoctorMigranaReverbAssets SOURCES
    Assets/fondo.jpg
    Assets/knob_strip.png
    Assets/switch_on.png
    Assets/switch_off.png)

target_sources(DoctorMigranaReverb PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(DoctorMigranaReverb PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

# Necesario en MSVC para que o nome con caracteres non ASCII compile ben
target_compile_options(DoctorMigranaReverb PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

target_link_libraries(DoctorMigranaReverb
    PRIVATE DoctorMigranaReverbAssets
            juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)

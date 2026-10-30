// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 GuitarFros

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "SpringReverb.h"

namespace ids
{
    inline constexpr const char* mix     = "mix";
    inline constexpr const char* decay   = "decay";
    inline constexpr const char* tone    = "tone";
    inline constexpr const char* dwell   = "dwell";
    inline constexpr const char* master  = "master";   // volume de saída en dB
    inline constexpr const char* enabled = "enabled";
}

class DoctorMigranaProcessor : public juce::AudioProcessor
{
public:
    DoctorMigranaProcessor();
    ~DoctorMigranaProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

private:
    std::atomic<float>* pMix     = nullptr;
    std::atomic<float>* pDecay   = nullptr;
    std::atomic<float>* pTone    = nullptr;
    std::atomic<float>* pDwell   = nullptr;
    std::atomic<float>* pMaster  = nullptr;

    float masterGain() const;
    std::atomic<float>* pEnabled = nullptr;

    dm::SpringReverb reverb;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DoctorMigranaProcessor)
};

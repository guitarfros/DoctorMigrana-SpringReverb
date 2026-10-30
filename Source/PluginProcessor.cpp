// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 GuitarFros

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout DoctorMigranaProcessor::createLayout()
{
    using Range = juce::NormalisableRange<float>;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ids::mix,   1 }, "Mix",   Range (0.0f, 1.0f, 0.001f), 0.35f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ids::decay, 1 }, "Decay", Range (0.0f, 1.0f, 0.001f), 0.60f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ids::tone,  1 }, "Tone",  Range (0.0f, 1.0f, 0.001f), 0.50f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ids::dwell, 1 }, "Dwell", Range (0.0f, 1.0f, 0.001f), 0.50f));
    // Master: -40 dB .. +6 dB, con 0 dB no centro do knob; ao mínimo (-40) queda en silencio
    Range masterRange (-40.0f, 6.0f, 0.1f);
    masterRange.setSkewForCentre (0.0f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ids::master, 1 }, "Master", masterRange, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterBool>  (juce::ParameterID { ids::enabled, 1 }, "Active", true));
    return layout;
}

DoctorMigranaProcessor::DoctorMigranaProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    pMix     = apvts.getRawParameterValue (ids::mix);
    pDecay   = apvts.getRawParameterValue (ids::decay);
    pTone    = apvts.getRawParameterValue (ids::tone);
    pDwell   = apvts.getRawParameterValue (ids::dwell);
    pMaster  = apvts.getRawParameterValue (ids::master);
    pEnabled = apvts.getRawParameterValue (ids::enabled);
}

float DoctorMigranaProcessor::masterGain() const
{
    const float db = pMaster->load();
    return db <= -39.9f ? 0.0f : juce::Decibels::decibelsToGain (db);
}

bool DoctorMigranaProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    const bool inOk  = in  == juce::AudioChannelSet::mono() || in  == juce::AudioChannelSet::stereo();
    const bool outOk = out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
    return inOk && outOk;
}

void DoctorMigranaProcessor::prepareToPlay (double sampleRate, int)
{
    reverb.setParams (pDwell->load(), pDecay->load(), pTone->load(), pMix->load(), masterGain(), pEnabled->load() > 0.5f);
    reverb.prepare (sampleRate);        // prepare() reinicia os estados cos valores actuais
}

void DoctorMigranaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    const int n      = buffer.getNumSamples();

    for (int c = numIn; c < numOut; ++c)
        buffer.clear (c, 0, n);

    reverb.setParams (pDwell->load(), pDecay->load(), pTone->load(), pMix->load(), masterGain(), pEnabled->load() > 0.5f);

    float* ch0 = buffer.getWritePointer (0);
    float* ch1 = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    const bool stereoIn = numIn > 1 && ch1 != nullptr;

    for (int i = 0; i < n; ++i)
    {
        const float inL = ch0[i];
        const float inR = stereoIn ? ch1[i] : inL;
        float outL = 0.0f, outR = 0.0f;
        reverb.process (inL, inR, outL, outR);

        if (ch1 != nullptr && numOut > 1) { ch0[i] = outL; ch1[i] = outR; }
        else                              { ch0[i] = 0.5f * (outL + outR); }
    }
}

juce::AudioProcessorEditor* DoctorMigranaProcessor::createEditor()
{
    return new DoctorMigranaEditor (*this);
}

void DoctorMigranaProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void DoctorMigranaProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DoctorMigranaProcessor();
}

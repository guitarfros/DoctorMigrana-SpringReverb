// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 GuitarFros

#pragma once
#include <array>
#include <memory>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// Knob que debuxa un fotograma dunha filmstrip vertical (N fotogramas apilados)
class FilmstripLookAndFeel : public juce::LookAndFeel_V4
{
public:
    FilmstripLookAndFeel (juce::Image stripImage, int frames) : strip (std::move (stripImage)), numFrames (frames) {}

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float, float, juce::Slider&) override;
private:
    juce::Image strip;
    int numFrames;
};

// Interruptor (bypass) + LED: dúas imaxes, ON e OFF
class PedalSwitch : public juce::Component
{
public:
    PedalSwitch (juce::RangedAudioParameter& param, juce::Image onImage, juce::Image offImage);
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    juce::Image onImg, offImg;
    bool isOn = true;
    juce::ParameterAttachment attachment;
};

class DoctorMigranaEditor : public juce::AudioProcessorEditor
{
public:
    explicit DoctorMigranaEditor (DoctorMigranaProcessor&);
    ~DoctorMigranaEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    DoctorMigranaProcessor& audioProcessor;
    juce::Image background, scaledBackground;
    std::unique_ptr<FilmstripLookAndFeel> lnf;
    std::array<juce::Slider, 5> knobs;                       // MIX, DECAY, TONE, DWELL, MASTER
    std::array<std::unique_ptr<SliderAttachment>, 5> attachments;
    std::unique_ptr<PedalSwitch> bypassSwitch;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DoctorMigranaEditor)
};

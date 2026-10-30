// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 GuitarFros

#include "PluginEditor.h"
#include <BinaryData.h>

// Coordenadas sobre a imaxe de fondo orixinal (1440 x 1200 px)
namespace layout
{
    constexpr float W = 1440.0f, H = 1200.0f;
    constexpr float knobX[5] = { 327.0f, 579.0f, 841.0f, 1097.0f, 1080.0f };   // MIX, DECAY, TONE, DWELL, MASTER
    constexpr float knobY[5] = { 890.0f, 890.0f, 890.0f, 890.0f, 345.0f };
    constexpr float knobSize = 230.0f;
    constexpr float swX = 226.0f, swY = 171.0f, swW = 277.0f, swH = 116.0f;
    constexpr int   knobFrames = 24;
}

//==============================================================================
void FilmstripLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                             float pos, float, float, juce::Slider&)
{
    if (strip.isNull() || numFrames <= 0)
        return;

    const int frameH = strip.getHeight() / numFrames;
    const int idx = juce::jlimit (0, numFrames - 1, (int) std::lround (pos * (float) (numFrames - 1)));

    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (strip, x, y, width, height, 0, idx * frameH, strip.getWidth(), frameH);
}

//==============================================================================
PedalSwitch::PedalSwitch (juce::RangedAudioParameter& param, juce::Image onImage, juce::Image offImage)
    : onImg (std::move (onImage)), offImg (std::move (offImage)),
      attachment (param, [this] (float v) { isOn = v > 0.5f; repaint(); }, nullptr)
{
    attachment.sendInitialUpdate();
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void PedalSwitch::paint (juce::Graphics& g)
{
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (isOn ? onImg : offImg, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
}

void PedalSwitch::mouseDown (const juce::MouseEvent&)
{
    attachment.setValueAsCompleteGesture (isOn ? 0.0f : 1.0f);
}

//==============================================================================
DoctorMigranaEditor::DoctorMigranaEditor (DoctorMigranaProcessor& p)
    : juce::AudioProcessorEditor (&p), audioProcessor (p)
{
    background = juce::ImageCache::getFromMemory (BinaryData::fondo_jpg, BinaryData::fondo_jpgSize);
    auto strip = juce::ImageCache::getFromMemory (BinaryData::knob_strip_png, BinaryData::knob_strip_pngSize);
    lnf = std::make_unique<FilmstripLookAndFeel> (strip, layout::knobFrames);

    const char* paramIds[5] = { ids::mix, ids::decay, ids::tone, ids::dwell, ids::master };

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];
        k.setLookAndFeel (lnf.get());
        k.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        k.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                               juce::MathConstants<float>::pi * 2.75f, true);
        k.setMouseDragSensitivity (220);
        addAndMakeVisible (k);

        attachments[i] = std::make_unique<SliderAttachment> (audioProcessor.apvts, paramIds[i], k);

        // Texto da burbulla ao mover o knob (despois do attachment, que pon o seu propio)
        if (i == 4)   // Master en dB
            k.textFromValueFunction = [] (double v) { return v <= -39.9 ? juce::String ("-inf dB") : juce::String (v, 1) + " dB"; };
        else
            k.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + " %"; };
        k.setPopupDisplayEnabled (true, true, this);
    }

    auto onImg  = juce::ImageCache::getFromMemory (BinaryData::switch_on_png,  BinaryData::switch_on_pngSize);
    auto offImg = juce::ImageCache::getFromMemory (BinaryData::switch_off_png, BinaryData::switch_off_pngSize);
    bypassSwitch = std::make_unique<PedalSwitch> (*audioProcessor.apvts.getParameter (ids::enabled), onImg, offImg);
    addAndMakeVisible (*bypassSwitch);

    setResizable (true, true);
    setResizeLimits (360, 300, (int) layout::W, (int) layout::H);
    getConstrainer()->setFixedAspectRatio ((double) layout::W / (double) layout::H);
    setSize (720, 600);
}

DoctorMigranaEditor::~DoctorMigranaEditor()
{
    for (auto& k : knobs)
        k.setLookAndFeel (nullptr);
}

void DoctorMigranaEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff141414));
    if (scaledBackground.isValid())
        g.drawImageAt (scaledBackground, 0, 0);
}

void DoctorMigranaEditor::resized()
{
    const float s = (float) getWidth() / layout::W;

    if (background.isValid())
        scaledBackground = background.rescaled (getWidth(), getHeight(), juce::Graphics::highResamplingQuality);

    for (size_t i = 0; i < knobs.size(); ++i)
        knobs[i].setBounds (juce::Rectangle<float> (layout::knobSize * s, layout::knobSize * s)
                                .withCentre ({ layout::knobX[i] * s, layout::knobY[i] * s })
                                .toNearestInt());

    if (bypassSwitch != nullptr)
        bypassSwitch->setBounds (juce::Rectangle<float> (layout::swX * s, layout::swY * s,
                                                         layout::swW * s, layout::swH * s).toNearestInt());
}

#pragma once
#include "PluginProcessor.h"

class VUEditor : public juce::AudioProcessorEditor,
                 private juce::Timer
{
public:
    explicit VUEditor (VUMeterProcessor&);
    ~VUEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    juce::Image makeSteel (int w, int h) const;
    juce::Image makePaper (int w, int h) const;
    void drawFace (juce::Graphics& g, juce::Rectangle<float> face,
                   float angleDeg, const juce::String& channel, float lamp, int mode);
    void drawScrew (juce::Graphics& g, juce::Point<float> c) const;

    static float degFor (float vu);

    VUMeterProcessor& proc;

    juce::ComboBox modeBox;
    juce::Slider refKnob, trimKnob, lampKnob;

    juce::AudioProcessorValueTreeState::ComboBoxAttachment modeAtt;
    juce::AudioProcessorValueTreeState::SliderAttachment   refAtt, trimAtt, lampAtt;

    juce::Image steel, paper;
    juce::Rectangle<float> faceL, faceR;

    float angL = -45.0f, angR = -45.0f;
    float velL = 0.0f,   velR = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VUEditor)
};

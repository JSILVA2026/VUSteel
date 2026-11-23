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

    void drawMeter (juce::Graphics& g, juce::Point<float> pivot, float angleDeg,
                    const juce::String& channel, float lamp, int mode) const;

    static float vuToDeg (float vu);

    VUMeterProcessor& proc;
    juce::Image background;

    juce::ComboBox modeBox;
    juce::Slider refKnob, trimKnob, lampKnob;

    juce::AudioProcessorValueTreeState::ComboBoxAttachment modeAtt;
    juce::AudioProcessorValueTreeState::SliderAttachment   refAtt, trimAtt, lampAtt;

    float angL = -42.0f, angR = -42.0f;
    float velL = 0.0f,   velR = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VUEditor)
};

#pragma once
#include "PluginProcessor.h"

// Knob estilo console: aro com escala 0-10, capa de metal escuro e indicador claro
class NeveLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;
};

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

    void drawMeter (juce::Graphics& g, juce::Point<float> pivot, juce::Point<float> centre,
                    float angleDeg, const juce::String& channel, float lamp, int mode) const;

    static float vuToDeg (float vu);
    juce::AffineTransform artToLocal() const;

    VUMeterProcessor& proc;
    juce::Image background;

    NeveLookAndFeel neve;

    juce::ComboBox modeBox;
    juce::Slider refKnob, trimKnob, lampKnob;
    juce::Label modeLabel, refLabel, trimLabel, lampLabel;

    juce::AudioProcessorValueTreeState::ComboBoxAttachment modeAtt;
    juce::AudioProcessorValueTreeState::SliderAttachment   refAtt, trimAtt, lampAtt;

    float angL = -42.0f, angR = -42.0f;
    float velL = 0.0f,   velR = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VUEditor)
};

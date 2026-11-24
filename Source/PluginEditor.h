#pragma once
#include "PluginProcessor.h"

// Knob que gira a própria imagem do botão (PNG)
class ImageKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void setImage (juce::Image img) { image = std::move (img); }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;

private:
    juce::Image image;
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
    void drawNeedle (juce::Graphics& g, int meterIndex, float angleDeg) const;
    static float vuToDeg (float vu);
    juce::AffineTransform artToLocal() const;

    VUMeterProcessor& proc;

    juce::Image background;
    juce::Image medidorImg[2];
    juce::Image btnImg[3];
    juce::Image panelImg;

    ImageKnobLookAndFeel knobLaf[3];

    juce::ComboBox modeBox;
    juce::Slider refKnob, trimKnob, lampKnob;

    juce::AudioProcessorValueTreeState::ComboBoxAttachment modeAtt;
    juce::AudioProcessorValueTreeState::SliderAttachment   refAtt, trimAtt, lampAtt;

    float angL = -42.0f, angR = -42.0f;
    float velL = 0.0f,   velR = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VUEditor)
};

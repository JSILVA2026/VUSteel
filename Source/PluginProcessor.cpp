#include "PluginProcessor.h"
#include "PluginEditor.h"

VUMeterProcessor::VUMeterProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout VUMeterProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "mode", "Modo", juce::StringArray { "VU", "PPM", "Peak" }, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "ref", "Referencia (dBFS)",
        juce::NormalisableRange<float> (-30.0f, 0.0f, 0.1f), -18.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "trim", "Calibracao (dB)",
        juce::NormalisableRange<float> (-6.0f, 6.0f, 0.1f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "lamp", "Lampada", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.6f));

    return layout;
}

void VUMeterProcessor::prepareToPlay (double sampleRate, int)
{
    ballL.prepare (sampleRate);
    ballR.prepare (sampleRate);
}

void VUMeterProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int mode  = (int) apvts.getRawParameterValue ("mode")->load();
    const float ref  = apvts.getRawParameterValue ("ref")->load();
    const float trim = apvts.getRawParameterValue ("trim")->load();

    const int n = buffer.getNumSamples();
    const float* l = buffer.getReadPointer (0);
    const float* r = buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : l;

    for (int i = 0; i < n; ++i)
    {
        ballL.process (l[i]);
        ballR.process (r[i]);
    }

    auto toDisplay = [ref, trim] (double linear)
    {
        const double db = 20.0 * std::log10 (std::max (linear, 1.0e-7));
        return (float) (db - ref + trim);
    };

    levelL.store (toDisplay (ballL.value (mode)));
    levelR.store (toDisplay (ballR.value (mode)));

    // O áudio passa sem alteração: o plugin é apenas um medidor.
}

juce::AudioProcessorEditor* VUMeterProcessor::createEditor()
{
    return new VUEditor (*this);
}

void VUMeterProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void VUMeterProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// Fábrica exigida pelo JUCE para o host
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VUMeterProcessor();
}

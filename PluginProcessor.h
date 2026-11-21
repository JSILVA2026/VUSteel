#pragma once
#include <JuceHeader.h>

class VUMeterProcessor : public juce::AudioProcessor
{
public:
    VUMeterProcessor();
    ~VUMeterProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()
            && ! layouts.getMainInputChannelSet().isDisabled();
    }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VU Steel"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Valores em dB já com referência e trim aplicados, lidos pelo editor
    std::atomic<float> levelL { -100.0f }, levelR { -100.0f };

private:
    struct MeterBallistics
    {
        void prepare (double sampleRate)
        {
            rmsCoef = std::exp (-1.0 / (sampleRate * 0.010));          // RMS de 10 ms
            vuCoef  = std::exp (-1.0 / (sampleRate * 0.0652));         // 300 ms até 99 %
            ppmRelease  = std::pow (10.0, -1.0 / (1.7 * sampleRate));  // 20 dB em 1,7 s
            peakRelease = std::pow (10.0, -1.0 / (0.3 * sampleRate));  // 20 dB em 0,3 s
            reset();
        }

        void reset() { ms = vuEnv = ppmEnv = peakEnv = 0.0; }

        inline void process (float x)
        {
            const double a = std::abs (x);
            ms = rmsCoef * ms + (1.0 - rmsCoef) * (double) x * (double) x;
            vuEnv = vuCoef * vuEnv + (1.0 - vuCoef) * std::sqrt (ms);

            ppmEnv  = a > ppmEnv  ? a : ppmEnv  * ppmRelease;
            peakEnv = a > peakEnv ? a : peakEnv * peakRelease;
        }

        double value (int mode) const
        {
            switch (mode)
            {
                case 0:  return vuEnv;
                case 1:  return ppmEnv;
                default: return peakEnv;
            }
        }

        double ms = 0.0, vuEnv = 0.0, ppmEnv = 0.0, peakEnv = 0.0;
        double rmsCoef = 0.0, vuCoef = 0.0, ppmRelease = 0.0, peakRelease = 0.0;
    };

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    MeterBallistics ballL, ballR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VUMeterProcessor)
};

#pragma once
#include <JuceHeader.h>
#include "SideChainEngine.h"

class ChainMaidAudioProcessor : public juce::AudioProcessor,
                                private juce::AudioProcessorValueTreeState::Listener
{
public:
    ChainMaidAudioProcessor();
    ~ChainMaidAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "ChainMaid"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    struct IDs
    {
        static constexpr const char* threshold = "threshold";
        static constexpr const char* ratio     = "ratio";
        static constexpr const char* knee      = "knee";
        static constexpr const char* attack    = "attack";
        static constexpr const char* release   = "release";
        static constexpr const char* makeup    = "makeup";
        static constexpr const char* mix       = "mix";
        static constexpr const char* extSc     = "external_sc";
    };

    std::atomic<float> grDbMeter { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParams();
    void parameterChanged(const juce::String& id, float newValue) override;
    void refreshDspTargets();

    chainmaid::SideChainEngine engines[2];
    std::atomic<bool> dirty { true };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChainMaidAudioProcessor)
};

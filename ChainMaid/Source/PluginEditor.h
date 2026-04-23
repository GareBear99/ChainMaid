#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class ChainMaidAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit ChainMaidAudioProcessorEditor(ChainMaidAudioProcessor&);
    ~ChainMaidAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    ChainMaidAudioProcessor& processor;

    juce::Slider thresholdSlider, ratioSlider, kneeSlider, attackSlider,
                 releaseSlider, makeupSlider, mixSlider;
    juce::ToggleButton extScToggle { "External Sidechain" };

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SA> aThr, aRat, aKnee, aAtk, aRel, aMup, aMix;
    std::unique_ptr<BA> aExtSc;

    float smoothedGrDb = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChainMaidAudioProcessorEditor)
};

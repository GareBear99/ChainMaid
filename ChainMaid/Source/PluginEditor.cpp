#include "PluginEditor.h"

using namespace juce;

ChainMaidAudioProcessorEditor::ChainMaidAudioProcessorEditor(ChainMaidAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    auto setupKnob = [this](Slider& s, const String& /*name*/)
    {
        s.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(Slider::TextBoxBelow, false, 78, 18);
        addAndMakeVisible(s);
    };
    setupKnob(thresholdSlider, "Threshold");
    setupKnob(ratioSlider,     "Ratio");
    setupKnob(kneeSlider,      "Knee");
    setupKnob(attackSlider,    "Attack");
    setupKnob(releaseSlider,   "Release");
    setupKnob(makeupSlider,    "Makeup");
    setupKnob(mixSlider,       "Mix");

    extScToggle.setClickingTogglesState(true);
    addAndMakeVisible(extScToggle);

    aThr   = std::make_unique<SA>(processor.apvts, ChainMaidAudioProcessor::IDs::threshold, thresholdSlider);
    aRat   = std::make_unique<SA>(processor.apvts, ChainMaidAudioProcessor::IDs::ratio,     ratioSlider);
    aKnee  = std::make_unique<SA>(processor.apvts, ChainMaidAudioProcessor::IDs::knee,      kneeSlider);
    aAtk   = std::make_unique<SA>(processor.apvts, ChainMaidAudioProcessor::IDs::attack,    attackSlider);
    aRel   = std::make_unique<SA>(processor.apvts, ChainMaidAudioProcessor::IDs::release,   releaseSlider);
    aMup   = std::make_unique<SA>(processor.apvts, ChainMaidAudioProcessor::IDs::makeup,    makeupSlider);
    aMix   = std::make_unique<SA>(processor.apvts, ChainMaidAudioProcessor::IDs::mix,       mixSlider);
    aExtSc = std::make_unique<BA>(processor.apvts, ChainMaidAudioProcessor::IDs::extSc,     extScToggle);

    setSize(720, 360);
    startTimerHz(30);
}

void ChainMaidAudioProcessorEditor::timerCallback()
{
    const float target = processor.grDbMeter.load(std::memory_order_relaxed);
    smoothedGrDb += 0.25f * (target - smoothedGrDb);
    repaint();
}

void ChainMaidAudioProcessorEditor::paint(Graphics& g)
{
    g.fillAll(Colour::fromRGB(10, 14, 22));

    auto header = getLocalBounds().toFloat().removeFromTop(56.0f).reduced(16.0f, 6.0f);
    g.setColour(Colour::fromRGB(214, 181, 106));
    g.setFont(FontOptions(26.0f, Font::bold));
    g.drawText("ChainMaid", header.toNearestInt(), Justification::centredLeft);

    g.setColour(Colours::white.withAlpha(0.75f));
    g.setFont(FontOptions(12.0f));
    g.drawText("Feed-forward sidechain compressor \u2014 external or internal detector \u00b7 soft-knee \u00b7 attack/release \u00b7 mix",
               header.translated(0, 24).toNearestInt(), Justification::centredLeft);

    // GR meter
    auto meterArea = getLocalBounds().toFloat().removeFromBottom(28.0f).reduced(16.0f, 4.0f);
    g.setColour(Colour::fromRGB(24, 32, 48));
    g.fillRoundedRectangle(meterArea, 6.0f);
    const float grDb = jlimit(-24.0f, 0.0f, smoothedGrDb);
    const float w    = meterArea.getWidth() * (1.0f - grDb / -24.0f);
    g.setColour(Colour::fromRGB(255, 180,  90));
    g.fillRoundedRectangle(meterArea.withWidth(w), 6.0f);
    g.setColour(Colours::white.withAlpha(0.9f));
    g.setFont(FontOptions(11.0f, Font::bold));
    g.drawText(String("Gain reduction: ") + String(grDb, 1) + " dB",
               meterArea.toNearestInt(), Justification::centred);
}

void ChainMaidAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(60);
    area.removeFromBottom(36);
    auto topRow = area.removeFromTop(area.getHeight() - 36);
    const int knobW = topRow.getWidth() / 7;
    thresholdSlider.setBounds(topRow.removeFromLeft(knobW).reduced(6));
    ratioSlider    .setBounds(topRow.removeFromLeft(knobW).reduced(6));
    kneeSlider     .setBounds(topRow.removeFromLeft(knobW).reduced(6));
    attackSlider   .setBounds(topRow.removeFromLeft(knobW).reduced(6));
    releaseSlider  .setBounds(topRow.removeFromLeft(knobW).reduced(6));
    makeupSlider   .setBounds(topRow.removeFromLeft(knobW).reduced(6));
    mixSlider      .setBounds(topRow.reduced(6));
    extScToggle.setBounds(area.reduced(4, 0));
}

#include "PluginProcessor.h"
#include "PluginEditor.h"

ChainMaidAudioProcessor::ChainMaidAudioProcessor()
  : AudioProcessor(BusesProperties()
        .withInput ("Main",       juce::AudioChannelSet::stereo(), true)
        .withOutput("Main",       juce::AudioChannelSet::stereo(), true)
        .withInput ("Sidechain",  juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "PARAMS", createParams())
{
    for (auto* id : { IDs::threshold, IDs::ratio, IDs::knee, IDs::attack,
                      IDs::release,   IDs::makeup, IDs::mix,  IDs::extSc })
        apvts.addParameterListener(id, this);
}

ChainMaidAudioProcessor::~ChainMaidAudioProcessor()
{
    for (auto* id : { IDs::threshold, IDs::ratio, IDs::knee, IDs::attack,
                      IDs::release,   IDs::makeup, IDs::mix,  IDs::extSc })
        apvts.removeParameterListener(id, this);
}

juce::AudioProcessorValueTreeState::ParameterLayout ChainMaidAudioProcessor::createParams()
{
    using P = juce::AudioParameterFloat;
    using B = juce::AudioParameterBool;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<P>(IDs::threshold, "Threshold", juce::NormalisableRange<float>(-60.f, 0.f, 0.01f),   -18.f));
    layout.add(std::make_unique<P>(IDs::ratio,     "Ratio",     juce::NormalisableRange<float>(  1.f, 20.f, 0.01f, 0.5f), 4.0f));
    layout.add(std::make_unique<P>(IDs::knee,      "Knee",      juce::NormalisableRange<float>(  0.f, 24.f, 0.01f),   6.0f));
    layout.add(std::make_unique<P>(IDs::attack,    "Attack",    juce::NormalisableRange<float>(  0.1f, 500.f, 0.01f, 0.3f), 5.0f));
    layout.add(std::make_unique<P>(IDs::release,   "Release",   juce::NormalisableRange<float>(  1.f, 2000.f, 0.01f, 0.3f), 120.f));
    layout.add(std::make_unique<P>(IDs::makeup,    "Makeup",    juce::NormalisableRange<float>(-24.f, 24.f, 0.01f),  0.0f));
    layout.add(std::make_unique<P>(IDs::mix,       "Mix",       juce::NormalisableRange<float>(  0.f, 100.f, 0.01f), 100.f));
    layout.add(std::make_unique<B>(IDs::extSc,     "External SC", true));
    return layout;
}

bool ChainMaidAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto main = layouts.getMainInputChannelSet();
    const auto out  = layouts.getMainOutputChannelSet();
    if (main != out) return false;
    if (main != juce::AudioChannelSet::mono()
        && main != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void ChainMaidAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    for (auto& e : engines) e.prepare(sampleRate);
    dirty.store(true, std::memory_order_release);
}

void ChainMaidAudioProcessor::parameterChanged(const juce::String&, float)
{
    dirty.store(true, std::memory_order_release);
}

void ChainMaidAudioProcessor::refreshDspTargets()
{
    if (!dirty.exchange(false, std::memory_order_acq_rel)) return;
    const float thr = apvts.getRawParameterValue(IDs::threshold)->load();
    const float rat = apvts.getRawParameterValue(IDs::ratio)    ->load();
    const float knw = apvts.getRawParameterValue(IDs::knee)     ->load();
    const float atk = apvts.getRawParameterValue(IDs::attack)   ->load();
    const float rel = apvts.getRawParameterValue(IDs::release)  ->load();
    const float mup = apvts.getRawParameterValue(IDs::makeup)   ->load();
    for (auto& e : engines)
    {
        e.setThresholdDb(thr);
        e.setRatio(rat);
        e.setKneeDb(knw);
        e.setAttackMs(atk);
        e.setReleaseMs(rel);
        e.setMakeupDb(mup);
    }
}

void ChainMaidAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    refreshDspTargets();

    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0) return;

    const auto mainBus = getBusBuffer(buffer, true, 0);
    auto       outBus  = getBusBuffer(buffer, false, 0);
    juce::AudioBuffer<float> scBus;
    const bool hasExtSc = getTotalNumInputChannels() > mainBus.getNumChannels()
                          && apvts.getRawParameterValue(IDs::extSc)->load() > 0.5f;
    if (hasExtSc)
        scBus = getBusBuffer(buffer, true, 1);

    const float mix01 = juce::jlimit(0.0f, 1.0f,
                          apvts.getRawParameterValue(IDs::mix)->load() / 100.0f);

    float grAcc = 0.0f;
    const int ch = juce::jmin(2, mainBus.getNumChannels());
    for (int c = 0; c < ch; ++c)
    {
        const float* in = mainBus.getReadPointer(c);
        float*       out = outBus.getWritePointer(c);
        const float* sc  = (hasExtSc && c < scBus.getNumChannels())
                             ? scBus.getReadPointer(c) : in;
        auto& e = engines[c];
        for (int i = 0; i < numSamples; ++i)
        {
            const float wet = e.processSample(in[i], sc[i]);
            out[i] = wet * mix01 + in[i] * (1.0f - mix01);
        }
        grAcc += e.getLastGrDb();
    }
    grDbMeter.store(ch > 0 ? grAcc / (float) ch : 0.0f, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* ChainMaidAudioProcessor::createEditor()
{
    return new ChainMaidAudioProcessorEditor(*this);
}

void ChainMaidAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (xml) copyXmlToBinary(*xml, destData);
}

void ChainMaidAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    dirty.store(true, std::memory_order_release);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChainMaidAudioProcessor();
}

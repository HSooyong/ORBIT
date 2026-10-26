#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
// Parameter IDs (new product: version hint 1, no v0.3 compatibility)
const char* const kIds[] = { "rate", "sync", "phase", "shape", "stages", "ecc", "radius", "bh_x", "bh_y",
                             "gravity", "horizon", "floor", "warp", "flutter", "chaos", "voices", "width",
                             "feedback", "tone", "drive", "kepler", "mix", "output", "hold", "bloom" };
enum Idx { RATE, SYNC, PHASE, SHAPE, STAGES, ECC, RADIUS, BHX, BHY, GRAV, HOR, FLOOR, WARP, FLUT, CHAOS,
           VOICES, WIDTH, FB, TONE, DRIVE, KEPLER, MIX, OUT, HOLD, BLOOM, NUM };
static_assert (NUM == 25, "25 params");

using FA = juce::AudioParameterFloatAttributes;
std::unique_ptr<juce::AudioParameterFloat> pf (int i, const char* name, float lo, float hi, float def, const char* label = "", float skew = 1.0f)
{
    return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kIds[i], 1 }, name,
        juce::NormalisableRange<float> (lo, hi, 0.0f, skew), def,
        FA().withLabel (label).withStringFromValueFunction ([lo, hi] (float v, int) {
            const float span = hi - lo; return juce::String (v, span > 20.0f ? 1 : (span > 2.5f ? 2 : 3)); }));
}
std::unique_ptr<juce::AudioParameterFloat> pct (int i, const char* name, float def)
{
    return pf (i, name, 0.0f, 100.0f, def, "%");
}
}

OrbitAudioProcessor::OrbitAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ORBIT_GW", createParameterLayout())
{
    for (auto* id : kIds) raw.push_back (apvts.getRawParameterValue (id));
}

juce::AudioProcessorValueTreeState::ParameterLayout OrbitAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> v;
    v.push_back (pf (RATE, "Rate", 0.02f, 10.0f, 0.35f, "Hz", 0.4f));
    v.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { kIds[SYNC], 1 }, "Sync",
        juce::StringArray { "Free", "4/1", "2/1", "1/1", "1/2D", "1/2", "1/4D", "1/4", "1/8", "1/8T", "1/16" }, 0));
    v.push_back (pf (PHASE, "Phase", 0.0f, 1.0f, 0.0f, "cyc"));
    v.push_back (pf (SHAPE, "Shape", 0.0f, 1.0f, 0.2f));
    v.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { kIds[STAGES], 1 }, "Stages",
        juce::StringArray { "Smooth", "2", "3", "4", "5", "6", "8", "12", "16" }, 0));
    v.push_back (pf (ECC, "Eccentricity", 0.0f, 0.9f, 0.3f));
    v.push_back (pf (RADIUS, "Orbit Size", 0.2f, 1.0f, 0.7f));
    v.push_back (pf (BHX, "Black Hole X", -1.0f, 1.0f, 0.85f));
    v.push_back (pf (BHY, "Black Hole Y", -1.0f, 1.0f, 0.0f));
    v.push_back (pct (GRAV, "Gravity", 50.0f));
    v.push_back (pf (HOR, "Horizon", 0.05f, 0.5f, 0.2f));
    v.push_back (pct (FLOOR, "Space Chorus", 15.0f));
    v.push_back (pct (WARP, "Wow", 60.0f));
    v.push_back (pct (FLUT, "Flutter", 30.0f));
    v.push_back (pct (CHAOS, "Chaos", 25.0f));
    v.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { kIds[VOICES], 1 }, "Voices",
        juce::StringArray { "1", "2", "3", "4" }, 1));
    v.push_back (pct (WIDTH, "Width", 70.0f));
    v.push_back (pf (FB, "Feedback", 0.0f, 70.0f, 0.0f, "%"));
    v.push_back (pf (TONE, "Tone", -1.0f, 1.0f, 0.0f));
    v.push_back (pct (DRIVE, "Tape", 0.0f));
    v.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { kIds[KEPLER], 1 }, "Kepler", true));
    v.push_back (pct (MIX, "Mix", 35.0f));
    v.push_back (pf (OUT, "Output", -12.0f, 12.0f, 0.0f, "dB"));
    v.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { kIds[HOLD], 1 }, "Hold", false));
    v.push_back (pct (BLOOM, "Horizon Bloom", 0.0f));
    return { v.begin(), v.end() };
}

orbit::Params OrbitAudioProcessor::readParams() const
{
    auto f = [this] (int i) { return raw[(size_t) i]->load(); };
    orbit::Params p;
    p.rate = f (RATE); p.sync = (int) f (SYNC); p.phase = f (PHASE); p.shape = f (SHAPE); p.stages = (int) f (STAGES);
    p.ecc = f (ECC); p.radius = f (RADIUS); p.bhX = f (BHX); p.bhY = f (BHY); p.gravity = f (GRAV) * 0.01f;
    p.horizon = f (HOR); p.floor = f (FLOOR) * 0.01f; p.warp = f (WARP) * 0.01f; p.flutter = f (FLUT) * 0.01f;
    p.chaos = f (CHAOS) * 0.01f; p.voices = (int) f (VOICES) + 1; p.width = f (WIDTH) * 0.01f;
    p.feedback = f (FB) * 0.01f; p.tone = f (TONE); p.drive = f (DRIVE) * 0.01f; p.kepler = f (KEPLER) > 0.5f;
    p.mix = f (MIX) * 0.01f; p.outputDb = f (OUT); p.hold = f (HOLD) > 0.5f; p.bloom = f (BLOOM) * 0.01f;
    return p;
}

void OrbitAudioProcessor::setCurrentProgram (int index)
{
    currentProgram = juce::jlimit (0, 6, index);
    const auto p = orbit::factoryPresets()[(size_t) currentProgram].p;
    auto set = [this] (int i, float plainValue)
    {
        if (auto* prm = apvts.getParameter (kIds[i]))
            prm->setValueNotifyingHost (prm->convertTo0to1 (plainValue));
    };
    set (RATE, p.rate); set (SYNC, (float) p.sync); set (PHASE, p.phase); set (SHAPE, p.shape); set (STAGES, (float) p.stages);
    set (ECC, p.ecc); set (RADIUS, p.radius); set (BHX, p.bhX); set (BHY, p.bhY); set (GRAV, p.gravity * 100);
    set (HOR, p.horizon); set (FLOOR, p.floor * 100); set (WARP, p.warp * 100); set (FLUT, p.flutter * 100);
    set (CHAOS, p.chaos * 100); set (VOICES, (float) (p.voices - 1)); set (WIDTH, p.width * 100); set (FB, p.feedback * 100);
    set (TONE, p.tone); set (DRIVE, p.drive * 100); set (KEPLER, p.kepler ? 1.0f : 0.0f); set (MIX, p.mix * 100);
    set (OUT, p.outputDb); set (HOLD, p.hold ? 1.0f : 0.0f); set (BLOOM, p.bloom * 100);
}

const juce::String OrbitAudioProcessor::getProgramName (int index)
{
    return orbit::factoryPresets()[(size_t) juce::jlimit (0, 6, index)].name;
}

void OrbitAudioProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    setLatencySamples (0);
}

bool OrbitAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    const bool inOk = in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
    const bool outOk = out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
    return inOk && outOk && in.size() <= out.size(); // mono->mono, mono->stereo, stereo->stereo
}

void OrbitAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int nIn = getTotalNumInputChannels(), nOut = getTotalNumOutputChannels();
    if (n == 0 || nOut == 0) return;
    for (int c = nIn; c < nOut; ++c) buffer.clear (c, 0, n);

    orbit::HostInfo host;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto bpm = pos->getBpm()) { host.hasBpm = true; host.bpm = *bpm; }
            if (auto ppq = pos->getPpqPosition()) { host.hasPpq = true; host.ppq = *ppq; }
            host.playing = pos->getIsPlaying();
        }

    const float* inL = buffer.getReadPointer (0);
    const float* inR = nIn > 1 ? buffer.getReadPointer (1) : nullptr;
    float* outL = buffer.getWritePointer (0);
    float* outR = nOut > 1 ? buffer.getWritePointer (1) : nullptr;
    engine.process (inL, inR, outL, outR, n, readParams(), host);

    const auto& t = engine.telemetry();
    visPx = t.px; visPy = t.py; visBhX = t.bhX; visBhY = t.bhY; visD = t.d; visG = t.g; visCents = t.cents; visBloom = t.bloom; visTheta = t.theta; visWowHz = t.wowHz;
}

juce::AudioProcessorEditor* OrbitAudioProcessor::createEditor()
{
    return new OrbitEditor (*this);
}

void OrbitAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void OrbitAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new OrbitAudioProcessor(); }

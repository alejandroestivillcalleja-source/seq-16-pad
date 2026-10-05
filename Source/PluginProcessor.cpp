#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // Orden de los parametros dentro de PadParams::p
    const char* const padParamIds[] = { sq::id::on, sq::id::source, sq::id::wave, sq::id::note, sq::id::fine,
                                        sq::id::pEnv, sq::id::pDecay, sq::id::start, sq::id::sPitch,
                                        sq::id::attack, sq::id::decay, sq::id::sustain, sq::id::release,
                                        sq::id::gate, sq::id::cutoff, sq::id::reso, sq::id::level, sq::id::pan };
    enum PadParamIndex { kOn, kSource, kWave, kNote, kFine, kPEnv, kPDecay, kStart, kSPitch,
                         kAttack, kDecay, kSustain, kRelease, kGate, kCutoff, kReso, kLevel, kPan, kNumPadParams };
    static_assert (kNumPadParams == 18);

    // Texto que se muestra debajo de cada mando (con pocos decimales)
    juce::AudioParameterFloatAttributes attrs (const juce::String& unit, int decimals)
    {
        return juce::AudioParameterFloatAttributes()
            .withLabel (unit)
            .withStringFromValueFunction ([unit, decimals] (float v, int)
            {
                return juce::String (v, decimals) + (unit.isNotEmpty() ? " " + unit : juce::String());
            });
    }

    juce::NormalisableRange<float> msRange (float lo, float hi, float centre)
    {
        juce::NormalisableRange<float> r (lo, hi);
        r.setSkewForCentre (centre);
        return r;
    }

    // Sonidos de ejemplo para que al darle a PLAY ya suene algo:
    // pasos 1,5,9,13 = bombo, pasos 3,7,11,15 = charles, el resto = notas (apagadas).
    struct PadDefaults { bool on; int wave; int note; float pEnv, pDecay, attack, decay, sustain, release, cutoff, level; };

    PadDefaults defaultsFor (int pad)
    {
        static const int melody[] = { 0, 60, 0, 63, 0, 67, 0, 70, 0, 72, 0, 70, 0, 67, 0, 65 };

        if (pad % 4 == 0) return { true,  sq::sine,  36, 24.0f, 40.0f, 0.5f, 350.0f, 0.0f, 80.0f, 20000.0f, -3.0f };
        if (pad % 4 == 2) return { true,  sq::noise, 60,  0.0f, 50.0f, 0.5f,  60.0f, 0.0f, 40.0f,  9000.0f, -14.0f };
        return                   { false, sq::saw,   melody[pad], 0.0f, 50.0f, 2.0f, 250.0f, 0.3f, 150.0f, 2500.0f, -12.0f };
    }
}

SQ16Processor::SQ16Processor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "SQ16", createLayout())
{
    formatManager.registerBasicFormats();

    for (int pad = 0; pad < sq::numPads; ++pad)
        for (int k = 0; k < kNumPadParams; ++k)
            padParams[(size_t) pad].p[k] = apvts.getRawParameterValue (sq::pid (pad, padParamIds[k]));

    bpmParam    = apvts.getRawParameterValue (sq::bpmId);
    masterParam = apvts.getRawParameterValue (sq::masterId);
    swingParam  = apvts.getRawParameterValue (sq::swingId);

    startTimer (1000);
}

SQ16Processor::~SQ16Processor()
{
    stopTimer();
}

juce::AudioProcessorValueTreeState::ParameterLayout SQ16Processor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { sq::bpmId, 1 }, "BPM",
                                                       NormalisableRange<float> (40.0f, 240.0f, 0.1f), 100.0f, attrs ({}, 1)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { sq::masterId, 1 }, "Master",
                                                       NormalisableRange<float> (-48.0f, 6.0f, 0.1f), 0.0f,
                                                       attrs ("dB", 1)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { sq::swingId, 1 }, "Swing",
                                                       NormalisableRange<float> (0.0f, 75.0f, 0.1f), 0.0f,
                                                       attrs ("%", 0)));

    for (int pad = 0; pad < sq::numPads; ++pad)
    {
        const auto d = defaultsFor (pad);
        auto group = std::make_unique<AudioProcessorParameterGroup> ("pad" + String (pad + 1), "Pad " + String (pad + 1), " | ");
        const String n = "Pad " + String (pad + 1) + " ";
        auto id = [pad] (const char* name) { return ParameterID { sq::pid (pad, name), 1 }; };
        auto ms = attrs ("ms", 1);

        group->addChild (std::make_unique<AudioParameterBool>   (id (sq::id::on), n + "Activo", d.on));
        group->addChild (std::make_unique<AudioParameterChoice> (id (sq::id::source), n + "Fuente", sq::sourceNames, 0));
        group->addChild (std::make_unique<AudioParameterChoice> (id (sq::id::wave), n + "Onda", sq::waveNames, d.wave));
        group->addChild (std::make_unique<AudioParameterInt>    (id (sq::id::note), n + "Nota", 24, 96, d.note));
        group->addChild (std::make_unique<AudioParameterFloat>  (id (sq::id::fine), n + "Afinacion",
                                                                  NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f,
                                                                  attrs ("ct", 0)));
        group->addChild (std::make_unique<AudioParameterFloat>  (id (sq::id::pEnv), n + "Env Tono",
                                                                  NormalisableRange<float> (0.0f, 48.0f, 0.1f), d.pEnv,
                                                                  attrs ("st", 1)));
        group->addChild (std::make_unique<AudioParameterFloat>  (id (sq::id::pDecay), n + "Caida Tono", msRange (2.0f, 1000.0f, 80.0f), d.pDecay, ms));
        group->addChild (std::make_unique<AudioParameterFloat>  (id (sq::id::start), n + "Inicio Sample",
                                                                  NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f,
                                                                  attrs ("%", 0)));
        group->addChild (std::make_unique<AudioParameterFloat>  (id (sq::id::sPitch), n + "Tono Sample",
                                                                  NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f,
                                                                  attrs ("st", 1)));
        group->addChild (std::make_unique<AudioParameterFloat>  (id (sq::id::attack),  n + "Ataque",  msRange (0.1f, 2000.0f, 50.0f), d.attack, ms));
        group->addChild (std::make_unique<AudioParameterFloat>  (id (sq::id::decay),   n + "Decay",   msRange (5.0f, 4000.0f, 300.0f), d.decay, ms));
        group->addChild (std::make_unique<AudioParameterFloat>  (id (sq::id::sustain), n + "Sustain",
                                                                  NormalisableRange<float> (0.0f, 1.0f, 0.001f), d.sustain, attrs ({}, 2)));
        group->addChild (std::make_unique<AudioParameterFloat>  (id (sq::id::release), n + "Release", msRange (5.0f, 4000.0f, 300.0f), d.release, ms));
        group->addChild (std::make_unique<AudioParameterFloat>  (id (sq::id::gate), n + "Gate",
                                                                  NormalisableRange<float> (5.0f, 100.0f, 0.1f), 50.0f,
                                                                  attrs ("%", 0)));
        group->addChild (std::make_unique<AudioParameterFloat>  (id (sq::id::cutoff), n + "Corte", msRange (20.0f, 20000.0f, 1000.0f), d.cutoff,
                                                                  attrs ("Hz", 0)));
        group->addChild (std::make_unique<AudioParameterFloat>  (id (sq::id::reso), n + "Resonancia",
                                                                  NormalisableRange<float> (0.3f, 8.0f, 0.01f, 0.5f), 0.707f, attrs ({}, 2)));
        group->addChild (std::make_unique<AudioParameterFloat>  (id (sq::id::level), n + "Nivel",
                                                                  NormalisableRange<float> (-48.0f, 6.0f, 0.1f), d.level,
                                                                  attrs ("dB", 1)));
        group->addChild (std::make_unique<AudioParameterFloat>  (id (sq::id::pan), n + "Pan",
                                                                  NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f, attrs ({}, 2)));
        layout.add (std::move (group));
    }

    return layout;
}

bool SQ16Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void SQ16Processor::prepareToPlay (double newSampleRate, int)
{
    sampleRate = newSampleRate;
    for (auto& v : voices)
        v.prepare (sampleRate);
}

void SQ16Processor::readPadSettings (int pad, sq::PadSettings& s) const
{
    const auto& p = padParams[(size_t) pad].p;
    s.source  = (int) p[kSource]->load();
    s.wave    = (int) p[kWave]->load();
    s.note    = p[kNote]->load();
    s.fine    = p[kFine]->load();
    s.pEnv    = p[kPEnv]->load();
    s.pDecay  = p[kPDecay]->load();
    s.start   = p[kStart]->load() * 0.01f;
    s.sPitch  = p[kSPitch]->load();
    s.attack  = p[kAttack]->load();
    s.decay   = p[kDecay]->load();
    s.sustain = p[kSustain]->load();
    s.release = p[kRelease]->load();
    s.cutoff  = p[kCutoff]->load();
    s.reso    = p[kReso]->load();
    s.gain    = juce::Decibels::decibelsToGain (p[kLevel]->load(), -47.9f);
    s.pan     = p[kPan]->load();
}

void SQ16Processor::triggerPad (int pad, float velocity, double samplesPerStep)
{
    std::shared_ptr<sq::SampleData> sample;
    {
        const juce::SpinLock::ScopedTryLockType lock (sampleLock);
        if (lock.isLocked())
            sample = samples[(size_t) pad];
    }

    const float gate = padParams[(size_t) pad].p[kGate]->load() * 0.01f;
    voices[(size_t) pad].trigger (std::move (sample), (int) (gate * samplesPerStep), velocity);
    ++triggerCount[(size_t) pad];
}

void SQ16Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    for (int pad = 0; pad < sq::numPads; ++pad)
    {
        readPadSettings (pad, settings[(size_t) pad]);
        voices[(size_t) pad].update (settings[(size_t) pad]);
    }

    // --- Reloj: el del DAW si esta reproduciendo, si no el interno (boton PLAY) ---
    double bpm = bpmParam->load();
    double hostPpq = 0.0;
    bool syncToHost = false;

    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (pos->getIsPlaying() && pos->getPpqPosition().hasValue())
            {
                syncToHost = true;
                hostPpq = *pos->getPpqPosition();
                if (pos->getBpm().hasValue())
                    bpm = *pos->getBpm();
            }
        }
    }

    hostPlaying = syncToHost;
    const bool playing = syncToHost || internalPlaying;
    const double samplesPerStep = sampleRate * 60.0 / bpm / 4.0;   // semicorcheas
    const double stepsPerSample = 1.0 / samplesPerStep;
    const double swingOffset = swingParam->load() * 0.01 * 0.5;
    double stepPos = syncToHost ? hostPpq * 4.0 : internalStepPos;

    auto stepTime = [swingOffset] (juce::int64 idx) { return (double) idx + ((idx & 1) != 0 ? swingOffset : 0.0); };

    if (playing && ! wasPlaying)
    {
        if (! syncToHost)
            stepPos = internalStepPos = 0.0;
        lastStepIndex = (juce::int64) std::floor (stepPos) - 1;
    }
    if (! playing)
        currentStep = -1;
    wasPlaying = playing;

    // Pads pulsados con el raton en la interfaz
    for (int pad = 0; pad < sq::numPads; ++pad)
        if (previewRequest[(size_t) pad].exchange (false))
            triggerPad (pad, 1.0f, samplesPerStep);

    const float master = juce::Decibels::decibelsToGain (masterParam->load(), -47.9f);
    auto* outL = buffer.getWritePointer (0);
    auto* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    auto midiIt = midi.cbegin();

    for (int n = 0; n < numSamples; ++n)
    {
        // MIDI: notas 36..51 (C1..D#2) disparan los pads 1..16, como en una MPC
        for (; midiIt != midi.cend() && (*midiIt).samplePosition <= n; ++midiIt)
        {
            const auto msg = (*midiIt).getMessage();
            if (msg.isNoteOn())
            {
                const int pad = msg.getNoteNumber() - 36;
                if (pad >= 0 && pad < sq::numPads)
                    triggerPad (pad, msg.getFloatVelocity(), samplesPerStep);
            }
        }

        if (playing)
        {
            if (stepPos < stepTime (lastStepIndex) - 0.01 || stepPos > stepTime (lastStepIndex + 1) + 1.0)
                lastStepIndex = (juce::int64) std::floor (stepPos) - 1;   // el DAW ha saltado (loop, etc.)

            if (stepPos >= stepTime (lastStepIndex + 1))
            {
                ++lastStepIndex;
                const int step = (int) (((lastStepIndex % sq::numPads) + sq::numPads) % sq::numPads);
                currentStep = step;
                if (padParams[(size_t) step].p[kOn]->load() > 0.5f)
                    triggerPad (step, 1.0f, samplesPerStep);
            }

            stepPos += stepsPerSample;
        }

        float l = 0.0f, r = 0.0f;
        for (auto& v : voices)
            v.renderSample (l, r);

        if (outR != nullptr) { outL[n] = l * master; outR[n] = r * master; }
        else                 { outL[n] = (l + r) * 0.5f * master; }
    }

    if (playing && ! syncToHost)
        internalStepPos = stepPos;

    midi.clear();
}

bool SQ16Processor::loadSample (int pad, const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr)
        return false;

    auto data = std::make_shared<sq::SampleData>();
    const auto maxLen = (juce::int64) (reader->sampleRate * 30.0);   // maximo 30 segundos
    const int len = (int) juce::jmin (reader->lengthInSamples, maxLen);
    const int chans = (int) juce::jlimit (1u, 2u, reader->numChannels);

    data->buffer.setSize (chans, len);
    reader->read (&data->buffer, 0, len, 0, true, chans > 1);
    data->sampleRate = reader->sampleRate;
    data->file = file;

    {
        const juce::SpinLock::ScopedLockType lock (sampleLock);
        std::swap (samples[(size_t) pad], data);
    }
    if (data != nullptr)
        retiredSamples.push_back (std::move (data));

    if (auto* src = apvts.getParameter (sq::pid (pad, sq::id::source)))
    {
        src->beginChangeGesture();
        src->setValueNotifyingHost (src->convertTo0to1 ((float) sq::sample));
        src->endChangeGesture();
    }
    return true;
}

void SQ16Processor::clearSample (int pad)
{
    std::shared_ptr<sq::SampleData> old;
    {
        const juce::SpinLock::ScopedLockType lock (sampleLock);
        std::swap (samples[(size_t) pad], old);
    }
    if (old != nullptr)
        retiredSamples.push_back (std::move (old));
}

juce::String SQ16Processor::getSampleName (int pad) const
{
    const juce::SpinLock::ScopedLockType lock (sampleLock);
    auto& s = samples[(size_t) pad];
    return s != nullptr ? s->file.getFileName() : juce::String();
}

void SQ16Processor::timerCallback()
{
    // Libera (fuera del hilo de audio) los samples que ya nadie esta usando
    retiredSamples.erase (std::remove_if (retiredSamples.begin(), retiredSamples.end(),
                                          [] (const auto& s) { return s.use_count() == 1; }),
                          retiredSamples.end());
}

void SQ16Processor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    juce::ValueTree sampleTree ("SAMPLES");
    {
        const juce::SpinLock::ScopedLockType lock (sampleLock);
        for (int pad = 0; pad < sq::numPads; ++pad)
            if (samples[(size_t) pad] != nullptr)
                sampleTree.appendChild (juce::ValueTree ("SAMPLE", { { "pad", pad },
                                                                     { "path", samples[(size_t) pad]->file.getFullPathName() } }),
                                        nullptr);
    }
    state.appendChild (sampleTree, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void SQ16Processor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr)
        return;

    auto state = juce::ValueTree::fromXml (*xml);
    if (! state.hasType (apvts.state.getType()))
        return;

    auto sampleTree = state.getChildWithName ("SAMPLES");
    state.removeChild (sampleTree, nullptr);
    apvts.replaceState (state);

    for (int pad = 0; pad < sq::numPads; ++pad)
        clearSample (pad);

    for (const auto& s : sampleTree)
    {
        const int pad = s["pad"];
        const juce::File file (s["path"].toString());
        if (pad >= 0 && pad < sq::numPads && file.existsAsFile())
        {
            // loadSample cambia la fuente a "Sample"; la restauramos a lo que estaba guardado
            const auto savedSource = apvts.getParameter (sq::pid (pad, sq::id::source))->getValue();
            loadSample (pad, file);
            apvts.getParameter (sq::pid (pad, sq::id::source))->setValueNotifyingHost (savedSource);
        }
    }
}

juce::AudioProcessorEditor* SQ16Processor::createEditor()
{
    return new SQ16Editor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SQ16Processor();
}

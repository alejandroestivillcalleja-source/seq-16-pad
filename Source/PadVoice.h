#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "Params.h"

namespace sq
{
    // Audio cargado en un pad
    struct SampleData
    {
        juce::AudioBuffer<float> buffer;
        double sampleRate = 44100.0;
        juce::File file;
    };

    // Copia de los ajustes de un pad, leida una vez por bloque de audio
    struct PadSettings
    {
        int   source  = oscillator;
        int   wave    = sine;
        float note    = 60.0f;
        float fine    = 0.0f;
        float pEnv    = 0.0f;
        float pDecay  = 50.0f;
        float start   = 0.0f;
        float sPitch  = 0.0f;
        float attack  = 1.0f, decay = 200.0f, sustain = 0.0f, release = 100.0f;
        float cutoff  = 20000.0f, reso = 0.707f;
        float gain    = 1.0f, pan = 0.0f;
    };

    // Una voz por pad: suena el oscilador o el sample, pasa por un filtro y una envolvente ADSR.
    class PadVoice
    {
    public:
        void prepare (double newSampleRate)
        {
            sampleRate = newSampleRate;
            adsr.setSampleRate (sampleRate);
            filter.prepare ({ sampleRate, 1, 1 });
            filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
            filter.reset();
        }

        void update (const PadSettings& s)
        {
            settings = s;
            adsr.setParameters ({ s.attack * 0.001f, s.decay * 0.001f, s.sustain, s.release * 0.001f });
            filter.setCutoffFrequency (juce::jlimit (20.0f, (float) (sampleRate * 0.45), s.cutoff));
            filter.setResonance (s.reso);

            const float angle = (s.pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
            gainL = s.gain * std::cos (angle);
            gainR = s.gain * std::sin (angle);
        }

        void trigger (std::shared_ptr<SampleData> sampleToPlay, int gateSamples, float velocity)
        {
            sample = std::move (sampleToPlay);
            phase = 0.0;
            pitchEnv = 1.0f;
            pitchEnvCoef = std::exp (-1.0f / (float) (juce::jmax (1.0f, settings.pDecay) * 0.001 * sampleRate));
            gateRemaining = juce::jmax (1, gateSamples);
            vel = velocity;

            if (sample != nullptr && sample->buffer.getNumSamples() > 0)
                samplePos = settings.start * (double) (sample->buffer.getNumSamples() - 1);

            filter.reset();
            adsr.reset();
            adsr.noteOn();
        }

        bool isActive() const { return adsr.isActive(); }

        void renderSample (float& outL, float& outR)
        {
            if (! adsr.isActive())
                return;

            float s = settings.source == oscillator ? nextOscSample() : nextSampleSample();

            s = filter.processSample (0, s);
            s *= adsr.getNextSample() * vel;

            outL += s * gainL;
            outR += s * gainR;

            if (gateRemaining > 0 && --gateRemaining == 0)
                adsr.noteOff();
        }

    private:
        static float polyBlep (double t, double dt)
        {
            if (t < dt)        { t /= dt;               return (float) (t + t - t * t - 1.0); }
            if (t > 1.0 - dt)  { t = (t - 1.0) / dt;    return (float) (t * t + t + t + 1.0); }
            return 0.0f;
        }

        float nextOscSample()
        {
            const float semis = settings.note - 69.0f + settings.fine * 0.01f + settings.pEnv * pitchEnv;
            const double freq = 440.0 * std::pow (2.0, semis / 12.0);
            const double dt = juce::jmin (0.49, freq / sampleRate);
            pitchEnv *= pitchEnvCoef;

            float out = 0.0f;
            switch (settings.wave)
            {
                case sine:     out = (float) std::sin (phase * juce::MathConstants<double>::twoPi); break;
                case saw:      out = (float) (2.0 * phase - 1.0) - polyBlep (phase, dt); break;
                case square:
                {
                    out = phase < 0.5 ? 1.0f : -1.0f;
                    out += polyBlep (phase, dt);
                    out -= polyBlep (std::fmod (phase + 0.5, 1.0), dt);
                    out *= 0.8f;
                    break;
                }
                case triangle: out = (float) (1.0 - 4.0 * std::abs (phase - 0.5)); break;
                case noise:    out = random.nextFloat() * 2.0f - 1.0f; break;
                default: break;
            }

            phase += dt;
            if (phase >= 1.0)
                phase -= 1.0;

            return out;
        }

        float nextSampleSample()
        {
            if (sample == nullptr)
                return 0.0f;

            const auto& buf = sample->buffer;
            const int len = buf.getNumSamples();
            if (len < 2 || samplePos >= len - 1 || samplePos < 0.0)
                return 0.0f;

            const int i = (int) samplePos;
            const float frac = (float) (samplePos - i);
            float out = 0.0f;
            for (int ch = 0; ch < buf.getNumChannels(); ++ch)
            {
                const float* d = buf.getReadPointer (ch);
                out += d[i] + frac * (d[i + 1] - d[i]);
            }
            out /= (float) buf.getNumChannels();

            samplePos += (sample->sampleRate / sampleRate) * std::pow (2.0, settings.sPitch / 12.0);
            return out;
        }

        double sampleRate = 44100.0;
        PadSettings settings;
        juce::ADSR adsr;
        juce::dsp::StateVariableTPTFilter<float> filter;
        juce::Random random;

        std::shared_ptr<SampleData> sample;
        double samplePos = 0.0;
        double phase = 0.0;
        float pitchEnv = 0.0f, pitchEnvCoef = 0.0f;
        float gainL = 0.7f, gainR = 0.7f, vel = 1.0f;
        int gateRemaining = 0;
    };
}

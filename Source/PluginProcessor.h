#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "PadVoice.h"

// El "motor" del plugin: secuenciador de 16 pasos + 16 voces (una por pad).
class SQ16Processor : public juce::AudioProcessor,
                      private juce::Timer
{
public:
    SQ16Processor();
    ~SQ16Processor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SQ-16"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- Usado por la interfaz ---
    juce::AudioProcessorValueTreeState apvts;

    void setPlaying (bool shouldPlay)       { internalPlaying = shouldPlay; }
    bool isPlaying() const                  { return internalPlaying || hostPlaying; }
    bool isSyncedToHost() const             { return hostPlaying; }
    int  getCurrentStep() const             { return currentStep; }
    int  getTriggerCount (int pad) const    { return triggerCount[(size_t) pad]; }

    void previewPad (int pad)               { previewRequest[(size_t) pad] = true; }
    bool loadSample (int pad, const juce::File& file);
    void clearSample (int pad);
    juce::String getSampleName (int pad) const;

    juce::AudioFormatManager formatManager;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void readPadSettings (int pad, sq::PadSettings& s) const;
    void triggerPad (int pad, float velocity, double samplesPerStep);
    void timerCallback() override;

    struct PadParams
    {
        std::atomic<float>* p[18] {};
    };
    std::array<PadParams, sq::numPads> padParams;
    std::atomic<float>* bpmParam = nullptr;
    std::atomic<float>* masterParam = nullptr;
    std::atomic<float>* swingParam = nullptr;

    std::array<sq::PadVoice, sq::numPads> voices;
    std::array<sq::PadSettings, sq::numPads> settings;

    // Samples: la interfaz los cambia, el audio los lee
    std::array<std::shared_ptr<sq::SampleData>, sq::numPads> samples;
    std::vector<std::shared_ptr<sq::SampleData>> retiredSamples;
    juce::SpinLock sampleLock;

    // Reloj del secuenciador
    double sampleRate = 44100.0;
    double internalStepPos = 0.0;
    juce::int64 lastStepIndex = -1;
    bool wasPlaying = false;

    std::atomic<bool> internalPlaying { false };
    std::atomic<bool> hostPlaying { false };
    std::atomic<int> currentStep { -1 };
    std::array<std::atomic<int>, sq::numPads> triggerCount {};
    std::array<std::atomic<bool>, sq::numPads> previewRequest {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SQ16Processor)
};

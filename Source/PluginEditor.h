#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace sq::colours
{
    const juce::Colour body      { 0xff2a2a2d };
    const juce::Colour bodyDark  { 0xff1b1b1e };
    const juce::Colour panel     { 0xff222225 };
    const juce::Colour lcdBg     { 0xff9fb58c };
    const juce::Colour lcdText   { 0xff1c2a18 };
    const juce::Colour accent    { 0xffff8a1e };
    const juce::Colour hit       { 0xffff3b2f };
    const juce::Colour text      { 0xffd8d6d0 };
    const juce::Colour textDim   { 0xff8c8a85 };
    const juce::Colour padTop    { 0xff6e6d70 };
    const juce::Colour padBottom { 0xff545356 };
}

// Aspecto visual (mandos giratorios, botones) estilo MPC oscura
class SQLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SQLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
};

// Un mando con su nombre encima
class Knob : public juce::Component
{
public:
    explicit Knob (const juce::String& name);
    void resized() override;

    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

class SQ16Editor;

// Uno de los 16 pads/pasos
class PadComponent : public juce::Component,
                     public juce::FileDragAndDropTarget
{
public:
    PadComponent (SQ16Editor& owner, int index);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragOver = true;  repaint(); }
    void fileDragExit (const juce::StringArray&) override            { dragOver = false; repaint(); }

    float flash = 0.0f;
    int lastTriggerCount = 0;

private:
    juce::Rectangle<float> ledArea() const;

    SQ16Editor& editor;
    const int index;
    bool dragOver = false;
};

class SQ16Editor : public juce::AudioProcessorEditor,
                   private juce::Timer
{
public:
    explicit SQ16Editor (SQ16Processor&);
    ~SQ16Editor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

    void selectPad (int pad);
    void togglePadOn (int pad);
    int  getSelectedPad() const { return selectedPad; }
    bool isPadOn (int pad) const;
    void loadSampleIntoPad (int pad, const juce::File& file);

    SQ16Processor& processor;

private:
    void timerCallback() override;
    void setSource (int source);
    void chooseSample();
    void updateSourceVisibility();
    juce::String lcdLine() const;

    SQLookAndFeel lnf;
    int selectedPad = 0;

    // Panel superior (ajustes del pad seleccionado)
    juce::TextButton oscTab { "OSCILADOR" }, sampleTab { "SAMPLE" };
    juce::ToggleButton stepOnButton { "Paso activo" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> stepOnAttachment;

    juce::ComboBox waveBox;
    juce::Label waveLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveAttachment;
    Knob noteKnob { "Nota" }, fineKnob { "Afinacion" }, pEnvKnob { "Env Tono" }, pDecayKnob { "Caida Tono" };

    juce::TextButton loadButton { "Cargar sample..." }, clearButton { "Quitar" };
    juce::Label sampleNameLabel;
    Knob startKnob { "Inicio" }, sPitchKnob { "Tono" };

    Knob attackKnob { "Ataque" }, decayKnob { "Decay" }, sustainKnob { "Sustain" }, releaseKnob { "Release" }, gateKnob { "Gate" };
    Knob cutoffKnob { "Corte" }, resoKnob { "Resonancia" }, levelKnob { "Nivel" }, panKnob { "Pan" };

    // Transporte
    juce::TextButton playButton { "PLAY" }, stopButton { "STOP" };
    Knob bpmKnob { "BPM" }, swingKnob { "Swing" }, masterKnob { "Master" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bpmAttachment, swingAttachment, masterAttachment;

    juce::OwnedArray<PadComponent> pads;
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Zonas de dibujo
    juce::Rectangle<int> panelArea, lcdArea, oscArea, envArea, transportArea, transportLcd, padArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SQ16Editor)
};

#include "PluginEditor.h"

using namespace sq;

//==============================================================================
// Aspecto visual

SQLookAndFeel::SQLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, colours::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::black);
    setColour (juce::Slider::textBoxBackgroundColourId, colours::bodyDark);
    setColour (juce::Label::textColourId, colours::text);
    setColour (juce::TextButton::textColourOffId, colours::text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::black);
    setColour (juce::TextButton::buttonOnColourId, colours::accent);
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff3a3a3e));
    setColour (juce::ToggleButton::textColourId, colours::lcdText);
    setColour (juce::ToggleButton::tickColourId, colours::lcdText);
    setColour (juce::ToggleButton::tickDisabledColourId, colours::lcdText.withAlpha (0.6f));
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff3a3a3e));
    setColour (juce::ComboBox::outlineColourId, juce::Colours::black);
    setColour (juce::ComboBox::textColourId, colours::text);
    setColour (juce::ComboBox::arrowColourId, colours::accent);
    setColour (juce::PopupMenu::backgroundColourId, colours::bodyDark);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::accent);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
}

void SQLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                      float startAngle, float endAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (3.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float arcRadius = radius - 2.0f;

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, angle, true);
    g.setColour (colours::accent);
    g.strokePath (value, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float knobR = radius - 7.0f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4a4f), centre.x, centre.y - knobR,
                                             juce::Colour (0xff1f1f22), centre.x, centre.y + knobR, false));
    g.fillEllipse (centre.x - knobR, centre.y - knobR, knobR * 2.0f, knobR * 2.0f);
    g.setColour (juce::Colours::black);
    g.drawEllipse (centre.x - knobR, centre.y - knobR, knobR * 2.0f, knobR * 2.0f, 1.0f);

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -knobR + 2.0f, 3.0f, knobR * 0.55f, 1.5f);
    g.setColour (colours::text);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));
}

void SQLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& bg,
                                          bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    auto base = b.getToggleState() ? colours::accent : bg;
    if (highlighted) base = base.brighter (0.12f);
    if (down)        base = base.darker (0.2f);

    g.setGradientFill (juce::ColourGradient (base.brighter (0.08f), 0, r.getY(), base.darker (0.15f), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
}

juce::Font SQLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return juce::FontOptions (juce::jmin (15.0f, (float) buttonHeight * 0.5f), juce::Font::bold);
}

//==============================================================================
// Mando

Knob::Knob (const juce::String& name)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::black);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, colours::bodyDark);
    slider.setColour (juce::Slider::textBoxTextColourId, colours::text);
    addAndMakeVisible (slider);

    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, colours::textDim);
    addAndMakeVisible (label);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (15));
    slider.setBounds (r);
}

//==============================================================================
// Pad

PadComponent::PadComponent (SQ16Editor& owner, int i) : editor (owner), index (i) {}

juce::Rectangle<float> PadComponent::ledArea() const
{
    return { (float) getWidth() - 20.0f, 9.0f, 10.0f, 10.0f };
}

void PadComponent::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    const bool selected = editor.getSelectedPad() == index;
    const bool playhead = editor.processor.getCurrentStep() == index;
    const bool on = editor.isPadOn (index);

    // sombra
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 7.0f);

    auto top = colours::padTop.interpolatedWith (colours::hit, flash * 0.85f);
    auto bottom = colours::padBottom.interpolatedWith (colours::hit.darker (0.3f), flash * 0.85f);
    g.setGradientFill (juce::ColourGradient (top, 0, r.getY(), bottom, 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 7.0f);

    // textura de goma
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawRoundedRectangle (r.reduced (5.0f), 5.0f, 1.0f);

    if (selected)
    {
        g.setColour (colours::accent);
        g.drawRoundedRectangle (r.reduced (1.0f), 7.0f, 3.0f);
    }
    else
    {
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (r, 7.0f, 1.0f);
    }

    // barra del paso que esta sonando
    if (playhead)
    {
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.fillRoundedRectangle (r.getX() + 10.0f, r.getBottom() - 9.0f, r.getWidth() - 20.0f, 4.0f, 2.0f);
    }

    // LED de paso activo
    auto led = ledArea();
    if (on)
    {
        g.setColour (colours::hit.withAlpha (0.35f));
        g.fillEllipse (led.expanded (4.0f));
        g.setColour (colours::hit);
    }
    else
    {
        g.setColour (juce::Colour (0xff2a1a1a));
    }
    g.fillEllipse (led);
    g.setColour (juce::Colours::black);
    g.drawEllipse (led, 1.0f);

    // textos
    const auto& p = editor.processor.apvts;
    const bool isSample = p.getRawParameterValue (pid (index, id::source))->load() > 0.5f;
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (isSample ? "SMP" : "OSC", juce::Rectangle<float> (r.getX() + 9.0f, r.getY() + 7.0f, 40.0f, 14.0f),
                juce::Justification::centredLeft);

    g.setColour (colours::text.withAlpha (0.85f));
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText ("PAD " + juce::String (index + 1), juce::Rectangle<float> (r.getX() + 9.0f, r.getBottom() - 30.0f, 70.0f, 16.0f),
                juce::Justification::centredLeft);

    if (dragOver)
    {
        g.setColour (colours::accent.withAlpha (0.35f));
        g.fillRoundedRectangle (r, 7.0f);
    }
}

void PadComponent::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu() || ledArea().expanded (8.0f).contains (e.position))
    {
        editor.togglePadOn (index);
        return;
    }

    editor.selectPad (index);
    editor.processor.previewPad (index);
}

bool PadComponent::isInterestedInFileDrag (const juce::StringArray& files)
{
    const auto wildcard = editor.processor.formatManager.getWildcardForAllFormats();
    for (const auto& f : files)
        if (juce::File (f).hasFileExtension (wildcard.removeCharacters ("*")))
            return true;
    return false;
}

void PadComponent::filesDropped (const juce::StringArray& files, int, int)
{
    dragOver = false;
    if (! files.isEmpty())
        editor.loadSampleIntoPad (index, juce::File (files[0]));
}

//==============================================================================
// Ventana principal

SQ16Editor::SQ16Editor (SQ16Processor& p)
    : AudioProcessorEditor (p), processor (p)
{
    setLookAndFeel (&lnf);
    setWantsKeyboardFocus (true);

    // Pestañas Oscilador / Sample
    for (auto* tab : { &oscTab, &sampleTab })
    {
        tab->setClickingTogglesState (false);
        addAndMakeVisible (tab);
    }
    oscTab.onClick    = [this] { setSource (oscillator); };
    sampleTab.onClick = [this] { setSource (sample); };

    addAndMakeVisible (stepOnButton);

    // Oscilador
    waveBox.addItemList (waveNames, 1);
    addAndMakeVisible (waveBox);
    waveLabel.setText ("Onda", juce::dontSendNotification);
    waveLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    waveLabel.setColour (juce::Label::textColourId, colours::textDim);
    addAndMakeVisible (waveLabel);

    // Sample
    loadButton.onClick = [this] { chooseSample(); };
    clearButton.onClick = [this] { processor.clearSample (selectedPad); };
    sampleNameLabel.setFont (juce::FontOptions (13.0f));
    sampleNameLabel.setColour (juce::Label::backgroundColourId, colours::bodyDark);
    sampleNameLabel.setColour (juce::Label::outlineColourId, juce::Colours::black);
    addAndMakeVisible (loadButton);
    addAndMakeVisible (clearButton);
    addAndMakeVisible (sampleNameLabel);

    for (auto* k : { &noteKnob, &fineKnob, &pEnvKnob, &pDecayKnob, &startKnob, &sPitchKnob,
                     &attackKnob, &decayKnob, &sustainKnob, &releaseKnob, &gateKnob,
                     &cutoffKnob, &resoKnob, &levelKnob, &panKnob, &bpmKnob, &swingKnob, &masterKnob })
        addAndMakeVisible (k);

    // Transporte
    playButton.onClick = [this] { processor.setPlaying (true); };
    stopButton.onClick = [this] { processor.setPlaying (false); };
    addAndMakeVisible (playButton);
    addAndMakeVisible (stopButton);
    bpmAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, bpmId, bpmKnob.slider);
    swingAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, swingId, swingKnob.slider);
    masterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, masterId, masterKnob.slider);

    for (int i = 0; i < numPads; ++i)
        addAndMakeVisible (pads.add (new PadComponent (*this, i)));

    selectPad (0);
    setSize (940, 794);
    startTimerHz (30);
}

SQ16Editor::~SQ16Editor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void SQ16Editor::selectPad (int pad)
{
    selectedPad = pad;
    auto& apvts = processor.apvts;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;

    auto attach = [&] (Knob& k, const char* name)
    {
        k.attachment.reset();
        k.attachment = std::make_unique<SA> (apvts, pid (pad, name), k.slider);
    };

    attach (noteKnob, id::note);
    attach (fineKnob, id::fine);
    attach (pEnvKnob, id::pEnv);
    attach (pDecayKnob, id::pDecay);
    attach (startKnob, id::start);
    attach (sPitchKnob, id::sPitch);
    attach (attackKnob, id::attack);
    attach (decayKnob, id::decay);
    attach (sustainKnob, id::sustain);
    attach (releaseKnob, id::release);
    attach (gateKnob, id::gate);
    attach (cutoffKnob, id::cutoff);
    attach (resoKnob, id::reso);
    attach (levelKnob, id::level);
    attach (panKnob, id::pan);

    noteKnob.slider.textFromValueFunction = [] (double v) { return juce::MidiMessage::getMidiNoteName ((int) v, true, true, 3); };
    noteKnob.slider.updateText();

    waveAttachment.reset();
    waveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, pid (pad, id::wave), waveBox);
    stepOnAttachment.reset();
    stepOnAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, pid (pad, id::on), stepOnButton);

    updateSourceVisibility();
    repaint();
}

bool SQ16Editor::isPadOn (int pad) const
{
    return processor.apvts.getRawParameterValue (pid (pad, id::on))->load() > 0.5f;
}

void SQ16Editor::togglePadOn (int pad)
{
    if (auto* param = processor.apvts.getParameter (pid (pad, id::on)))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (isPadOn (pad) ? 0.0f : 1.0f);
        param->endChangeGesture();
    }
}

void SQ16Editor::setSource (int source)
{
    if (auto* param = processor.apvts.getParameter (pid (selectedPad, id::source)))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 ((float) source));
        param->endChangeGesture();
    }
    updateSourceVisibility();
}

void SQ16Editor::updateSourceVisibility()
{
    const bool isSample = processor.apvts.getRawParameterValue (pid (selectedPad, id::source))->load() > 0.5f;
    oscTab.setToggleState (! isSample, juce::dontSendNotification);
    sampleTab.setToggleState (isSample, juce::dontSendNotification);

    for (juce::Component* c : { (juce::Component*) &waveBox, (juce::Component*) &waveLabel,
                                (juce::Component*) &noteKnob, (juce::Component*) &fineKnob,
                                (juce::Component*) &pEnvKnob, (juce::Component*) &pDecayKnob })
        c->setVisible (! isSample);

    for (juce::Component* c : { (juce::Component*) &loadButton, (juce::Component*) &clearButton,
                                (juce::Component*) &sampleNameLabel, (juce::Component*) &startKnob,
                                (juce::Component*) &sPitchKnob })
        c->setVisible (isSample);
}

void SQ16Editor::chooseSample()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Elige un sample para el pad " + juce::String (selectedPad + 1),
                                                       juce::File(), processor.formatManager.getWildcardForAllFormats());
    const int pad = selectedPad;
    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this, pad] (const juce::FileChooser& fc)
                              {
                                  const auto file = fc.getResult();
                                  if (file.existsAsFile())
                                      loadSampleIntoPad (pad, file);
                              });
}

void SQ16Editor::loadSampleIntoPad (int pad, const juce::File& file)
{
    if (processor.loadSample (pad, file))
    {
        selectPad (pad);
        processor.previewPad (pad);
    }
}

bool SQ16Editor::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::spaceKey)
    {
        processor.setPlaying (! processor.isPlaying());
        return true;
    }
    return false;
}

void SQ16Editor::timerCallback()
{
    for (int i = 0; i < numPads; ++i)
    {
        auto* pad = pads[i];
        const int count = processor.getTriggerCount (i);
        if (count != pad->lastTriggerCount)
        {
            pad->lastTriggerCount = count;
            pad->flash = 1.0f;
        }
        else
        {
            pad->flash = juce::jmax (0.0f, pad->flash - 0.12f);
        }
        pad->repaint();
    }

    updateSourceVisibility();
    const auto name = processor.getSampleName (selectedPad);
    sampleNameLabel.setText (name.isEmpty() ? "(sin sample - arrastra un audio a un pad)" : name, juce::dontSendNotification);
    playButton.setToggleState (processor.isPlaying(), juce::dontSendNotification);

    repaint (lcdArea);
    repaint (transportLcd);
}

juce::String SQ16Editor::lcdLine() const
{
    const auto& apvts = processor.apvts;
    const bool isSample = apvts.getRawParameterValue (pid (selectedPad, id::source))->load() > 0.5f;
    juce::String s = "PAD " + juce::String (selectedPad + 1).paddedLeft ('0', 2) + "   ";

    if (isSample)
    {
        const auto name = processor.getSampleName (selectedPad);
        s << "SAMPLE   " << (name.isEmpty() ? juce::String ("---") : name);
    }
    else
    {
        const int wave = (int) apvts.getRawParameterValue (pid (selectedPad, id::wave))->load();
        const int note = (int) apvts.getRawParameterValue (pid (selectedPad, id::note))->load();
        s << "OSCILADOR   " << waveNames[wave].toUpperCase() << "   " << juce::MidiMessage::getMidiNoteName (note, true, true, 3);
    }
    return s;
}

void SQ16Editor::paint (juce::Graphics& g)
{
    g.fillAll (colours::body);

    // Cabecera
    g.setColour (colours::text);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("SQ-16", 16, 8, 100, 30, juce::Justification::centredLeft);
    g.setColour (colours::accent);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("STEP SEQUENCER  /  SAMPLER", 100, 8, 300, 30, juce::Justification::centredLeft);
    g.setColour (colours::textDim);
    g.drawText ("MIDI 36-51 = PAD 1-16", getWidth() - 220, 8, 204, 30, juce::Justification::centredRight);

    // Panel superior
    g.setColour (colours::panel);
    g.fillRoundedRectangle (panelArea.toFloat(), 8.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (panelArea.toFloat(), 8.0f, 1.0f);

    // Pantalla LCD
    g.setColour (colours::lcdBg);
    g.fillRoundedRectangle (lcdArea.toFloat(), 4.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (lcdArea.toFloat(), 4.0f, 2.0f);
    g.setColour (colours::lcdText);
    g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 16.0f, juce::Font::bold));
    g.drawText (lcdLine(), lcdArea.reduced (12, 0).withTrimmedRight (140), juce::Justification::centredLeft);

    // Separadores y titulos de seccion
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawVerticalLine (envArea.getX() - 10, (float) envArea.getY(), (float) envArea.getBottom());
    g.setColour (colours::accent);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText ("ENVOLVENTE", envArea.getX(), envArea.getY(), 200, 14, juce::Justification::centredLeft);
    g.drawText ("FILTRO  /  SALIDA", envArea.getX(), envArea.getY() + 112, 200, 14, juce::Justification::centredLeft);

    // Transporte
    g.setColour (colours::panel);
    g.fillRoundedRectangle (transportArea.toFloat(), 8.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (transportArea.toFloat(), 8.0f, 1.0f);

    g.setColour (colours::lcdBg);
    g.fillRoundedRectangle (transportLcd.toFloat(), 4.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (transportLcd.toFloat(), 4.0f, 2.0f);

    auto lcd = transportLcd.reduced (14, 8);
    const int step = processor.getCurrentStep();
    g.setColour (colours::lcdText);
    g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 30.0f, juce::Font::bold));
    g.drawText ("PASO " + (step < 0 ? juce::String ("--") : juce::String (step + 1).paddedLeft ('0', 2)) + "/16",
                lcd.removeFromTop (40), juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::bold));
    juce::String status = processor.isPlaying() ? "> REPRODUCIENDO" : "[] PARADO";
    status << "     RELOJ: " << (processor.isSyncedToHost() ? "DAW" : "INTERNO");
    g.drawText (status, lcd, juce::Justification::centredLeft);

    // Ayuda
    auto help = transportArea.reduced (16).withTop (transportArea.getBottom() - 104);
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (12.5f));
    g.drawMultiLineText ("Clic en un pad: seleccionarlo y escucharlo.\n"
                         "Clic en el LED rojo (o clic derecho): activar/desactivar el paso.\n"
                         "Arrastra un archivo de audio sobre un pad para cargarlo.\n"
                         "Barra espaciadora: PLAY / STOP.",
                         help.getX(), help.getY() + 14, help.getWidth());
}

void SQ16Editor::resized()
{
    // Panel superior
    panelArea = { 12, 50, getWidth() - 24, 306 };
    auto panel = panelArea.reduced (12);
    lcdArea = panel.removeFromTop (36);
    stepOnButton.setBounds (lcdArea.getRight() - 136, lcdArea.getY() + 6, 126, 24);
    panel.removeFromTop (10);

    oscArea = panel.removeFromLeft (380);
    panel.removeFromLeft (20);
    envArea = panel;

    auto src = oscArea;
    auto tabs = src.removeFromTop (30);
    oscTab.setBounds (tabs.removeFromLeft (130));
    tabs.removeFromLeft (6);
    sampleTab.setBounds (tabs.removeFromLeft (130));
    src.removeFromTop (12);

    // contenido Oscilador
    auto waveRow = src.removeFromTop (26);
    waveLabel.setBounds (waveRow.removeFromLeft (50));
    waveBox.setBounds (waveRow.removeFromLeft (170));
    src.removeFromTop (10);
    auto oscKnobs = src.removeFromTop (100);
    for (auto* k : { &noteKnob, &fineKnob, &pEnvKnob, &pDecayKnob })
        k->setBounds (oscKnobs.removeFromLeft (95));

    // contenido Sample (misma zona, se muestra uno u otro)
    auto smp = oscArea.withTrimmedTop (42);
    auto smpRow = smp.removeFromTop (26);
    loadButton.setBounds (smpRow.removeFromLeft (150));
    smpRow.removeFromLeft (6);
    clearButton.setBounds (smpRow.removeFromLeft (80));
    smp.removeFromTop (6);
    sampleNameLabel.setBounds (smp.removeFromTop (24).withWidth (370));
    smp.removeFromTop (4);
    auto smpKnobs = smp.removeFromTop (100);
    startKnob.setBounds (smpKnobs.removeFromLeft (95));
    sPitchKnob.setBounds (smpKnobs.removeFromLeft (95));

    // envolvente / filtro
    auto env = envArea;
    auto row1 = env.removeFromTop (110).withTrimmedTop (14);
    for (auto* k : { &attackKnob, &decayKnob, &sustainKnob, &releaseKnob, &gateKnob })
        k->setBounds (row1.removeFromLeft (94));
    auto row2 = env.removeFromTop (110).withTrimmedTop (14);
    for (auto* k : { &cutoffKnob, &resoKnob, &levelKnob, &panKnob })
        k->setBounds (row2.removeFromLeft (94));

    // Zona inferior: pads a la derecha, transporte a la izquierda (como una MPC)
    const int padSize = 96, gap = 10;
    const int gridSize = padSize * 4 + gap * 3;
    const int bottomY = panelArea.getBottom() + 14;
    padArea = { getWidth() - 12 - gridSize, bottomY, gridSize, gridSize };
    for (int i = 0; i < numPads; ++i)
        pads[i]->setBounds (padArea.getX() + (i % 4) * (padSize + gap),
                            padArea.getY() + (i / 4) * (padSize + gap), padSize, padSize);

    transportArea = { 12, bottomY, padArea.getX() - 12 - 16, gridSize };
    auto t = transportArea.reduced (12);
    transportLcd = t.removeFromTop (86);
    t.removeFromTop (12);
    auto buttons = t.removeFromTop (44);
    playButton.setBounds (buttons.removeFromLeft (150));
    buttons.removeFromLeft (10);
    stopButton.setBounds (buttons.removeFromLeft (150));
    t.removeFromTop (10);
    auto knobs = t.removeFromTop (100);
    for (auto* k : { &bpmKnob, &swingKnob, &masterKnob })
        k->setBounds (knobs.removeFromLeft (100));
}

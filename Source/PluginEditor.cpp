#include "PluginEditor.h"

OrbitEditor::Knob* OrbitEditor::knob (const char* id, const char* label)
{
    auto k = std::make_unique<Knob>();
    k->s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 84, 15);
    k->s.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
    k->s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff0b0d10));
    k->s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff23292f));
    k->s.setColour (juce::Slider::textBoxTextColourId, OrbitLookAndFeel::text());
    k->s.setTextValueSuffix ("");
    k->l.setText (label, juce::dontSendNotification);
    k->l.setJustificationType (juce::Justification::centred);
    k->a = std::make_unique<SA> (proc.apvts, id, k->s); // attachment wraps drags in begin/endChangeGesture
    addAndMakeVisible (k->s); addAndMakeVisible (k->l);
    knobs.push_back (std::move (k));
    return knobs.back().get();
}

OrbitEditor::Combo* OrbitEditor::combo (const char* id, const char* label)
{
    auto c = std::make_unique<Combo>();
    if (auto* prm = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id)))
        c->c.addItemList (prm->choices, 1);
    c->l.setText (label, juce::dontSendNotification);
    c->l.setJustificationType (juce::Justification::centredLeft);
    c->a = std::make_unique<CA> (proc.apvts, id, c->c);
    addAndMakeVisible (c->c); addAndMakeVisible (c->l);
    combos.push_back (std::move (c));
    return combos.back().get();
}

OrbitEditor::OrbitEditor (OrbitAudioProcessor& p) : AudioProcessorEditor (p), proc (p), field (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (field);

    left  = { knob ("rate", "RATE"), knob ("phase", "PHASE"), knob ("shape", "SHAPE"), knob ("ecc", "ECCENTRIC"),
              knob ("radius", "ORBIT SIZE"), knob ("gravity", "GRAVITY") };
    right = { knob ("warp", "WOW"), knob ("flutter", "FLUTTER"), knob ("chaos", "CHAOS"), knob ("width", "WIDTH"),
              knob ("drive", "TAPE"), knob ("tone", "TONE"), knob ("mix", "MIX"), knob ("output", "OUTPUT") };
    adv   = { knob ("horizon", "HORIZON"), knob ("bloom", "H. BLOOM"), knob ("floor", "SPACE CHORUS"), knob ("feedback", "FEEDBACK"),
              knob ("bh_x", "HOLE X"), knob ("bh_y", "HOLE Y") };
    sync = combo ("sync", "SYNC");
    {
        auto* rateK = left[0];
        auto* syncP = proc.apvts.getParameter ("sync");
        auto* rateP = proc.apvts.getParameter ("rate");
        rateK->s.textFromValueFunction = [syncP, rateP] (double v) {
            const int si = juce::roundToInt (syncP->convertFrom0to1 (syncP->getValue()));
            if (si > 0) return "Sync " + syncP->getCurrentValueAsText();
            return rateP->getText (rateP->convertTo0to1 ((float) v), 0); };
        rateK->s.updateText();
        sync->c.onChange = [rateK] { rateK->s.updateText(); };
    } stages = combo ("stages", "STAGES"); voices = combo ("voices", "VOICES");

    for (int i = 0; i < proc.getNumPrograms(); ++i) presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    prevBtn.onClick = [this] { presetBox.setSelectedId ((presetBox.getSelectedId() + 5) % 7 + 1); };
    nextBtn.onClick = [this] { presetBox.setSelectedId (presetBox.getSelectedId() % 7 + 1); };
    for (auto* b : { &holdBtn, &keplerBtn }) b->setClickingTogglesState (true);
    holdAtt = std::make_unique<BA> (proc.apvts, "hold", holdBtn);
    keplerAtt = std::make_unique<BA> (proc.apvts, "kepler", keplerBtn);
    advBtn.setClickingTogglesState (true);
    advBtn.onClick = [this] { setAdvancedOpen (advBtn.getToggleState()); };
    for (auto* c : { (juce::Component*) &presetBox, (juce::Component*) &prevBtn, (juce::Component*) &nextBtn,
                     (juce::Component*) &holdBtn, (juce::Component*) &keplerBtn, (juce::Component*) &advBtn })
        addAndMakeVisible (c);

    setSize (920, 640);
    setAdvancedOpen (false);
}

OrbitEditor::~OrbitEditor() { setLookAndFeel (nullptr); }

void OrbitEditor::setAdvancedOpen (bool open)
{
    advOpen = open;
    advBtn.setToggleState (open, juce::dontSendNotification);
    for (auto* k : adv) { k->s.setVisible (open); k->l.setVisible (open); }
    keplerBtn.setVisible (open); voices->c.setVisible (open); voices->l.setVisible (open);
    resized(); repaint();
}

void OrbitEditor::layoutKnobs (std::vector<Knob*>& ks, juce::Rectangle<int> area, int cols)
{
    const int rows = ((int) ks.size() + cols - 1) / cols;
    const int cw = area.getWidth() / cols, rh = area.getHeight() / juce::jmax (1, rows);
    for (size_t i = 0; i < ks.size(); ++i)
    {
        auto cell = juce::Rectangle<int> (area.getX() + (int) (i % (size_t) cols) * cw, area.getY() + (int) (i / (size_t) cols) * rh, cw, rh).reduced (4, 2);
        ks[i]->l.setBounds (cell.removeFromTop (14));
        ks[i]->s.setBounds (cell);
    }
}

void OrbitEditor::resized()
{
    auto r = getLocalBounds();
    header = r.removeFromTop (52);
    r.reduce (10, 0); r.removeFromBottom (10);
    auto bar = r.removeFromBottom (24);
    advBtn.setBounds (bar.withSizeKeepingCentre (120, 20));
    r.removeFromBottom (6);
    if (advOpen) { advPanel = r.removeFromBottom (128); r.removeFromBottom (8); } else advPanel = {};

    leftPanel = r.removeFromLeft (184); r.removeFromLeft (8);
    rightPanel = r.removeFromRight (184); r.removeFromRight (8);
    field.setBounds (r);

    auto h = header.reduced (14, 12);
    auto mid = h.withSizeKeepingCentre (290, 26);
    prevBtn.setBounds (mid.removeFromLeft (26)); nextBtn.setBounds (mid.removeFromRight (26));
    presetBox.setBounds (mid.reduced (4, 0));
    holdBtn.setBounds (h.removeFromRight (70).withSizeKeepingCentre (70, 26));

    auto lp = leftPanel.reduced (6, 26);
    auto lc = lp.removeFromBottom (68);
    sync->l.setBounds (lc.removeFromTop (14).withTrimmedLeft (6)); sync->c.setBounds (lc.removeFromTop (22).reduced (6, 0));
    lc.removeFromTop (4);
    stages->l.setBounds (lc.removeFromTop (0)); // label drawn inline below
    auto lc2 = lc; stages->c.setBounds (lc2.removeFromTop (22).reduced (6, 0).withTrimmedLeft (60)); stages->l.setBounds (lc.removeFromTop (22).withTrimmedLeft (6).withWidth (60));
    layoutKnobs (left, lp, 2);

    auto rp = rightPanel.reduced (6, 26);
    layoutKnobs (right, rp, 2);

    if (advOpen)
    {
        auto a = advPanel.reduced (10, 0).withTrimmedTop (24).withTrimmedBottom (4);
        auto tail = a.removeFromRight (150);
        layoutKnobs (adv, a, (int) adv.size());
        voices->l.setBounds (tail.removeFromTop (14)); voices->c.setBounds (tail.removeFromTop (22).reduced (0, 0));
        tail.removeFromTop (10); keplerBtn.setBounds (tail.removeFromTop (24));
    }
}

static void drawPanel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    if (r.isEmpty()) return;
    auto f = r.toFloat();
    juce::ColourGradient grad (juce::Colour (0xff1c2128), f.getX(), f.getY(), juce::Colour (0xff111419), f.getX(), f.getBottom(), false);
    g.setGradientFill (grad); g.fillRoundedRectangle (f, 5.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f)); g.drawRoundedRectangle (f.translated (0, 1), 5.0f, 1.0f);
    g.setColour (juce::Colour (0xff2f363f)); g.drawRoundedRectangle (f.reduced (0.5f), 5.0f, 1.0f);
    g.setColour (OrbitLookAndFeel::muted());
    g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)).withExtraKerningFactor (0.25f));
    g.drawText (title, r.reduced (10, 6).removeFromTop (12), juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xff2a3038)); g.drawHorizontalLine (r.getY() + 21, (float) r.getX() + 10, (float) r.getRight() - 10);
}

void OrbitEditor::paint (juce::Graphics& g)
{
    // brushed-metal chassis
    g.fillAll (juce::Colour (0xff15181d));
    juce::Random rn (5);
    for (int y = 0; y < getHeight(); y += 2)
    {
        g.setColour (juce::Colours::white.withAlpha (0.008f + 0.012f * rn.nextFloat()));
        g.drawHorizontalLine (y, 0.0f, (float) getWidth());
    }
    // header bezel
    auto hb = header.toFloat();
    juce::ColourGradient hg (juce::Colour (0xff2b323b), 0, hb.getY(), juce::Colour (0xff171b20), 0, hb.getBottom(), false);
    g.setGradientFill (hg); g.fillRect (hb);
    g.setColour (juce::Colours::black); g.drawHorizontalLine (header.getBottom() - 1, 0, (float) getWidth());
    g.setColour (juce::Colours::white.withAlpha (0.07f)); g.drawHorizontalLine (0, 0, (float) getWidth());
    // screws
    for (auto pt : { juce::Point<float> (8, 8), juce::Point<float> ((float) getWidth() - 8, 8), juce::Point<float> (8, (float) getHeight() - 8), juce::Point<float> ((float) getWidth() - 8, (float) getHeight() - 8) })
    {
        g.setColour (juce::Colour (0xff3a424c)); g.fillEllipse (pt.x - 3.5f, pt.y - 3.5f, 7, 7);
        g.setColour (juce::Colour (0xff12151a)); g.drawLine (pt.x - 2, pt.y, pt.x + 2, pt.y, 1.0f);
    }
    g.setColour (OrbitLookAndFeel::text());
    g.setFont (juce::Font (juce::FontOptions (22.0f, juce::Font::bold)).withExtraKerningFactor (0.35f));
    g.drawText ("ORBIT", 22, 10, 120, 32, juce::Justification::centredLeft);
    g.setColour (OrbitLookAndFeel::muted());
    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)).withExtraKerningFactor (0.3f));
    g.drawText ("GRAVITY WARP", 126, 10, 140, 32, juce::Justification::centredLeft);
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    g.drawText ("PLUGIN LAB", getWidth() - 200, 10, 100, 32, juce::Justification::centredRight);

    drawPanel (g, leftPanel, "ORBIT / MOTION");
    drawPanel (g, rightPanel, "SOUND CHARACTER");
    drawPanel (g, advPanel, "ADVANCED");

    // frame around the field (metal bezel)
    auto fr = field.getBounds().toFloat().expanded (3.0f);
    juce::ColourGradient fg (juce::Colour (0xff3a424c), 0, fr.getY(), juce::Colour (0xff0e1114), 0, fr.getBottom(), false);
    g.setGradientFill (fg); g.drawRoundedRectangle (fr, 4.0f, 3.0f);
}

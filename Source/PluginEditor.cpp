#include "PluginEditor.h"
using namespace juce;

VocMixAudioProcessorEditor::VocMixAudioProcessorEditor (VocMixAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    laf.setColour (Slider::rotarySliderFillColourId, Colour (0xffd8d8d8));
    laf.setColour (Slider::rotarySliderOutlineColourId, Colour (0xff555558));
    laf.setColour (Slider::thumbColourId, Colours::white);
    laf.setColour (Slider::textBoxTextColourId, Colour (0xffcfcfcf));
    laf.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    laf.setColour (Label::textColourId, Colour (0xffb8b8b8));
    laf.setColour (ToggleButton::tickColourId, Colour (0xffff3b30));
    laf.setColour (ToggleButton::textColourId, Colour (0xffcfcfcf));
    laf.setColour (ComboBox::backgroundColourId, Colour (0xff1d1d20));
    setLookAndFeel (&laf);

    panels.reserve (7);
    const char* titles[] = { "preamp", "comp", "equalizer", "shine / doubler", "delay", "space", "de-esser / out" };
    for (auto* t : titles) { panels.emplace_back(); panels.back().title = t; }

    addKnob (0, "pre_gain", "gain");   addChoice (0, "pre_emu", "emulation");
    addKnob (0, "pre_hpf", "low cut"); addKnob (0, "pre_clar", "clarity");
    addKnob (0, "pre_lpf", "high cut");addKnob (0, "pre_level", "level");
    addToggle (0, "pre_on", "on");

    addChoice (1, "comp_mode", "mode"); addKnob (1, "comp_amt", "comp");
    addKnob (1, "comp_mix", "mix");     addKnob (1, "comp_level", "level");
    addToggle (1, "comp_on", "on");

    addChoice (2, "eq_air", "air band"); addKnob (2, "eq_airgain", "air gain");
    addKnob (2, "eq_2k5", "2.5 kHz");    addKnob (2, "eq_650", "650 Hz");
    addKnob (2, "eq_160", "160 Hz");     addKnob (2, "eq_40", "40 Hz");
    addKnob (2, "eq_sub", "sub");        addToggle (2, "eq_on", "on");

    addKnob (3, "shine_level", "shine"); addKnob (3, "shine_freq", "freq");
    addKnob (3, "dbl_wide", "wide");     addKnob (3, "dbl_level", "dbl level");
    addToggle (3, "shine_on", "on");

    addToggle (4, "dly_sync", "sync");   addKnob (4, "dly_time", "time (ms)");
    addChoice (4, "dly_div", "division");addKnob (4, "dly_fb", "feedback");
    addToggle (4, "dly_pp", "ppong");    addToggle (4, "dly_lofi", "lofi");
    addKnob (4, "dly_duck", "ducking");  addKnob (4, "dly_dry", "dry");
    addKnob (4, "dly_wet", "wet");       addToggle (4, "dly_on", "on");

    addKnob (5, "spc_len", "length");    addKnob (5, "spc_tone", "tone");
    addToggle (5, "spc_shim", "shimmer");addKnob (5, "spc_duck", "ducking");
    addKnob (5, "spc_dry", "dry");       addKnob (5, "spc_wet", "wet");
    addToggle (5, "spc_on", "on");

    addKnob (6, "ds_int", "intensity");  addKnob (6, "ds_freq", "freq");
    addToggle (6, "ds_listen", "listen");addKnob (6, "lim_ceil", "limiter");
    addKnob (6, "master", "master");

    setSize (1400, 640);
}

VocMixAudioProcessorEditor::~VocMixAudioProcessorEditor() { setLookAndFeel (nullptr); }

void VocMixAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (Colour (0xff111113));
    g.setColour (Colours::white);
    g.setFont (Font (FontOptions (22.f, Font::bold)));
    g.drawText ("VocMix", 20, 8, 600, 34, Justification::centredLeft);

    for (auto& p : panels)
    {
        g.setColour (Colour (0xff2a2a2d));
        g.fillRoundedRectangle (p.bounds.toFloat(), 6.f);
        g.setColour (Colour (0xff3c3c40));
        g.drawRoundedRectangle (p.bounds.toFloat(), 6.f, 1.5f);
        g.setColour (Colour (0xffa8a8ad));
        g.setFont (Font (FontOptions (17.f, Font::bold)));
        g.drawText (p.title, p.bounds.withHeight (30), Justification::centred);
    }
}

void VocMixAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (44);
    const int gap = 6, np = (int) panels.size();
    const int pw = (area.getWidth() - gap * (np - 1)) / np;

    for (int k = 0; k < np; ++k)
    {
        auto& p = panels[(size_t) k];
        p.bounds = Rectangle<int> (area.getX() + k * (pw + gap), area.getY(), pw, area.getHeight());
        auto inner = p.bounds.reduced (6); inner.removeFromTop (28);
        const int cnt = (int) p.items.size(), rows = (cnt + 1) / 2;
        const int cw = inner.getWidth() / 2, ch = jmin (120, inner.getHeight() / jmax (1, rows));
        for (int i = 0; i < cnt; ++i)
            p.items[(size_t) i]->setBounds (inner.getX() + (i % 2) * cw, inner.getY() + (i / 2) * ch, cw, ch);
    }
    repaint();
}

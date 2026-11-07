#pragma once
#include "PluginProcessor.h"

class VocMixAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VocMixAudioProcessorEditor (VocMixAudioProcessor&);
    ~VocMixAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using APVTS = juce::AudioProcessorValueTreeState;

    struct Knob : juce::Component
    {
        Knob (APVTS& s, const juce::String& id, const juce::String& name)
        {
            slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 14);
            label.setText (name, juce::dontSendNotification);
            label.setJustificationType (juce::Justification::centred);
            addAndMakeVisible (slider); addAndMakeVisible (label);
            att = std::make_unique<APVTS::SliderAttachment> (s, id, slider);
        }
        void resized() override { auto r = getLocalBounds(); label.setBounds (r.removeFromBottom (16)); slider.setBounds (r); }
        juce::Slider slider; juce::Label label; std::unique_ptr<APVTS::SliderAttachment> att;
    };

    struct Choice : juce::Component
    {
        Choice (APVTS& s, const juce::String& id, const juce::String& name)
        {
            if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (id))) box.addItemList (p->choices, 1);
            label.setText (name, juce::dontSendNotification);
            label.setJustificationType (juce::Justification::centred);
            addAndMakeVisible (box); addAndMakeVisible (label);
            att = std::make_unique<APVTS::ComboBoxAttachment> (s, id, box);
        }
        void resized() override { auto r = getLocalBounds().reduced (4, 0); label.setBounds (r.removeFromTop (16)); box.setBounds (r.withSizeKeepingCentre (r.getWidth(), 26)); }
        juce::ComboBox box; juce::Label label; std::unique_ptr<APVTS::ComboBoxAttachment> att;
    };

    struct Toggle : juce::Component
    {
        Toggle (APVTS& s, const juce::String& id, const juce::String& name)
        {
            button.setButtonText (name);
            addAndMakeVisible (button);
            att = std::make_unique<APVTS::ButtonAttachment> (s, id, button);
        }
        void resized() override { button.setBounds (getLocalBounds().withSizeKeepingCentre (getWidth() - 8, 30)); }
        juce::ToggleButton button; std::unique_ptr<APVTS::ButtonAttachment> att;
    };

    struct Panel { juce::String title; std::vector<std::unique_ptr<juce::Component>> items; juce::Rectangle<int> bounds; };

    void addKnob (size_t p, const char* id, const char* name)   { add (p, std::make_unique<Knob>   (proc.apvts, id, name)); }
    void addChoice (size_t p, const char* id, const char* name) { add (p, std::make_unique<Choice> (proc.apvts, id, name)); }
    void addToggle (size_t p, const char* id, const char* name) { add (p, std::make_unique<Toggle> (proc.apvts, id, name)); }
    void add (size_t p, std::unique_ptr<juce::Component> c) { addAndMakeVisible (*c); panels[p].items.push_back (std::move (c)); }

    VocMixAudioProcessor& proc;
    juce::LookAndFeel_V4 laf;
    std::vector<Panel> panels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocMixAudioProcessorEditor)
};

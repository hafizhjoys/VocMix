#pragma once
#include <JuceHeader.h>

class VocMixAudioProcessor : public juce::AudioProcessor
{
public:
    VocMixAudioProcessor();
    ~VocMixAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VocMix"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    // ---- util ----
    float fv (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    bool  bv (const char* id) const { return fv (id) > 0.5f; }

    struct Ring
    {
        std::vector<float> d; int w = 0;
        void init (int n) { d.assign ((size_t) n, 0.f); w = 0; }
        void push (float x) { d[(size_t) w] = x; if (++w >= (int) d.size()) w = 0; }
        float read (float delay) const
        {
            const int n = (int) d.size();
            float p = (float) w - 1.f - delay;
            while (p < 0.f) p += (float) n;
            int i0 = (int) p; float f = p - (float) i0;
            if (i0 >= n) i0 -= n;
            int i1 = i0 + 1; if (i1 >= n) i1 = 0;
            return d[(size_t) i0] * (1.f - f) + d[(size_t) i1] * f;
        }
    };

    // pitch shifter sederhana (+1 oktaf) untuk "shimmer"
    struct OctaveUp
    {
        Ring r; float ph = 0.f; static constexpr float W = 2048.f;
        void init() { r.init (4096); ph = 0.f; }
        float process (float x)
        {
            r.push (x);
            ph += 1.f; if (ph >= W) ph -= W;
            float ph2 = ph + W * 0.5f; if (ph2 >= W) ph2 -= W;
            auto tap = [&] (float p) { return r.read (W - p) * std::sin (juce::MathConstants<float>::pi * p / W); };
            return tap (ph) + tap (ph2);
        }
    };

    using Filt = juce::dsp::IIR::Filter<float>;
    using Coef = juce::dsp::IIR::Coefficients<float>;

    // ---- modul ----
    void preamp (juce::AudioBuffer<float>&);
    void compressor (juce::AudioBuffer<float>&);
    void equalizer (juce::AudioBuffer<float>&);
    void shineDoubler (juce::AudioBuffer<float>&);
    void delayFx (juce::AudioBuffer<float>&);
    void spaceFx (juce::AudioBuffer<float>&);
    void deesser (juce::AudioBuffer<float>&);

    double sr = 44100.0;
    int maxBlock = 512;

    // preamp
    Filt preHpf[2], preLpf[2], preClar[2];
    float lastHpf = -1, lastLpf = -1, lastClar = -1, preGPrev = 1, preLPrev = 1;
    // comp
    float compGr = 0.f, compLPrev = 1.f;
    // eq
    Filt eq[2][6];
    float eqLast[7] { -999, -999, -999, -999, -999, -999, -999 };
    // shine + doubler
    Filt shineHp[2]; float lastShineF = -1.f;
    Ring dbl[2]; float lfo1 = 0.f, lfo2 = 0.25f;
    // delay
    Ring dlyRing[2]; float dlyCur = 10000.f, dlyEnv = 0.f, lofiState[2] { 0.f, 0.f };
    // space
    juce::Reverb reverb; OctaveUp shim[2]; float spcEnv = 0.f;
    std::vector<float> wetL, wetR;
    // de-esser / out
    Filt dsHp[2]; float lastDsF = -1.f, dsEnv = 0.f, masterPrev = 1.f;
    juce::dsp::Limiter<float> limiter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocMixAudioProcessor)
};

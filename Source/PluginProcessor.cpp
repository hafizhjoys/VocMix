#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

//==============================================================================
AudioProcessorValueTreeState::ParameterLayout VocMixAudioProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto F = [&] (const String& id, const String& n, float lo, float hi, float def, float skew = 1.f)
    { p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, n, NormalisableRange<float> (lo, hi, 0.01f, skew), def)); };
    auto B = [&] (const String& id, const String& n, bool def)
    { p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, n, def)); };
    auto C = [&] (const String& id, const String& n, StringArray s, int def)
    { p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, n, s, def)); };

    // Preamp
    B ("pre_on", "On", true);
    F ("pre_gain", "Gain", -20.f, 20.f, 0.f);
    C ("pre_emu", "Emulation", { "Off", "Tube", "Alloy", "Tube 2", "Tape", "Vinyl" }, 1);
    F ("pre_hpf", "Low Cut", 20.f, 500.f, 20.f, 0.4f);
    F ("pre_clar", "Clarity", 0.f, 100.f, 0.f);
    F ("pre_lpf", "High Cut", 1000.f, 20000.f, 20000.f, 0.4f);
    F ("pre_level", "Level", -20.f, 20.f, 0.f);
    // Comp
    B ("comp_on", "On", true);
    C ("comp_mode", "Mode", { "Light", "Trans", "Moderate", "Bold", "Heavy" }, 2);
    F ("comp_amt", "Comp", 0.f, 100.f, 30.f);
    F ("comp_mix", "Mix", 0.f, 100.f, 100.f);
    F ("comp_level", "Level", -20.f, 20.f, 0.f);
    // EQ
    B ("eq_on", "On", true);
    C ("eq_air", "Air Band", { "Off", "2.5k", "5k", "10k", "20k", "40k" }, 3);
    F ("eq_airgain", "Air Gain", -10.f, 10.f, 0.f);
    F ("eq_2k5", "2.5 kHz", -10.f, 10.f, 0.f);
    F ("eq_650", "650 Hz", -10.f, 10.f, 0.f);
    F ("eq_160", "160 Hz", -10.f, 10.f, 0.f);
    F ("eq_40", "40 Hz", -10.f, 10.f, 0.f);
    F ("eq_sub", "Sub", -10.f, 10.f, 0.f);
    // Shine + Doubler
    B ("shine_on", "On", true);
    F ("shine_level", "Shine", 0.f, 100.f, 20.f);
    F ("shine_freq", "Shine Freq", 1000.f, 20000.f, 5000.f, 0.4f);
    F ("dbl_wide", "Wide", 0.f, 100.f, 0.f);
    F ("dbl_level", "Dbl Level", -20.f, 20.f, 0.f);
    // Delay
    B ("dly_on", "On", true);
    B ("dly_sync", "Sync", true);
    F ("dly_time", "Time ms", 20.f, 1500.f, 300.f, 0.5f);
    C ("dly_div", "Division", { "1/4", "1/4D", "1/4T", "1/8", "1/8D", "1/8T", "1/16", "1/16D", "1/16T" }, 4);
    F ("dly_fb", "Feedback", 0.f, 100.f, 28.f);
    B ("dly_pp", "PPong", true);
    B ("dly_lofi", "LoFi", true);
    F ("dly_duck", "Ducking", 0.f, 100.f, 0.f);
    F ("dly_dry", "Dry dB", -100.f, 0.f, 0.f, 3.f);
    F ("dly_wet", "Wet dB", -100.f, 0.f, -18.f, 3.f);
    // Space
    B ("spc_on", "On", true);
    F ("spc_len", "Length", 0.f, 100.f, 40.f);
    F ("spc_tone", "Tone", 0.f, 100.f, 50.f);
    B ("spc_shim", "Shimmer", false);
    F ("spc_duck", "Ducking", 0.f, 100.f, 0.f);
    F ("spc_dry", "Dry dB", -100.f, 0.f, 0.f, 3.f);
    F ("spc_wet", "Wet dB", -100.f, 0.f, -25.f, 3.f);
    // De-esser / output
    F ("ds_int", "Intensity", 0.f, 100.f, 0.f);
    F ("ds_freq", "DS Freq", 4000.f, 16000.f, 8000.f, 0.6f);
    B ("ds_listen", "Listen", false);
    F ("lim_ceil", "Limiter", -20.f, 0.f, 0.f);
    F ("master", "Master", -20.f, 20.f, 0.f);

    return { p.begin(), p.end() };
}

//==============================================================================
VocMixAudioProcessor::VocMixAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool VocMixAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == AudioChannelSet::stereo()
        && l.getMainOutputChannelSet() == AudioChannelSet::stereo();
}

void VocMixAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate; maxBlock = samplesPerBlock;

    for (int c = 0; c < 2; ++c)
    {
        preHpf[c].reset(); preLpf[c].reset(); preClar[c].reset();
        shineHp[c].reset(); dsHp[c].reset();
        for (auto& f : eq[c]) f.reset();
        dbl[c].init ((int) (sr * 0.1));
        dlyRing[c].init ((int) (sr * 2.1));
        shim[c].init();
        lofiState[c] = 0.f;
    }
    lastHpf = lastLpf = lastClar = lastShineF = lastDsF = -1.f;
    for (auto& v : eqLast) v = -999.f;
    compGr = 0.f; dlyEnv = spcEnv = dsEnv = 0.f;
    dlyCur = (float) (0.3 * sr);
    preGPrev = preLPrev = compLPrev = masterPrev = 1.f;

    reverb.setSampleRate (sr); reverb.reset();
    wetL.assign ((size_t) samplesPerBlock, 0.f);
    wetR.assign ((size_t) samplesPerBlock, 0.f);

    dsp::ProcessSpec spec { sr, (uint32) samplesPerBlock, 2 };
    limiter.prepare (spec);
    limiter.setRelease (50.f);
}

//==============================================================================
void VocMixAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    if (buffer.getNumChannels() < 2) return;

    if (bv ("pre_on"))   preamp (buffer);
    if (bv ("comp_on"))  compressor (buffer);
    if (bv ("eq_on"))    equalizer (buffer);
    if (bv ("shine_on")) shineDoubler (buffer);
    if (bv ("dly_on"))   delayFx (buffer);
    if (bv ("spc_on"))   spaceFx (buffer);
    deesser (buffer);

    // master + limiter
    const int n = buffer.getNumSamples();
    const float target = Decibels::decibelsToGain (fv ("master"));
    for (int c = 0; c < 2; ++c)
    {
        auto* d = buffer.getWritePointer (c);
        for (int i = 0; i < n; ++i)
            d[i] *= masterPrev + (target - masterPrev) * (float) (i + 1) / (float) n;
    }
    masterPrev = target;

    limiter.setThreshold (fv ("lim_ceil"));
    dsp::AudioBlock<float> block (buffer);
    dsp::ProcessContextReplacing<float> ctx (block);
    limiter.process (ctx);
}

//==============================================================================
// 1. PREAMP
static float shapeFn (int t, float x)
{
    switch (t)
    {
        case 1:  return std::tanh (x + 0.15f) - std::tanh (0.15f);                       // tube (asimetris)
        case 2:  return std::atan (3.f * x) * 0.5f;                                      // alloy (lebih keras)
        case 3:  return (std::tanh (1.5f * x + 0.3f) - std::tanh (0.3f)) * 0.8f;         // tube 2
        case 4:  return 1.1f * std::tanh (0.9f * x);                                     // tape (halus)
        case 5:  return std::tanh (1.3f * x) * 0.9f;                                     // vinyl
        default: return x;
    }
}

void VocMixAudioProcessor::preamp (AudioBuffer<float>& b)
{
    const int n = b.getNumSamples();
    const float gT = Decibels::decibelsToGain (fv ("pre_gain"));
    const float lT = Decibels::decibelsToGain (fv ("pre_level"));
    const int emu = (int) fv ("pre_emu");
    const float hpf = fv ("pre_hpf"), lpf = fv ("pre_lpf"), clar = fv ("pre_clar");

    if (hpf != lastHpf)  { lastHpf = hpf;  auto c = Coef::makeHighPass (sr, hpf, 0.707f);  for (auto& f : preHpf) f.coefficients = c; }
    if (lpf != lastLpf)  { lastLpf = lpf;  auto c = Coef::makeLowPass  (sr, jmin ((double) lpf, sr * 0.45), 0.707f); for (auto& f : preLpf) f.coefficients = c; }
    if (clar != lastClar){ lastClar = clar; auto c = Coef::makePeakFilter (sr, 3500.f, 0.7f, Decibels::decibelsToGain (clar * 0.08f)); for (auto& f : preClar) f.coefficients = c; }
    const bool useHp = hpf > 21.f, useLp = lpf < 19900.f, useCl = clar > 0.5f;

    for (int c = 0; c < 2; ++c)
    {
        auto* d = b.getWritePointer (c);
        for (int i = 0; i < n; ++i)
        {
            const float t = (float) (i + 1) / (float) n;
            float x = d[i] * (preGPrev + (gT - preGPrev) * t);
            x = shapeFn (emu, x);
            if (useHp) x = preHpf[c].processSample (x);
            if (useLp) x = preLpf[c].processSample (x);
            if (useCl) x = preClar[c].processSample (x);
            d[i] = x * (preLPrev + (lT - preLPrev) * t);
        }
    }
    preGPrev = gT; preLPrev = lT;
}

//==============================================================================
// 2. COMPRESSOR
void VocMixAudioProcessor::compressor (AudioBuffer<float>& b)
{
    struct M { float ratio, att, rel; };
    static const M modes[] = { { 2.f, 30.f, 200.f }, { 3.f, 25.f, 100.f }, { 4.f, 10.f, 150.f },
                               { 6.f, 5.f, 100.f }, { 10.f, 2.f, 80.f } };
    const M m = modes[jlimit (0, 4, (int) fv ("comp_mode"))];
    const float amt = fv ("comp_amt");
    const float thr = -6.f - amt * 0.4f;
    const float makeup = Decibels::decibelsToGain ((1.f - 1.f / m.ratio) * (-thr) * 0.4f);
    const float aA = std::exp (-1.f / (0.001f * m.att * (float) sr));
    const float aR = std::exp (-1.f / (0.001f * m.rel * (float) sr));
    const float mix = fv ("comp_mix") * 0.01f;
    const float lT = Decibels::decibelsToGain (fv ("comp_level"));
    const int n = b.getNumSamples();
    auto* L = b.getWritePointer (0); auto* R = b.getWritePointer (1);

    for (int i = 0; i < n; ++i)
    {
        const float lvlDb = Decibels::gainToDecibels (jmax (std::abs (L[i]), std::abs (R[i])), -100.f);
        const float over = lvlDb - thr;
        float gr = 0.f; // dB negatif
        if (2.f * over < -6.f)           gr = 0.f;
        else if (2.f * std::abs (over) <= 6.f) gr = (1.f / m.ratio - 1.f) * (over + 3.f) * (over + 3.f) / 12.f;
        else                              gr = (1.f / m.ratio - 1.f) * over;
        const float target = (amt < 0.5f) ? 0.f : -gr;
        const float coef = target > compGr ? aA : aR;
        compGr = target + coef * (compGr - target);
        const float g = Decibels::decibelsToGain (-compGr) * makeup;
        const float lv = compLPrev + (lT - compLPrev) * (float) (i + 1) / (float) n;
        L[i] = (L[i] * (1.f - mix) + L[i] * g * mix) * lv;
        R[i] = (R[i] * (1.f - mix) + R[i] * g * mix) * lv;
    }
    compLPrev = lT;
}

//==============================================================================
// 3. EQUALIZER
void VocMixAudioProcessor::equalizer (AudioBuffer<float>& b)
{
    static const float airF[] = { 0.f, 2500.f, 5000.f, 10000.f, 20000.f, 40000.f };
    const float v[7] = { fv ("eq_air"), fv ("eq_airgain"), fv ("eq_2k5"), fv ("eq_650"),
                         fv ("eq_160"), fv ("eq_40"), fv ("eq_sub") };
    bool changed = false;
    for (int i = 0; i < 7; ++i) if (v[i] != eqLast[i]) { changed = true; eqLast[i] = v[i]; }

    if (changed)
    {
        const int ai = jlimit (0, 5, (int) v[0]);
        const double af = jmin ((double) airF[ai], sr * 0.45);
        const float airG = ai == 0 ? 0.f : v[1];
        Coef::Ptr cs[6] = {
            Coef::makeHighShelf (sr, jmax (af, 1000.0), 0.6f, Decibels::decibelsToGain (airG)),
            Coef::makePeakFilter (sr, 2500.f, 0.8f, Decibels::decibelsToGain (v[2])),
            Coef::makePeakFilter (sr, 650.f,  0.8f, Decibels::decibelsToGain (v[3])),
            Coef::makePeakFilter (sr, 160.f,  0.8f, Decibels::decibelsToGain (v[4])),
            Coef::makePeakFilter (sr, 40.f,   1.0f, Decibels::decibelsToGain (v[5])),
            Coef::makeLowShelf   (sr, 70.f,   0.7f, Decibels::decibelsToGain (v[6])) };
        for (int c = 0; c < 2; ++c)
            for (int k = 0; k < 6; ++k) eq[c][k].coefficients = cs[k];
    }

    const int n = b.getNumSamples();
    for (int c = 0; c < 2; ++c)
    {
        auto* d = b.getWritePointer (c);
        for (int i = 0; i < n; ++i)
        {
            float x = d[i];
            for (int k = 0; k < 6; ++k) x = eq[c][k].processSample (x);
            d[i] = x;
        }
    }
}

//==============================================================================
// 4. SHINE (harmonic exciter) + DOUBLER
void VocMixAudioProcessor::shineDoubler (AudioBuffer<float>& b)
{
    const float lvl = fv ("shine_level") * 0.01f;
    const float sf = fv ("shine_freq");
    if (sf != lastShineF) { lastShineF = sf; auto c = Coef::makeHighPass (sr, jmin ((double) sf, sr * 0.45), 0.707f); for (auto& f : shineHp) f.coefficients = c; }

    const float dg = Decibels::decibelsToGain (fv ("dbl_level")) * 0.5f;
    const float w = fv ("dbl_wide") * 0.01f;
    const float amt = std::sqrt (w);
    const float twoPi = MathConstants<float>::twoPi;
    const int n = b.getNumSamples();
    auto* L = b.getWritePointer (0); auto* R = b.getWritePointer (1);

    for (int i = 0; i < n; ++i)
    {
        float y[2] = { L[i], R[i] };
        for (int c = 0; c < 2; ++c)
        {
            const float hp = shineHp[c].processSample (y[c]);
            y[c] += lvl * 1.5f * std::tanh (4.f * hp) * 0.25f;
        }
        if (w > 0.001f)
        {
            lfo1 += 0.30f / (float) sr; if (lfo1 >= 1.f) lfo1 -= 1.f;
            lfo2 += 0.37f / (float) sr; if (lfo2 >= 1.f) lfo2 -= 1.f;
            const float dl = dbl[0].read ((float) (0.018 * sr) + (float) (0.0015 * sr) * std::sin (twoPi * lfo1));
            const float dr = dbl[1].read ((float) (0.026 * sr) + (float) (0.0015 * sr) * std::sin (twoPi * lfo2));
            const float mono = 0.5f * (dl + dr);
            dbl[0].push (y[0]); dbl[1].push (y[1]);
            L[i] = y[0] + dg * amt * (w * dl + (1.f - w) * mono);
            R[i] = y[1] + dg * amt * (w * dr + (1.f - w) * mono);
        }
        else { dbl[0].push (y[0]); dbl[1].push (y[1]); L[i] = y[0]; R[i] = y[1]; }
    }
}

//==============================================================================
// 5. DELAY (sync, ping-pong, lo-fi, ducking)
void VocMixAudioProcessor::delayFx (AudioBuffer<float>& b)
{
    const bool sync = bv ("dly_sync"), pp = bv ("dly_pp"), lofi = bv ("dly_lofi");
    const float fb = fv ("dly_fb") * 0.01f * 0.95f, duck = fv ("dly_duck") * 0.01f;
    const float dry = Decibels::decibelsToGain (fv ("dly_dry"), -99.f);
    const float wet = Decibels::decibelsToGain (fv ("dly_wet"), -99.f);

    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bb = pos->getBpm()) bpm = *bb;

    static const float beats[] = { 1.f, 1.5f, 0.6667f, 0.5f, 0.75f, 0.3333f, 0.25f, 0.375f, 0.1667f };
    const float ms = sync ? beats[jlimit (0, 8, (int) fv ("dly_div"))] * 60000.f / (float) bpm : fv ("dly_time");
    const float target = jlimit (2.f, (float) dlyRing[0].d.size() - 3.f, ms * 0.001f * (float) sr);
    const float aA = std::exp (-1.f / (0.005f * (float) sr)), aR = std::exp (-1.f / (0.15f * (float) sr));

    const int n = b.getNumSamples();
    auto* L = b.getWritePointer (0); auto* R = b.getWritePointer (1);
    for (int i = 0; i < n; ++i)
    {
        dlyCur += 0.0008f * (target - dlyCur);
        const float inL = L[i], inR = R[i];
        float yL = dlyRing[0].read (dlyCur), yR = dlyRing[1].read (dlyCur);
        if (lofi)
        {
            yL = std::round (yL * 48.f) / 48.f; lofiState[0] += 0.25f * (yL - lofiState[0]); yL = lofiState[0];
            yR = std::round (yR * 48.f) / 48.f; lofiState[1] += 0.25f * (yR - lofiState[1]); yR = lofiState[1];
        }
        const float lv = jmax (std::abs (inL), std::abs (inR));
        dlyEnv = lv + (lv > dlyEnv ? aA : aR) * (dlyEnv - lv);

        if (pp) { const float m = 0.5f * (inL + inR); dlyRing[0].push (m + fb * yR); dlyRing[1].push (fb * yL); }
        else    { dlyRing[0].push (inL + fb * yL); dlyRing[1].push (inR + fb * yR); }

        const float dg = 1.f - duck * jmin (1.f, dlyEnv * 4.f);
        L[i] = inL * dry + yL * wet * dg;
        R[i] = inR * dry + yR * wet * dg;
    }
}

//==============================================================================
// 6. SPACE (reverb + shimmer + ducking)
void VocMixAudioProcessor::spaceFx (AudioBuffer<float>& b)
{
    const int n = b.getNumSamples();
    if ((int) wetL.size() < n) { wetL.assign ((size_t) n, 0.f); wetR.assign ((size_t) n, 0.f); }

    Reverb::Parameters rp;
    rp.roomSize = 0.3f + 0.69f * fv ("spc_len") * 0.01f;
    rp.damping = 1.f - fv ("spc_tone") * 0.01f;
    rp.wetLevel = 1.f; rp.dryLevel = 0.f; rp.width = 1.f; rp.freezeMode = 0.f;
    reverb.setParameters (rp);

    const bool sh = bv ("spc_shim");
    const float duck = fv ("spc_duck") * 0.01f;
    const float dry = Decibels::decibelsToGain (fv ("spc_dry"), -99.f);
    const float wet = Decibels::decibelsToGain (fv ("spc_wet"), -99.f);
    auto* L = b.getWritePointer (0); auto* R = b.getWritePointer (1);

    for (int i = 0; i < n; ++i)
    {
        wetL[(size_t) i] = L[i] + (sh ? 0.6f * shim[0].process (L[i]) : 0.f);
        wetR[(size_t) i] = R[i] + (sh ? 0.6f * shim[1].process (R[i]) : 0.f);
    }
    reverb.processStereo (wetL.data(), wetR.data(), n);

    const float aA = std::exp (-1.f / (0.005f * (float) sr)), aR = std::exp (-1.f / (0.2f * (float) sr));
    for (int i = 0; i < n; ++i)
    {
        const float lv = jmax (std::abs (L[i]), std::abs (R[i]));
        spcEnv = lv + (lv > spcEnv ? aA : aR) * (spcEnv - lv);
        const float dg = 1.f - duck * jmin (1.f, spcEnv * 4.f);
        L[i] = L[i] * dry + wetL[(size_t) i] * wet * dg;
        R[i] = R[i] * dry + wetR[(size_t) i] * wet * dg;
    }
}

//==============================================================================
// 7. DE-ESSER
void VocMixAudioProcessor::deesser (AudioBuffer<float>& b)
{
    const float inten = fv ("ds_int");
    const bool listen = bv ("ds_listen");
    if (inten < 0.5f && ! listen) return;

    const float f = fv ("ds_freq");
    if (f != lastDsF) { lastDsF = f; auto c = Coef::makeHighPass (sr, jmin ((double) f, sr * 0.45), 0.707f); for (auto& x : dsHp) x.coefficients = c; }

    const float thr = -15.f - inten * 0.25f;
    const float ratio = 1.f + inten * 0.1f;
    const float aA = std::exp (-1.f / (0.0005f * (float) sr)), aR = std::exp (-1.f / (0.04f * (float) sr));
    const int n = b.getNumSamples();
    auto* L = b.getWritePointer (0); auto* R = b.getWritePointer (1);

    for (int i = 0; i < n; ++i)
    {
        const float hl = dsHp[0].processSample (L[i]);
        const float hr = dsHp[1].processSample (R[i]);
        const float lv = jmax (std::abs (hl), std::abs (hr));
        dsEnv = lv + (lv > dsEnv ? aA : aR) * (dsEnv - lv);
        const float envDb = Decibels::gainToDecibels (dsEnv, -100.f);
        const float red = envDb > thr ? (envDb - thr) * (1.f - 1.f / ratio) : 0.f;
        const float g = Decibels::decibelsToGain (-red);
        if (listen) { L[i] = hl; R[i] = hr; }
        else        { L[i] -= hl * (1.f - g); R[i] -= hr * (1.f - g); }
    }
}

//==============================================================================
AudioProcessorEditor* VocMixAudioProcessor::createEditor() { return new VocMixAudioProcessorEditor (*this); }

void VocMixAudioProcessor::getStateInformation (MemoryBlock& d)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, d);
}

void VocMixAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VocMixAudioProcessor(); }

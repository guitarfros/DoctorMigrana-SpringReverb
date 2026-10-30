// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 GuitarFros

#pragma once
// Motor DSP da reverb de resorte (estilo Fender). Só usa a libraría estándar de C++,
// así que se pode probar fóra de JUCE. Port directo do prototipo Faust SpringFender.dsp.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace dm
{
constexpr double kPi = 3.14159265358979323846;

// ---------- Biquad Butterworth (RBJ) ----------
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    void setLowpass (double fs, double f, double q = 0.70710678)
    {
        const double w0 = 2.0 * kPi * f / fs, c = std::cos (w0), al = std::sin (w0) / (2.0 * q);
        const double a0 = 1.0 + al;
        b0 = float (((1.0 - c) * 0.5) / a0); b1 = float ((1.0 - c) / a0); b2 = b0;
        a1 = float ((-2.0 * c) / a0);        a2 = float ((1.0 - al) / a0);
    }
    void setHighpass (double fs, double f, double q = 0.70710678)
    {
        const double w0 = 2.0 * kPi * f / fs, c = std::cos (w0), al = std::sin (w0) / (2.0 * q);
        const double a0 = 1.0 + al;
        b0 = float (((1.0 + c) * 0.5) / a0); b1 = float (-(1.0 + c) / a0); b2 = b0;
        a1 = float ((-2.0 * c) / a0);         a2 = float ((1.0 - al) / a0);
    }
    float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void reset() { z1 = z2 = 0; }
};

// ---------- Pasabaixos dun polo ----------
struct OnePole
{
    float y = 0;
    static float coeff (double fs, double fc) { return float (1.0 - std::exp (-2.0 * kPi * fc / fs)); }
    float process (float x, float g) { y += g * (x - y); return y; }
    void reset() { y = 0; }
};

// ---------- Ruído de baixa frecuencia (interpolado) ----------
struct LFNoise
{
    uint32_t s = 12345; float cur = 0, next = 0, phase = 0, inc = 0;
    float rnd() { s = s * 1664525u + 1013904223u; return float (s >> 8) / 8388608.0f - 1.0f; }
    void prepare (double fs, double hz, uint32_t seed) { inc = float (hz / fs); s = seed; cur = rnd(); next = rnd(); phase = 0; }
    float tick()
    {
        const float y = cur + (next - cur) * phase;
        phase += inc;
        if (phase >= 1.0f) { phase -= 1.0f; cur = next; next = rnd(); }
        return y;
    }
};

// ---------- Delay fraccionario (interpolación lineal) ----------
struct FracDelay
{
    std::vector<float> buf; unsigned mask = 0, w = 0;
    void prepare (int maxSamples)
    {
        unsigned n = 1; while (n < unsigned (maxSamples + 4)) n <<= 1;
        buf.assign (n, 0.0f); mask = n - 1; w = 0;
    }
    void write (float x) { buf[w] = x; w = (w + 1) & mask; }
    // d = mostras de atraso respecto da última mostra escrita (d = 0 -> a última)
    float read (float d) const
    {
        const int i = int (d); const float fr = d - float (i);
        const float a = buf[(w - 1u - unsigned (i)) & mask];
        const float b = buf[(w - 2u - unsigned (i)) & mask];
        return a + (b - a) * fr;
    }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); w = 0; }
};

// ---------- Un resorte: lazo delay + cadea allpass dispersiva + perdas ----------
class Spring
{
public:
    void prepare (double fs, double delaySec, float allpassCoeff, double modHz, uint32_t seed)
    {
        a = allpassCoeff;
        // Mantense o mesmo "chirp" en segundos a calquera taxa de mostraxe (calibrado a 48 kHz)
        nStages = std::clamp (int (std::lround (80.0 * fs / 48000.0)), 40, 320);
        xz.assign (size_t (nStages), 0.0f); yz.assign (size_t (nStages), 0.0f);
        baseDelay = float (delaySec * fs);
        modDepth  = float (1.5 * fs / 48000.0);
        delay.prepare (int (baseDelay + modDepth + 8));
        noise.prepare (fs, modHz, seed);
        lpG = OnePole::coeff (fs, 3500.0);
        lp.reset(); yPrev = 0;
    }
    void reset() { delay.reset(); std::fill (xz.begin(), xz.end(), 0.0f); std::fill (yz.begin(), yz.end(), 0.0f); lp.reset(); yPrev = 0; }

    float process (float in, float fb)
    {
        delay.write (in + fb * yPrev);
        float v = delay.read (baseDelay + noise.tick() * modDepth);
        for (int i = 0; i < nStages; ++i)          // allpass: y = a*x + x1 - a*y1
        {
            const float y = a * v + xz[size_t (i)] - a * yz[size_t (i)];
            xz[size_t (i)] = v; yz[size_t (i)] = y; v = y;
        }
        yPrev = lp.process (v, lpG);
        return yPrev;
    }

private:
    float a = -0.6f, baseDelay = 1000, modDepth = 1.5f, lpG = 0.3f, yPrev = 0;
    int nStages = 80;
    std::vector<float> xz, yz;
    FracDelay delay; LFNoise noise; OnePole lp;
};

// ---------- Reverb completa ----------
class SpringReverb
{
public:
    static constexpr float kWetGain = 1.5f, kFbMin = 0.70f, kFbRange = 0.25f;
    static constexpr float kNormRef = 0.5268f;   // sqrt(1 - 0.85^2): referencia de nivel con Decay = 0.6

    static float softLimit (float x)             // lineal ata 0.7 e satura suavemente cara a 1.0
    {
        const float ax = std::fabs (x);
        if (ax <= 0.7f) return x;
        const float y = 0.7f + 0.3f * std::tanh ((ax - 0.7f) / 0.3f);
        return x < 0 ? -y : y;
    }

    void prepare (double sampleRate)
    {
        fs = sampleRate;
        hp.setHighpass (fs, 100.0); lp.setLowpass (fs, 4500.0);
        springL.prepare (fs, 0.031, -0.62f, 1.7, 1111u);
        springR.prepare (fs, 0.037, -0.58f, 2.3, 2222u);
        smoothK = float (1.0 - std::exp (-1.0 / (0.02 * fs)));
        reset();
    }
    void reset()
    {
        hp.reset(); lp.reset(); springL.reset(); springR.reset(); toneL.reset(); toneR.reset();
        sDwell = tDwell; sDecay = tDecay; sTone = tTone; sMix = tMix; sMaster = tMaster; sActive = tActive;
    }
    // todos os valores en 0..1
    // masterGain: gañancia lineal de saída (1.0 = 0 dB), só soa con o pedal activo
    void setParams (float dwell, float decay, float tone, float mix, float masterGain, bool active)
    {
        tDwell = dwell; tDecay = decay; tTone = tone; tMix = mix; tMaster = masterGain; tActive = active ? 1.0f : 0.0f;
    }

    void process (float inL, float inR, float& outL, float& outR)
    {
        sDwell += smoothK * (tDwell - sDwell); sDecay += smoothK * (tDecay - sDecay);
        sTone  += smoothK * (tTone  - sTone);  sMix   += smoothK * (tMix   - sMix);
        sActive += smoothK * (tActive - sActive);  sMaster += smoothK * (tMaster - sMaster);

        const float fb = kFbMin + sDecay * kFbRange;

        float x = lp.process (hp.process (0.5f * (inL + inR)));          // entrada con banda limitada
        x = std::tanh (x * (0.5f + sDwell * 6.0f)) * 0.8f;               // Dwell: saturación suave

        const float toneG = OnePole::coeff (fs, 1500.0 + double (sTone) * 6500.0);
        const float wl = toneL.process (springL.process (x, fb), toneG);
        const float wr = toneR.process (springR.process (x, fb), toneG);

        // Nivel húmido estable aínda que cambie Decay + limitador suave só para o húmido
        const float norm = std::sqrt (1.0f - fb * fb) / kNormRef;
        const float dry = 1.0f - sMix, wet = sMix * kWetGain * norm;
        const float eL = inL * dry + softLimit (wl * wet), eR = inR * dry + softLimit (wr * wet);
        outL = inL + (eL * sMaster - inL) * sActive;                     // Master + bypass con fundido
        outR = inR + (eR * sMaster - inR) * sActive;
    }

private:
    double fs = 48000.0;
    Biquad hp, lp; Spring springL, springR; OnePole toneL, toneR;
    float smoothK = 0.001f;
    float tDwell = 0.5f, tDecay = 0.6f, tTone = 0.5f, tMix = 0.35f, tMaster = 1.0f, tActive = 1.0f;
    float sDwell = 0.5f, sDecay = 0.6f, sTone = 0.5f, sMix = 0.35f, sMaster = 1.0f, sActive = 1.0f;
};
} // namespace dm

// Eliane - porting C++ per Daisy (Seed / Pod), v1.0-a
//
// Opera derivata. Originale SuperCollider:
//   // @sixolet
//   // Eliane
//   // Feedback, slow beatings, highs and lows
//   // (seven sines phase-modulating each other in a crosslinked ring)
// Distribuito con dronecaster (norns), https://norns.community/dronecaster/
// Licenza di dronecaster: GNU GPL v3.0. Questo file e' distribuito con la stessa licenza.
//
// Modifiche rispetto all'originale (2026-10-28, progetto Aeolian):
//   - riscrittura in C++ senza dipendenze (nessun modulo DaisySP)
//   - UGen SuperCollider sostituiti da equivalenti propri (vedi note nel codice)
//   - esposti solo hz e amp; gli altri valori restano costanti dell'originale
//
// Questo programma e' software libero: puoi redistribuirlo e/o modificarlo secondo i termini
// della GNU General Public License come pubblicata dalla Free Software Foundation, versione 3
// o (a tua scelta) successiva. E' distribuito senza alcuna garanzia.
#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace eliane
{
constexpr int   kBlock  = 64;    // blocco audio = blocco di controllo SC (LocalIn ritarda di 1 blocco)
constexpr int   kVoices = 7;
constexpr int   kCombMax = 14400; // 0.3 s a 48 kHz
constexpr float kPi     = 3.14159265358979f;
constexpr float kTwoPi  = 6.28318530717959f;

// Costanti dell'originale
constexpr float kIntervals[kVoices] = {0, 7, 14, 20, 27, 34, 41}; // semitoni
constexpr float kEnvOffset = 0.2f;   // add: di LFDNoise3
constexpr float kLagSec    = 13.0f;  // lag2 inviluppi

// Generatore pseudo-casuale xorshift32, uniforme in [-1,1)
class Rng
{
  public:
    void  Seed(uint32_t v) { s_ = v ? v : 0x9E3779B9u; for(int i = 0; i < 4; i++) Bipolar(); }
    float Bipolar()
    {
        s_ ^= s_ << 13; s_ ^= s_ >> 17; s_ ^= s_ << 5;
        return float(int32_t(s_)) * (1.0f / 2147483648.0f);
    }
    uint32_t Raw() { Bipolar(); return s_; }
  private:
    uint32_t s_ = 1;
};

// Equivalente di LFNoise2.kr: valori casuali a frequenza freq, interpolazione quadratica
// (B-spline sui punti medi, continua in valore e derivata). Non e' l'algoritmo esatto di SC.
class Noise2
{
  public:
    void Init(Rng* r)
    {
        r_ = r; p0_ = r->Bipolar(); p1_ = r->Bipolar(); p2_ = r->Bipolar();
        ph_ = 0.5f * (r->Bipolar() + 1.0f);
    }
    float Process(float freq, float dt)
    {
        ph_ += freq * dt;
        while(ph_ >= 1.0f) { ph_ -= 1.0f; p0_ = p1_; p1_ = p2_; p2_ = r_->Bipolar(); }
        const float u = 1.0f - ph_;
        return u * u * 0.5f * (p0_ + p1_) + 2.0f * u * ph_ * p1_ + ph_ * ph_ * 0.5f * (p1_ + p2_);
    }
  private:
    Rng* r_ = nullptr;
    float p0_ = 0, p1_ = 0, p2_ = 0, ph_ = 0;
};

// Equivalente di LFDNoise3.kr: frequenza variabile, interpolazione cubica
class Noise3
{
  public:
    void Init(Rng* r)
    {
        r_ = r; p0_ = r->Bipolar(); p1_ = r->Bipolar(); p2_ = r->Bipolar(); p3_ = r->Bipolar();
        ph_ = 0.5f * (r->Bipolar() + 1.0f);
    }
    float Process(float freq, float dt)
    {
        ph_ += freq * dt;
        while(ph_ >= 1.0f)
        {
            ph_ -= 1.0f; p0_ = p1_; p1_ = p2_; p2_ = p3_; p3_ = r_->Bipolar();
        }
        const float c1 = 0.5f * (p2_ - p0_);
        const float c2 = p0_ - 2.5f * p1_ + 2.0f * p2_ - 0.5f * p3_;
        const float c3 = 0.5f * (p3_ - p0_) + 1.5f * (p1_ - p2_);
        return ((c3 * ph_ + c2) * ph_ + c1) * ph_ + p1_;
    }
  private:
    Rng* r_ = nullptr;
    float p0_ = 0, p1_ = 0, p2_ = 0, p3_ = 0, ph_ = 0;
};

// Lag2: due one-pole in cascata, tempo = decadimento di 60 dB (come Lag di SC)
class Lag2
{
  public:
    float Process(float in, float b1)
    {
        if(!init_) { y1_ = y2_ = in; init_ = true; }
        y1_ = in + b1 * (y1_ - in);
        y2_ = y1_ + b1 * (y2_ - y1_);
        return y2_;
    }
  private:
    float y1_ = 0, y2_ = 0;
    bool  init_ = false;
};

// clip(LFDNoise3.kr(LFNoise2.kr(1/300).range(1/120,1/30), add: 0.2), 0, 1).lag2(13)
class GainEnv
{
  public:
    void Init(Rng* r) { rate_.Init(r); noise_.Init(r); }
    float Process(float dt, float lagB1)
    {
        const float rn   = rate_.Process(1.0f / 300.0f, dt);
        const float rate = 1.0f / 120.0f + 0.5f * (rn + 1.0f) * (1.0f / 30.0f - 1.0f / 120.0f);
        float v = noise_.Process(rate, dt) + kEnvOffset;
        v = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
        return lag_.Process(v, lagB1);
    }
  private:
    Noise2 rate_;
    Noise3 noise_;
    Lag2   lag_;
};

struct Voice
{
    Noise2  depthN, rotN, freqN[2], subN[2];
    GainEnv fbEnv, outEnv;
    float   ph[2]    = {0, 0};
    float   phSub[2] = {0, 0};
};

class Eliane
{
  public:
    // combL/combR: due buffer da kCombMax float ciascuno (su Daisy: SDRAM)
    bool Init(float sampleRate, float* combL, float* combR, uint32_t seed)
    {
        sr_ = sampleRate;
        combLen_ = int(0.3f * sr_ + 0.5f);
        if(combLen_ > kCombMax || combLen_ < 1) return false;
        comb_[0] = combL; comb_[1] = combR;
        std::memset(combL, 0, sizeof(float) * kCombMax);
        std::memset(combR, 0, sizeof(float) * kCombMax);
        std::memset(fb_, 0, sizeof(fb_));
        rng_.Seed(seed);
        for(int x = 0; x < kVoices; x++)
        {
            Voice& v = voices_[x];
            v.depthN.Init(&rng_); v.rotN.Init(&rng_);
            for(int c = 0; c < 2; c++) { v.freqN[c].Init(&rng_); v.subN[c].Init(&rng_); }
            v.fbEnv.Init(&rng_); v.outEnv.Init(&rng_);
            for(int c = 0; c < 2; c++)
            {
                v.ph[c]    = 0.5f * (rng_.Bipolar() + 1.0f);
                v.phSub[c] = 0.5f * (rng_.Bipolar() + 1.0f);
            }
        }
        niceN_.Init(&rng_); decayN_.Init(&rng_);
        const float blockRate = sr_ / float(kBlock);
        lagB1_  = std::exp(std::log(0.001f) / (kLagSec * blockRate));
        slewHz_ = 1.0f - std::exp(-1.0f / (1.0f * blockRate));   // tau 1 s
        slewAmp_ = 1.0f - std::exp(-1.0f / (0.05f * blockRate)); // tau 50 ms
        hz_ = hzTarget_; amp_ = ampTarget_;
        return true;
    }

    // hz: frequenza fondamentale (SC: arg hz, default 60); amp: ampiezza (SC: arg amp, default 0.5)
    void SetHz(float hz)  { hzTarget_  = hz < 10.0f ? 10.0f : (hz > 1000.0f ? 1000.0f : hz); }
    void SetAmp(float a)  { ampTarget_ = a < 0.0f ? 0.0f : (a > 1.0f ? 1.0f : a); }
    float Hz() const { return hzTarget_; }

    // Produce esattamente kBlock campioni per canale
    void Process(float* outL, float* outR)
    {
        const float dt = float(kBlock) / sr_;
        hz_  += (hzTarget_ - hz_) * slewHz_;
        amp_ += (ampTarget_ - amp_) * slewAmp_;
        const float hz = hz_;

        const float nice = niceN_.Process(1.0f / 200.0f, dt);
        const float expo = 0.9f + 0.5f * (nice + 1.0f) * (1.7f - 0.9f); // nice.range(0.9, 1.7)

        float mix[2][kBlock];
        std::memset(mix, 0, sizeof(mix));
        float (*prev)[2][kBlock] = fb_[prevIdx_];
        float (*cur)[2][kBlock]  = fb_[1 - prevIdx_];

        for(int x = 0; x < kVoices; x++)
        {
            Voice& v = voices_[x];
            const float u     = 0.5f * (v.depthN.Process(1.0f / 66.0f, dt) + 1.0f);
            const float depth = std::pow(u < 0.0f ? 0.0f : u, expo);
            const float modK  = kPi * depth;
            const float rot   = v.rotN.Process(1.0f / 25.0f, dt) * 0.5f * kPi; // Rotate2 pos*pi
            const float ca = std::cos(rot), sa = std::sin(rot);
            const float fbG  = v.fbEnv.Process(dt, lagB1_);
            const float outG = v.outEnv.Process(dt, lagB1_);

            float inc[2], incSub[2];
            const float ratio = std::pow(2.0f, kIntervals[x] / 12.0f);
            for(int c = 0; c < 2; c++)
            {
                inc[c] = (hz * ratio + (float(x) * 0.5f) * v.freqN[c].Process(1.0f / 15.0f, dt)) / sr_;
                incSub[c] = 0.0f;
            }
            // voci 5 e 6: crossfade (equal power, come SelectX) tra parziale alto e subarmonica
            float gMain = 1.0f / float(x + 1), gSub = 0.0f;
            if(x == 6 || x == 5)
            {
                const float div = (x == 6) ? hz / (150.0f + hz) : hz / (300.0f + hz);
                const float w   = div;
                const float sub = (x == 6) ? hz / 2.0f : hz / 3.0f;
                gMain = std::cos(w * 0.5f * kPi) / ((x == 6 ? 1.5f : 1.3f) * float(x) + 1.0f);
                gSub  = std::sin(w * 0.5f * kPi) * (x == 6 ? 1.0f : 0.5f);
                for(int c = 0; c < 2; c++)
                    incSub[c] = (sub + v.subN[c].Process(1.0f / 15.0f, dt)) / sr_;
            }

            const float(*nx)[kBlock] = prev[(x + 1) % kVoices];
            const float(*nn)[kBlock] = prev[(x + 2) % kVoices];
            const float k1 = float(x + 1);
            const bool  hasSub = (x == 5 || x == 6);

            for(int n = 0; n < kBlock; n++)
            {
                float st[2];
                for(int c = 0; c < 2; c++)
                {
                    const float m = modK * (k1 * nx[c][n] + nn[c][n]);
                    v.ph[c] += inc[c];
                    v.ph[c] -= std::floor(v.ph[c]);
                    float s = gMain * std::sin(kTwoPi * v.ph[c] + m);
                    if(hasSub)
                    {
                        v.phSub[c] += incSub[c];
                        v.phSub[c] -= std::floor(v.phSub[c]);
                        s += gSub * std::sin(kTwoPi * v.phSub[c] + m);
                    }
                    st[c] = s;
                }
                const float l = ca * st[0] + sa * st[1];
                const float r = ca * st[1] - sa * st[0];
                cur[x][0][n] = l * fbG;
                cur[x][1][n] = r * fbG;
                mix[0][n] += l * outG;
                mix[1][n] += r * outG;
            }
        }
        prevIdx_ = 1 - prevIdx_;

        // CombN: uscita = segnale ritardato (solo wet), g = 0.001^(delay/decay)
        const float dv    = decayN_.Process(0.02f, dt);
        const float decay = 0.3f * std::pow(10.0f, 0.5f * (dv + 1.0f)); // exprange(0.3, 3.0)
        const float delay = float(combLen_) / sr_;
        const float g     = std::pow(0.001f, delay / decay);
        const float scale = 0.5f * amp_;
        bool bad = false;
        for(int n = 0; n < kBlock; n++)
        {
            const float dl = comb_[0][combIdx_], dr = comb_[1][combIdx_];
            comb_[0][combIdx_] = mix[0][n] * scale + g * dl;
            comb_[1][combIdx_] = mix[1][n] * scale + g * dr;
            if(++combIdx_ >= combLen_) combIdx_ = 0;
            outL[n] = std::tanh(dl);
            outR[n] = std::tanh(dr);
            if(!(std::isfinite(outL[n]) && std::isfinite(outR[n]))) bad = true;
        }
        if(bad) Reset();
    }

  private:
    // Rete di sicurezza: azzera lo stato se compare un valore non finito
    void Reset()
    {
        std::memset(comb_[0], 0, sizeof(float) * kCombMax);
        std::memset(comb_[1], 0, sizeof(float) * kCombMax);
        std::memset(fb_, 0, sizeof(fb_));
    }

    float    sr_ = 48000.0f;
    int      combLen_ = kCombMax, combIdx_ = 0;
    float*   comb_[2] = {nullptr, nullptr};
    float    fb_[2][kVoices][2][kBlock];
    int      prevIdx_ = 0;
    Voice    voices_[kVoices];
    Noise2   niceN_, decayN_;
    Rng      rng_;
    float    hz_ = 60.0f, hzTarget_ = 60.0f, amp_ = 0.5f, ampTarget_ = 0.5f;
    float    lagB1_ = 0, slewHz_ = 0, slewAmp_ = 0;
};

} // namespace eliane

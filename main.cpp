// Aeolian v1.0-a - Eliane su Daisy Pod (Seed inclusa nel Pod)
//
// Opera derivata da "Eliane" di @sixolet (dronecaster, GPLv3):
//   // @sixolet
//   // Eliane
//   // Feedback, slow beatings, highs and lows
//   // (seven sines phase-modulating each other in a crosslinked ring)
// Porting Daisy: progetto Aeolian, 2026-10-28. Licenza: GNU GPL v3.0 (vedi eliane.h).
//
// Controlli (v1.0-b): encoder = hz (un semitono per scatto, 10..1000 Hz, default 60),
// POT1 = densita', POT2 = ruvidita'. amp e' fissa (0.5): il volume reale si regola col
// trimmer analogico del Pod (stadio cuffie) o, sull'uscita AUX, non e' regolabile in questo step.
// LED1: verde fisso = in esecuzione. LED2: carico CPU (verde < 50%, giallo < 80%, rosso oltre).
// Prerequisiti: libDaisy compilata, Daisy Toolchain. Nessun modulo DaisySP.
#include "daisy_pod.h"
#include "util/CpuLoadMeter.h"
#include "eliane.h"

using namespace daisy;

static DaisyPod       hw;
static eliane::Eliane synth;
static CpuLoadMeter   cpu;

// Buffer del comb in SDRAM (2 x 14400 float = 115 kB)
static float DSY_SDRAM_BSS combL[eliane::kCombMax];
static float DSY_SDRAM_BSS combR[eliane::kCombMax];

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    cpu.OnBlockStart();
    (void)in;
    // size e' sempre kBlock (SetAudioBlockSize(64))
    if(size == eliane::kBlock) { synth.Process(out[0], out[1]); }
    else
    {
        for(size_t i = 0; i < size; i++) { out[0][i] = 0.0f; out[1][i] = 0.0f; }
    }
    cpu.OnBlockEnd();
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(eliane::kBlock);
    const float sr = hw.AudioSampleRate();

    synth.Init(sr, combL, combR, System::GetTick());
    synth.SetHz(60.0f);
    synth.SetAmp(0.5f);
    cpu.Init(sr, eliane::kBlock);

    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    float hzTarget = 60.0f;
    uint32_t t0 = System::GetNow();
    while(1)
    {
        hw.ProcessAllControls();

        const int32_t inc = hw.encoder.Increment();
        if(inc != 0)
        {
            hzTarget *= std::pow(2.0f, float(inc) / 12.0f);
            synth.SetHz(hzTarget);
            hzTarget = synth.Hz(); // allinea ai limiti
        }
        synth.SetDensity(hw.knob1.Process());
        synth.SetRoughness(hw.knob2.Process());

        hw.led1.Set(0.0f, 0.3f, 0.0f);
        if(System::GetNow() - t0 > 500)
        {
            t0 = System::GetNow();
            const float load = cpu.GetAvgCpuLoad();
            if(load < 0.5f)      hw.led2.Set(0.0f, 0.3f, 0.0f);
            else if(load < 0.8f) hw.led2.Set(0.3f, 0.3f, 0.0f);
            else                 hw.led2.Set(0.3f, 0.0f, 0.0f);
        }
        hw.UpdateLeds();
        System::Delay(1);
    }
}

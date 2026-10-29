// Aeolian v1.0-a - Eliane su Daisy Pod (Seed inclusa nel Pod)
//
// Opera derivata da "Eliane" di @sixolet (dronecaster, GPLv3):
//   // @sixolet
//   // Eliane
//   // Feedback, slow beatings, highs and lows
//   // (seven sines phase-modulating each other in a crosslinked ring)
// Porting Daisy: progetto Aeolian, 2026-10-28. Licenza: GNU GPL v3.0 (vedi eliane.h).
//
// Controlli (v1.1): tutti e sei i parametri (hz, amp, densita', ruvidita', coda, battimenti)
// arrivano via USB MIDI CC da Max su canale 1 (CC 20-25, vedi switch sotto). I controlli
// fisici del Pod (POT1, POT2, encoder) sono disattivati: il Pod e' hardware di passaggio
// per questa fase del progetto, il mapping vive fuori dal Daisy (vedi sintesi-progetto.md).
// LED1: verde lampo breve ad ogni messaggio MIDI ricevuto, altrimenti spento (indicatore di
// ricezione, utile per capire al volo se la patch Max sta davvero parlando col Pod).
// LED2: carico CPU (verde < 50%, giallo < 80%, rosso oltre).
// Prerequisiti: libDaisy compilata, Daisy Toolchain. Nessun modulo DaisySP.
#include "daisy_pod.h"
#include "util/CpuLoadMeter.h"
#include "eliane.h"

using namespace daisy;

static DaisyPod       hw;
static eliane::Eliane synth;
static CpuLoadMeter   cpu;
static MidiUsbHandler midi; // MIDI su USB (hw.midi e' il MidiUartHandler integrato, TRS/UART, riservato a v2)

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

    MidiUsbHandler::Config midi_cfg;
    midi_cfg.transport_config.periph = MidiUsbTransport::Config::INTERNAL;
    midi.Init(midi_cfg);

    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    uint32_t t0 = System::GetNow();
    // Timestamp dell'ultimo messaggio MIDI ricevuto (qualsiasi tipo), per il lampo di LED1.
    // Inizializzato "nel passato remoto" (overflow unsigned voluto) per non accendersi al boot.
    uint32_t lastMidiMs = (uint32_t)-100000;
    while(1)
    {
        midi.Listen();
        while(midi.HasEvents())
        {
            auto msg = midi.PopEvent();
            lastMidiMs = System::GetNow();
            if(msg.type == ControlChange)
            {
                auto cc = msg.AsControlChange();
                if(cc.channel == 0) // canale MIDI 1 (0-indicizzato internamente)
                {
                    const float v = float(cc.value) / 127.0f;
                    switch(cc.control_number)
                    {
                        case 20: synth.SetHzNorm(v);    break; // hz
                        case 21: synth.SetAmp(v);       break; // amp
                        case 22: synth.SetDensity(v);   break; // densita'
                        case 23: synth.SetRoughness(v); break; // ruvidita'
                        case 24: synth.SetTail(v);      break; // coda
                        case 25: synth.SetBeat(v);      break; // battimenti
                        default: break;
                    }
                }
            }
        }

        if(System::GetNow() - lastMidiMs < 100) hw.led1.Set(0.0f, 0.3f, 0.0f);
        else                                    hw.led1.Set(0.0f, 0.0f, 0.0f);
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

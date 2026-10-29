# Aeolian — istruzioni di progetto per Claude Code

Aggiornato: 2026-10-29. Riferimento di stato esteso: `sintesi-progetto.md` (copiarlo nel repo accanto a questo file).

## Progetto
Device portatile a batteria per musica generativa (drone, stile Eno) guidata da dati esterni (API; in futuro bioritmo). Motore audio: Daisy Seed. Per la v1.0 si usa il Daisy Pod (Seed incluso, estraibile). Dati e mapping stanno fuori dal Daisy: v1 Max su Mac via MIDI CC USB, v2 micro con wifi via UART MIDI. Il mapping dato -> parametro vive in un file JSON, non nel firmware.

Obiettivo: primo strumento personale funzionante (v1). Replicabilità (firmware open-source + BOM/kit) da valutare solo dopo un checkpoint di ascolto con 4-5 persone. Bioritmo e PCB dedicata fuori dalla v1.

## Metodo
- Micro-step versionati (v1.0-a, v1.0-b, v1.1...), nessun refactor non richiesto, nessun over-engineering. Prima il suono, poi l'integrazione.
- Interventi chirurgici. Nessuna dipendenza nuova senza motivo.
- Distinguere sempre Daisy Seed da Daisy Pod quando si parla di hardware.
- Dichiarare ciò che è non verificato invece di darlo per certo. Marcare: verificato su fonte o prova, non verificato, fornito dall'utente.
- Replicabilità: segnalare impatti su licenze, certificazioni radio, dipendenze da API terze.
- Aggiornare `sintesi-progetto.md` quando cambiano direzione o decisioni.

## Stile di risposta (utente: Alessandro, sviluppatore)
- Task tecnici/debug: Problema / Causa / Soluzione / Codice / Nota critica. Conversazione e valutazioni: prosa, tono da collega.
- Codice completo ed eseguibile, zero placeholder. Commenti brevi sopra il blocco, che spiegano il perché.
- Niente emoji, firme, storytelling, suggerimenti non richiesti.
- Correzioni nette e motivate; errori ammessi senza giri di parole.

## Licenza e crediti
Il firmware è un porting di "Eliane" di @sixolet (da dronecaster, GPLv3): opera derivata, quindi GPLv3 se distribuita. Mantenere l'intestazione originale:
```
// @sixolet
// Eliane
// Feedback, slow beatings, highs and lows
// (seven sines phase-modulating each other in a crosslinked ring)
```
più la riga sul porting per Daisy e le modifiche datate. Nessun modulo DaisySP usato (evita la questione LGPL). libDaisy è MIT.

## Stato del codice
File alla radice del repo (non in una sottocartella `v1.0-a/`, nonostante il nome): `eliane.h` (DSP header-only, host-testabile), `main.cpp` (Pod), `Makefile`, `README.md`, `host_test.cpp` (render su PC). libDaisy come cartella sorella (`LIBDAISY_DIR ?= ../libDaisy`).

### v1.1 (chiuso 2026-10-29)
Mapping: tutti e sei i parametri via USB MIDI CC da Max, canale 1 — CC 20=hz, 21=amp, 22=densità, 23=ruvidità, 24=coda, 25=battimenti (tabella completa in `sintesi-progetto.md`). POT1/POT2/encoder disattivati: il Pod è hardware di passaggio, sostituito dal Seed su breadboard da v1.3 in poi. `amp` torna controllabile (non più fissa come in v1.0-b).

Implementazione: aggiunto `MidiUsbHandler` (periferica USB interna); hz mappato esponenzialmente (`SetHzNorm`, 10-1000 Hz); coda sposta il range min/max del decay del comb (0,1-0,5 s a coda minima, 1-8 s a coda massima); battimenti scala il fattore di deriva di frequenza per voce, con minimo non-zero 0,05 (deriva sempre percettibile, mai del tutto ferma — richiesta esplicita dell'utente). Slew dedicato (50 ms) su coda e battimenti.

**Causa di un bug lungo da trovare, per riferimento futuro**: dopo l'implementazione nessun CC arrivava al Daisy nonostante USB MIDI correttamente enumerato e Tx (Daisy->Max) funzionante. Non era il firmware (anche l'esempio ufficiale intatto non riceveva nulla), non era un permesso di sistema (Max riceveva da tre fonti diverse). La causa era in Max: gli oggetti `ctlout`/`noteout` non seguono le spunte "On" di MIDI Setup per scegliere la porta di destinazione — hanno un target esplicito, selezionabile con doppio click sull'oggetto in modalità performance (apre il menu reale delle porte, comprese quelle disattivate). L'oggetto puntava altrove. Lezione per Max in generale, non solo per questo progetto.

Verificato (2026-10-29, sul Pod): tutti e sei gli slider rispondono. hz/amp/densità/ruvidità già buoni; coda e battimenti confermati all'ascolto dall'utente. **v1.1 chiuso.**

### v1.0-b (chiuso 2026-10-29)
Mapping: encoder = hz (invariato), POT1 = densità, POT2 = ruvidità, `amp` fissa a 0,5 (il volume reale passa dal trimmer analogico del Pod sulle cuffie; sull'uscita AUX non c'è regolazione di volume in questo step — scelta consapevole dell'utente).

Implementazione: `kEnvOffset` (costante 0,2) diventato parametro `density_` in `[0,1]`, mappato su un offset `[-0,6, 0,9]` passato a `GainEnv::Process`; già smussato dal lag2(13 s) esistente, nessuno slew dedicato necessario. Il range fisso dell'esponente di modulazione (0,9-1,7) diventato `rough_` in `[0,1]`: estremo basso fisso a 0,9, estremo alto che scorre da 1,1 a 4,0; serve uno slew dedicato (tau 50 ms, come `amp`) perché agisce direttamente sulla profondità di modulazione per campione, senza altro filtro a valle.

Verificato (2026-10-29, su PC): nessun NaN su tutte le combinazioni estreme di densità/ruvidità, picco massimo 0,58. A densità minima il suono può restare silenzioso anche oltre 20-30 s prima di ricomparire (inviluppo lentissimo, non un guasto). Compila pulito per Cortex-M7: FLASH 82,9 KB (63%), SRAM 59,2 KB, SDRAM 115 KB.

Verificato anche su hardware (2026-10-29): flashato sul Pod, controlli reattivi e senza scatti, encoder cambia frequenza correttamente, estremi di densità/ruvidità giudicati coerenti all'ascolto, mai raggiunta saturazione/"panic". Range lasciati come da prima implementazione, nessuna modifica alle costanti richiesta.

### v1.0-a (chiuso 2026-10-29)

Verificato (2026-10-28):
- Il firmware compila e linka per Cortex-M7 (arm-none-eabi-gcc 13.2.1): FLASH 90,7 KB, SRAM 61,7 KB, SDRAM 115 KB.
- Il solo DSP, compilato su PC e renderizzato offline a 48 kHz (180 s a 60 Hz, 120 s con salto a 300 Hz): nessun NaN, picco 0,47, spettro coerente con l'originale.

Verificato anche su hardware (2026-10-29): audio presente su aux e cuffie, carico CPU 36-40% medio / 40-41% massimo. Non verificato: confronto d'ascolto con l'originale SC, stabilità su 30 minuti continui.

Comportamento atteso: il comb restituisce solo il segnale ritardato, quindi nei primi 0,3 s dall'avvio l'uscita è silenziosa. Poi l'audio deve comparire anche con le manopole ferme.

## Problema risolto (2026-10-29)
Causa: in `main.cpp` mancava `hw.StartAdc()`. `hw.Init()` configura i canali delle manopole ma non avvia la conversione ADC; senza quella chiamata `hw.knob1.Process()`/`knob2.Process()` restituivano sempre 0, quindi `amp` restava a 0 e il comb non riceveva mai energia nuova (uscita a zero dopo il transitorio iniziale). Verificato per confronto con 5 esempi Pod ufficiali di questo repository, tutti con `StartAdc()` prima di `StartAudio()`. Fix: aggiunta la chiamata. Confermato ad ascolto dall'utente il 2026-10-29.

Percorso di debug seguito (per riferimento, ipotesi scartate):
1. Carico CPU eccessivo: escluso, misurato 36-40% medio / 40-41% massimo con `CpuLoadMeter`, sotto la soglia gialla (50%).
2. Riavvii ripetuti: escluso con un contatore di boot in BACKUP_SRAM e log seriale (nessuna ripetizione di "boot #").
3. NaN/valori non finiti nel DSP: escluso con un contatore dedicato nella classe `Eliane` (sempre 0).
4. Causa reale: manopole sempre lette a 0 indipendentemente dalla posizione fisica -> ADC mai avviato.

Nota sul lampeggio dei LED (aggiornata 2026-10-29, durante il debug di v1.1): il sfarfallio rapido e simmetrico dei due LED, entrambi verdi, osservato nel primissimo tentativo e mai spiegato con certezza, in realtà non è mai sparito — l'utente conferma che era presente anche durante il test di stabilità di 30 minuti (audio confermato pulito per tutta la durata), solo non l'aveva segnalato perché l'attenzione era sull'ascolto. Probabilmente è un comportamento cosmetico del PWM software dei LED a bassa luminosità (`Set(0, 0.3, 0)`), non un sintomo di reset/crash: l'audio non ha mai avuto interruzioni mentre il lampeggio era presente. Non bloccante, non collegato al problema MIDI in corso di diagnosi in v1.1. Da approfondire solo se si deciderà di dare peso all'estetica dei LED in futuro.

**v1.0-b chiuso (2026-10-29).** Confermato dall'utente ad ascolto: controlli reattivi, encoder cambia frequenza, estremi di densità/ruvidità coerenti, mai raggiunto un punto di saturazione/"panic". Range considerati buoni così come sono, nessuna taratura richiesta.

## v1.0-a (chiuso 2026-10-29): checklist completata
1. ~~Rimuovere la strumentazione di debug~~ — fatto: `main.cpp`/`eliane.h` nel repo non contengono log seriale, bootCount, badCount né letture di debug.
2. ~~Ascolto A/B con l'originale (norns shield) a parità di hz~~ — fatto: confermato dall'utente il 2026-10-29. Non c'è aderenza 1:1 (porting SC -> C++), ma all'ascolto il carattere è simile e analogo: voci che si sovrappongono e battimenti che variano e si alternano in modo coerente con l'originale.
3. ~~30 minuti di funzionamento continuo senza click né glitch~~ — confermato dall'utente il 2026-10-29: 30' di esecuzione con solo il potenziometro del volume attivo, nessuna interruzione.

## Build e flash
```
cd v1.0-a
make clean && make
# Seed in DFU (BOOT premuto, poi RESET)
make program-dfu
```
Ambiente dell'utente: macOS, VS Code, Daisy Toolchain, libDaisy clonata con `--recurse-submodules` e compilata.

## Roadmap
| Step | Cosa | Stato |
|---|---|---|
| v0 | Toolchain e tono di prova | fatto dall'utente |
| v1.0-a | Eliane così com'è (hz, amp), block 64 | chiuso (2026-10-29): audio su hardware, 30 min senza glitch, ascolto A/B con l'originale (carattere simile, non aderenza 1:1) |
| v1.0-b | Densità e ruvidità sui due potenziometri, hz su encoder | chiuso (2026-10-29): confermato all'ascolto sul Pod, controlli reattivi e senza scatti, nessuna saturazione |
| v1.1 | Sei parametri via MIDI CC USB con slew | chiuso (2026-10-29): tutti gli slider rispondono, coda/battimenti confermati all'ascolto |
| v1.2 | Un dato reale + mapping in JSON | da fare |
| v1.3 | OLED su Seed in breadboard | da fare |
| v1.4 | LiPo + boost 5 V | da fare |
| v1.5 | Checkpoint di ascolto con 4-5 persone | da fare |

Fuori dalla v1: wifi a bordo, più sorgenti, preset, bioritmo, PCB, case.

# Aeolian — sintesi di progetto e roadmap v1 (documento v1.12, 2026-10-29)
 
Convenzione: [V] verificato su fonte (elencata in fondo) o con prova eseguita in sessione, [P] verificato in parte, [NV] non verificato, da controllare prima di contarci, [U] dichiarato o fornito dall'utente. Nessuna parte è stata provata su hardware: "verificato" significa confrontato con documentazione, sorgenti pubblici o compilazione/prova su PC, non testato al banco. Le note sulle licenze sono una lettura dei testi, non consulenza legale.
 
Cambiamenti rispetto a v1.5 del documento: l'utente ha approvato il passaggio a C++/libDaisy; scritto il porting v1.0-a di Eliane (eliane.h, main.cpp, Makefile, README); compilato per ARM (Cortex-M7) con libDaisy e provato il solo DSP su PC; verificate in sorgente le API usate (CpuLoadMeter, DSY_SDRAM_BSS, Pod); SelectX verificato equal power.

Cambiamenti v1.6 -> v1.7 (2026-10-29): risolto il bug "nessun audio sul Pod" apparso al primo test hardware. Causa verificata: in `main.cpp` mancava la chiamata `hw.StartAdc()`. `DaisyPod::Init()` configura i canali delle manopole (`InitKnobs()`) ma non avvia la conversione ADC; senza `StartAdc()`, `hw.knob1.Process()`/`hw.knob2.Process()` restituiscono sempre 0, quindi `amp` restava a 0 e nessuna energia nuova entrava nel comb (uscita a zero dopo il transitorio di avvio). Confermato per confronto con 5 esempi Pod ufficiali in questo stesso repository (ByteBeatOne, ChordMachine, Midi, MultiEffect, EuclideanDrums, SimpleLed), tutti con `StartAdc()` prima di `StartAudio()`. Percorso di debug seguito: test con sinusoide fissa (isolato l'hardware/Pod, ok) -> reintrodotto Eliane con log seriale USB (StartLog/PrintLine) e contatori di debug (bootCount in BACKUP_SRAM, badCount per i NaN, picco di uscita, valori letti dalle manopole) -> escluso il loop di riavvio (nessuna ripetizione di "boot #") -> escluso il sovraccarico CPU (36-40% medio, sotto la soglia del 50%) -> escluso il NaN (bad=0 sempre) -> isolato: knob1/knob2 restavano a 0/100 anche a manopola tutta girata -> causa trovata. Dopo il fix: manopola letta correttamente (es. 58/100), picco di uscita passato da 0 a ~0,3-0,37, utente confermato ad ascolto "ora lo sento bene".

Cambiamenti v1.7 -> v1.8 (2026-10-29): v1.0-a chiuso. Confermati dall'utente gli ultimi due criteri rimasti: 30 minuti di funzionamento continuo senza interruzioni (solo potenziometro volume attivo) e ascolto A/B con l'originale (norns shield) a parità di hz. Sull'A/B: non c'è aderenza 1:1 (è un porting da SC a C++, non una traduzione esatta campione per campione), ma al giudizio dell'utente il carattere è simile e analogo all'originale — le voci che si sovrappongono e i battimenti che variano e si alternano risultano coerenti. Prossimo passo: v1.0-b (densità e ruvidità sui due potenziometri, hz sull'encoder).

Cambiamenti v1.8 -> v1.9 (2026-10-29): v1.0-b implementato. Scoperto (foto dell'utente) un terzo controllo fisico sul Pod non documentato finora: un trimmer analogico sullo stadio di uscita cuffie, non collegato all'ADC del Daisy, che non ha alcun effetto sull'uscita AUX. Decisione operativa: POT1 -> densità, POT2 -> ruvidità, `amp` fissa a 0,5 in firmware (il volume reale passa dal trimmer solo su cuffie; su AUX resta fisso in questo step, scelta consapevole dell'utente). `kEnvOffset` (costante 0,2) diventato parametro densità in `[-0,6, 0,9]`, già smussato dal lag2(13s) esistente. Il range dell'esponente di modulazione (fisso 0,9-1,7) diventato parametro ruvidità: estremo basso fisso 0,9, estremo alto variabile `[1,1, 4,0]`, con slew dedicato (tau 50 ms) perché non passa per nessun filtro a valle. Verificato su PC: nessun NaN su tutte le combinazioni estreme, picco massimo 0,58; a densità minima il suono può restare silenzioso oltre 20-30 s (inviluppo lentissimo, non un guasto). Compila pulito per Cortex-M7 (FLASH 82,9 KB). Non verificato: comportamento sul Pod reale, taratura dei due range all'ascolto.

Cambiamenti v1.9 -> v1.10 (2026-10-29): v1.0-b chiuso. Confermato dall'utente ad ascolto sul Pod reale: controlli reattivi e senza scatti, encoder cambia frequenza, estremi di densità/ruvidità coerenti, mai raggiunta saturazione/"panic". Range lasciati invariati, nessuna taratura necessaria. Prossimo passo: v1.1 (sei parametri via MIDI CC da Max su USB, con slew nel firmware).

Cambiamenti v1.10 -> v1.11 (2026-10-29): v1.1 implementato e confermato operativo end-to-end. Aggiunto `MidiUsbHandler` (USB MIDI, periferica interna FS) in `main.cpp`; i sei parametri arrivano via CC MIDI canale 1 da Max (CC 20-25, tabella sopra). Controlli fisici del Pod (POT1, POT2, encoder) disattivati per questo step — scelta dell'utente, coerente con la natura temporanea del Pod nella roadmap (sostituito dal Seed su breadboard da v1.3). `amp` torna controllabile via CC (non più fissa come in v1.0-b).

Implementazione: hz mappato con curva esponenziale (`SetHzNorm`, 10-1000 Hz, stessa logica dei semitoni già usata sull'encoder). Coda: il range min/max del decay del comb (fisso 0,3-3 s nell'originale) diventa parametro mobile, da [0,1-0,5 s] a [1-8 s]. Battimenti: il fattore che pesa la deriva di frequenza per voce (fisso 0,5 nell'originale) diventa parametro con un minimo non-zero [0,05, 1,0] — su richiesta esplicita dell'utente, anche a battimenti minimi la deriva deve restare percettibile, mai completamente ferma. Entrambi con slew dedicato (50 ms) per evitare scatti.

Debug MIDI (sessione lunga, causa reale trovata): dopo l'implementazione, nessun CC da Max veniva ricevuto dal Daisy, nonostante enumerazione USB corretta (il Pod compariva in Max come "Daisy Seed Built In", input e output) e trasmissione Daisy->Max perfettamente funzionante (verificato con un test inverso: POT1 sul Pod mandava Note On e CC, letti correttamente in Max). Escluse in ordine: bug noto di CoreMIDI sui messaggi CC/Pitch Bend (non applicabile: fallivano anche i Note On); problema di timing nell'armare la ricezione USB (attesa di 500 ms aggiunta, nessun effetto); bug nel nostro codice (anche l'esempio ufficiale intatto `seed/USB_MIDI/USB_MIDI.cpp`, flashato senza modifiche, non riceveva nulla); blocco di permessi/sistema di macOS su Max (escluso: Max riceveva perfettamente da tre fonti indipendenti — Daisy, bus IAC virtuale, controller MIDI Mix esterno). Causa reale, trovata dall'utente: gli oggetti `ctlout`/`noteout` di Max non seguono le spunte "On" della finestra MIDI Setup per scegliere dove mandare i dati — hanno un proprio target di porta esplicito, selezionabile con un doppio click sull'oggetto in modalità performance (apre un menu con tutti i dispositivi, anche quelli disattivati in MIDI Setup). L'oggetto nella patch puntava a un'altra porta di default, non al Daisy. Nessuna modifica firmware necessaria per la causa reale; la strumentazione di debug aggiunta nel frattempo (LED diagnostici, test Tx inverso) è stata rimossa dal firmware finale, mantenendo solo un lampo di LED1 ad ogni CC ricevuto come indicatore di ricezione permanente.

Verificato (2026-10-29, sul Pod): tutti e sei gli slider rispondono in Max. hz, amp, densità, ruvidità già buoni (confermati in step precedenti). Non verificato: coda e battimenti, in valutazione all'ascolto da parte dell'utente — v1.1 non ancora chiuso.

Cambiamenti v1.11 -> v1.12 (2026-10-29): v1.1 chiuso. L'utente conferma all'ascolto anche coda e battimenti (gli ultimi due parametri rimasti da giudicare). Nessuna modifica ai range richiesta. Prossimo passo: v1.2 (un dato reale letto da Max, mapping dato -> parametro in un file JSON).
 
## Idea e obiettivo
Device portatile a batteria per musica generativa (drone / stile Eno) guidata da dati esterni (API; in futuro bioritmo). Prima strumento personale (v1); replicabilità (firmware open-source + BOM/kit) da valutare solo dopo un checkpoint di ascolto con 4-5 persone della cerchia sperimentale.
 
## Fatti verificati sull'hardware
| Voce | Stato | Dettaglio |
|---|---|---|
| Daisy Seed | V | Cortex-M7 480 MHz, 64 MB SDRAM, audio 96 kHz / 24 bit, 31 GPIO, SPI/UART/I2S/I2C, pin VIN dedicato |
| Audio del Seed: default | V | Sample rate 48 kHz e block size 48 di default; il block size si imposta con SetAudioBlockSize, limite superiore 128 |
| Uscita audio | V | Accoppiata in AC (un rivenditore lo indica nelle specifiche): le componenti sotto circa 20-30 Hz arrivano attenuate |
| Alimentazione Seed | V | VIN 4-17 V (datasheet); alimentabile da USB e VIN insieme senza danni (datasheet, forum Electro-Smith). Una LiPo (≈3,0-4,2 V) collegata diretta a VIN esce dal range: serve un boost |
| Radio sul Seed | P | Il datasheet non elenca alcuna radio; quindi il wifi richiede un modulo esterno |
| Daisy Pod | V | Include un Seed; 2 potenziometri, 1 encoder, 2 pulsanti, 2 LED RGB, MIDI in TRS, jack audio; alimentato via USB; ha "extra pinouts" per sensori e schermi OLED |
| Daisy Pod: trimmer volume | V (foto + osservazione dell'utente) | Oltre a POT1/POT2 e all'encoder, la scheda ha un terzo controllo: un trimmer a rotella vicino ai condensatori d'uscita, sullo stadio analogico di uscita. Non e' collegato all'ADC del Daisy, il firmware non lo legge ne' lo gestisce. **Agisce solo sull'uscita cuffie, non sull'uscita AUX**: sull'AUX il trimmer non ha alcun effetto, l'unico controllo di livello disponibile su quell'uscita e' `amp` via software |
| Pod: ADC liberi | P | Secondo il forum Electro-Smith, sul Pod restano liberi solo due ingressi analogici |
| Seed estraibile dal Pod | U | Confermato dall'utente a vista: non è saldato |
| MIDI UART | V | libDaisy gestisce MIDI su UART1; il Seed usa USART1 sui pin 13 e 14 (wiki Oopsy) |
| MIDI USB | P | Secondo un riscontro sul forum Electro-Smith il Seed è un device USB MIDI class-compliant; il supporto USB MIDI host è in libDaisy dalla 7.1.0. Plugdata ha l'opzione "USB MIDI" nell'export per Daisy |
| OLED | P | libDaisy ha driver della famiglia SSD130x (con trasporti SPI), SH1106 (aggiunto), SSD1307, SSD1327, SSD1351. Trasporto I2C per SSD1306: non visto nelle fonti controllate [NV] |
| Seed su breadboard | V | La wiki Electro-Smith lo descrive adatto a breadboard, moduli e pedali |
| Ambiente di sviluppo | V | Il databrief del Pod indica Daisy Toolchain e VS Code, su Windows, Linux 64 bit o macOS. Build e flash: make, make program-dfu con il Seed in modalità DFU |
| API libDaisy usate dal porting | V (sorgenti clonati) | daisy_pod.h: DaisyPod, SetAudioBlockSize, StartAudio, ProcessAllControls, encoder.Increment(), knob1.Process(), led1/led2.Set, UpdateLeds. src/util/CpuLoadMeter.h esiste (OnBlockStart / OnBlockEnd / GetAvgCpuLoad / GetMaxCpuLoad). DSY_SDRAM_BSS definito in src/dev/sdram.h |
| Compilazione ARM | V (prova eseguita) | libDaisy compilata e firmware v1.0-a linkato con arm-none-eabi-gcc 13.2.1 su Linux: FLASH 90.668 B su 128 KB (69%), SRAM 61.728 B, SDRAM 115.200 B. Solo i warning standard degli stub newlib. Non eseguito su hardware |
 
## Fatti verificati su toolchain e licenze
| Voce | Stato | Dettaglio |
|---|---|---|
| libDaisy | V | Licenza MIT |
| DaisySP | P | Licenza MIT, ma i moduli LGPL sono stati separati in DaisySP-LGPL (release 1.0.0), da abilitare con USE_DAISYSP_LGPL=1. Quali moduli siano oggi LGPL (riverbero incluso) e con quale versione di LGPL non è verificato [NV]: controllare l'intestazione di licenza di ogni modulo usato. Il porting v1.0-a non usa DaisySP |
| hvcc (Pd -> C++) | V | Compilatore in GPLv3; codice runtime "static" in ISC; i file "generated" portano un'intestazione di copyright modificabile con l'opzione --copyright |
| hvcc: oggetti supportati | V | Tra gli altri cos~, phasor~, noise~, osc~, delwrite~, delread~, send~, receive~. block~ è accettato ma ignorato (il block size lo decide l'hardware). tanh nella lista vanilla: non visto [NV] |
| Licenza dei file generati da hvcc | NV | La documentazione non dichiara una licenza per i file generati oltre all'intestazione: leggere LICENSE e chiedere nelle discussioni del progetto Wasted Audio prima di distribuire |
| Target Daisy di hvcc | V | Richiede libDaisy e json2daisy, entrambi MIT |
| Plugdata -> Daisy | V | Esporta C++, binario o flash diretto; hvcc supporta solo un sottoinsieme degli oggetti di Pd vanilla; la "Compiled Mode" segnala gli oggetti non compatibili. Il toolchain pesa circa 1,5 GB (dato di una versione precedente) |
| Max gen~ -> Daisy (Oopsy) | P | Max/gen~ è un linguaggio supportato per Daisy; Oopsy gestisce [param] da MIDI CC. Licenza del codice esportato da gen~: NV. Dipende da una licenza Max |
| SuperCollider: LocalIn/LocalOut | V | L'audio scritto con LocalOut è letto da LocalIn solo al ciclo successivo, cioè un blocco dopo; il block size di SC è di solito 64 campioni |
| SuperCollider: CombN | V | Coefficiente di feedback = 0.001^(delay/|decay|); con ritardo fisso è la scelta più efficiente senza perdita di qualità |
| SuperCollider: SelectX / XFade2 | V | Documentazione SC: crossfade equal power tra due canali adiacenti |
| dronecaster: progetto | V | Autore del progetto: tyleretters (organizzazione northern-information). Repository: github.com/northern-information/dronecaster. Catalogato su norns.community, con un invito a contribuire nuovi SynthDef |
| dronecaster: licenza | V (fonte fornita dall'utente) | GPL versione 3.0: lo indicano il pannello licenza di GitHub per northern-information/dronecaster (schermata dell'utente) e il testo LICENSE incollato. Il pannello riassume: uso commerciale, modifica, distribuzione, uso privato e uso dei brevetti consentiti; condizioni: avviso di licenza e copyright, indicazione delle modifiche, divulgazione del sorgente, stessa licenza; nessuna responsabilità né garanzia. Titolari del copyright non letti dal file (contiene solo il testo standard): punto chiuso per decisione dell'utente, non bloccante per la v1 |
| dronecaster: autori dei singoli drone | Chiuso (decisione dell'utente) | Prassi adottata: nel porting si copia l'intestazione originale del drone scelto e si accredita l'autore. Nessuna verifica ulteriore prima della v1 |
| SuperCollider su Daisy | NV | Non verificato; assumo di no (richiede un sistema operativo) |
 
## Porting di Eliane (@sixolet): valutazione e stato
Drone scelto dall'utente. Sorgente fornito dall'utente [U]. Intestazione copiata nei file del porting: "@sixolet / Eliane / Feedback, slow beatings, highs and lows / (seven sines phase-modulating each other in a crosslinked ring)", con riga sul porting per Daisy, riferimento a dronecaster/GPLv3 e modifiche datate 2026-10-28.
 
Struttura: 7 coppie stereo di oscillatori sinusoidali (rapporti in semitoni 0, 7, 14, 20, 27, 34, 41) in modulazione di fase reciproca in un anello incrociato, con feedback via LocalIn/LocalOut (14 canali); inviluppi di ampiezza casuali molto lenti (lag 13 s), rotazione stereo lenta, due oscillatori subarmonici crossfadati sulle voci 5 e 6 in funzione di hz, comb fisso da 0,3 s con tempo di decadimento casuale tra 0,3 e 3 s, tanh finale. Argomenti originali: solo hz (60) e amp (0,5): il resto evolve da solo.
 
| Elemento SC | Equivalente nel porting (come scritto) | Nota |
|---|---|---|
| SinOsc.ar(freq, phase) x 18 (14 dell'anello + 4 subarmonici) | accumulatore di fase + sinf(fase + modulazione) | nessun modulo DaisySP |
| LocalIn/LocalOut a 14 canali | doppio buffer: il blocco corrente scrive, il successivo legge; block size 64 | replica il ritardo di un blocco di SC [V]; 64 rientra nel limite di 128 [V] |
| LFNoise2.kr | classe Noise2: valori casuali, interpolazione quadratica (B-spline sui punti medi) | carattere simile, non l'algoritmo esatto di SC |
| LFDNoise3.kr | classe Noise3: frequenza variabile, interpolazione cubica | idem |
| lag2(13) | due one-pole in cascata a ritmo di blocco | parte dal primo valore, come Lag di SC (comportamento SC non riverificato) [NV] |
| Rotate2 | rotazione 2x2, seno e coseno a blocco | |
| SelectX (voci 5 e 6) | crossfade equal power con cos/sin | tipo di crossfade [V] |
| CombN (0,3 s, decay 0,3-3 s) | anello circolare di 14400 campioni per canale in SDRAM, solo segnale ritardato in uscita, g = 0.001^(0.3/decay) | formula [V]; comportamento "solo wet" ricavato dalla struttura del CombN, non riverificato sul sorgente C++ di SC [NV] |
| tanh | std::tanh | |
 
Punti di fedeltà rispettati: due insiemi indipendenti di inviluppi per voce (feedback e uscita); un unico generatore condiviso per l'esponente della profondità ("nice"); hz con slew di 1 s (tau); amp con slew di 50 ms; valori di controllo aggiornati per blocco come in SC.
 
Prove eseguite su PC (solo eliane.h, compilato con g++, rendering offline a 48 kHz): 180 s a hz = 60 e 120 s con salto da 60 a 300 Hz a metà. Risultati: nessun NaN, picco 0,47 (0,57 col salto), nessun campione oltre 0,99, DC trascurabile, correlazione L/R vicina a zero (immagine stereo ampia). Spettro a hz = 60 con picchi attorno a 30 Hz (subarmonica), 60, 90 e 135 Hz, coerente con rapporti 0, 7 e 14 semitoni e con la subarmonica a hz/2. RMS che varia da 0,05 a 0,18 su scala di decine di secondi (comportamento atteso degli inviluppi lenti). NON fatto: confronto d'ascolto con l'originale SC, esecuzione su Daisy, misura del carico CPU.
 
Costi: 18 sinusoidi per campione, circa 60 generatori lenti, un comb stereo. Il carico CPU non è misurato [NV]: il firmware accende il LED2 in base a CpuLoadMeter (verde < 50%, giallo < 80%, rosso oltre); da leggere al banco.
 
Parametri proposti per la v1 (da validare all'ascolto):
| # | Parametro | Dove sta nell'originale |
|---|---|---|
| 1 | hz (fondamentale) | argomento hz |
| 2 | amp | argomento amp |
| 3 | densità (quante parziali suonano insieme) | offset 0,2 degli inviluppi lenti |
| 4 | ruvidità (profondità di modulazione) | range dell'esponente 0,9-1,7 |
| 5 | coda (lunghezza del comb) | range del decay 0,3-3 s |
| 6 | battimenti (velocità) | fattore x/2 Hz sulla deriva |
Restano fissi: rotazione stereo, lag di 13 s. In v1.0-a erano esposti solo 1 e 2 (hz, amp); in v1.0-b si aggiungono 3 e 4 (densità, ruvidità) su POT1/POT2, amp passa fissa; in v1.1 tutti e sei arrivano via MIDI CC da Max (canale 1), POT1/POT2/encoder disattivati:

| CC | Parametro | Mappatura nel firmware |
|---|---|---|
| 20 | hz | esponenziale, 10-1000 Hz (0 = 10 Hz, 127 = 1000 Hz) |
| 21 | amp | lineare, 0-1 |
| 22 | densità | lineare su offset [-0,6, 0,9] |
| 23 | ruvidità | lineare su estremo alto dell'esponente [1,1, 4,0] (estremo basso fisso 0,9) |
| 24 | coda | lineare su range del decay, da [0,1-0,5 s] a [1-8 s] |
| 25 | battimenti | lineare su fattore di deriva [0,05, 1,0] (mai zero vero, deriva sempre percettibile) |
 
Note di ascolto: con hz = 60 le voci subarmoniche scendono a circa 30 e 20 Hz, e l'uscita del Seed è accoppiata in AC: sul Daisy parte di quel contenuto sarà attenuata rispetto all'originale su norns [V per l'accoppiamento AC, effetto da ascoltare].
 
## Conseguenze della GPLv3 sul porting (dal testo della licenza)
- Uso privato, senza distribuire: nessuna condizione (sez. 2). La v1 personale è libera.
- Un porting del drone (traduzione del codice SC in C++/Pd) è un adattamento, quindi una "work based on" l'originale (sez. 0).
- Se il firmware che lo contiene viene distribuito: l'intera opera va sotto GPLv3 (sez. 5c); vanno mantenuti gli avvisi di copyright (sez. 4), segnalate le modifiche con data (sez. 5a), reso disponibile il sorgente corrispondente anche per il binario (sez. 6). L'attribuzione diventa quindi un obbligo, non solo una cortesia.
- L'uso commerciale è consentito dalla GPLv3 (pannello GitHub): vendere un kit o un dispositivo non è escluso, purché si rispettino le condizioni sopra.
- Se si vende un oggetto di consumo con quel firmware dentro: la sez. 6 prevede di fornire le "Installation Information", cioè come installare firmware modificato sul dispositivo. Da progettare nel kit.
- Compatibilità con le altre dipendenze: MIT (libDaisy) e ISC (runtime hvcc) sono in genere compatibili con GPLv3 [P: conoscenza generale, non verificata in questa sessione].
- Programmi separati che comunicano via MIDI (es. Max con il mapping JSON): probabilmente non coperti, ma non è certo [NV].
## Radio e certificazioni (UE)
- Raspberry Pi Radio Module 2: rivenditori indicano certificazione modulare CE, FCC, IC, UKCA [P: fonti commerciali]. Pico W: risulta con conformità FCC/CE/RCM [P].
- ESP32-WROOM-32E: il datasheet indica certificazione RF FCC/CE-RED [V].
- Attenzione: un laboratorio di conformità sostiene che usare un modulo certificato non esonera dal valutare il dispositivo finito (le direttive CE si applicano all'intero apparecchio) [P: una sola fonte, che vende servizi di test]. Prima di vendere un oggetto con wifi, confrontarsi con un laboratorio o un consulente.
## Dipendenze da terzi
- API dati: non ancora scelta. Verificare termini d'uso, limiti e uso commerciale della scelta, al momento della scelta [NV].
- Batterie LiPo: la normativa di trasporto e vendita pesa solo se si distribuisce; non verificata.
## Decisioni prese
- Motore audio: Daisy Seed. Per la v1.0 si usa il Pod (controlli e jack già pronti), poi Seed su breadboard con OLED e batteria. Poiché il Seed si estrae dal Pod [U], lo stesso Seed può passare dal Pod alla breadboard.
- Dati e mapping: fuori dal Daisy. v1: Max su Mac, MIDI CC via USB. v2: micro con wifi nel case, MIDI CC via UART. Il mapping dato -> parametro vive in un file JSON di configurazione, non nel firmware.
- OSC: solo a monte (Max e dintorni). Ultimo tratto verso il Daisy: MIDI CC.
- Telefono: non fa parte di v1 né v2. BLE-MIDI e OSC non arrivano al Seed senza un ponte radio.
- Drone di base: Eliane (@sixolet), da dronecaster. Credito all'autore con l'intestazione copiata nel porting. Titolari del copyright e intestazioni: chiusi come non bloccanti per la v1; si riaprono solo se si decide di distribuire.
- Toolchain per la v1.0: C++ con libDaisy, senza moduli DaisySP (APPROVATO dall'utente il 2026-10-28). Motivo: la struttura di Eliane (anello di 7 voci identiche, feedback a blocchi, circa 60 generatori lenti) è molto più semplice in codice che in una patch visuale e non dipende da cosa supporti hvcc. Plugdata/hvcc resta per prototipi di altri suoni. Gen~/Oopsy resta alternativa personale ma legata a Max, quindi sconsigliata per un firmware replicabile.
- Layout del repository: cartella v1.0-a con eliane.h (DSP), main.cpp (Pod), Makefile, README, host_test.cpp; libDaisy come cartella sorella (LIBDAISY_DIR modificabile).
## Scelta aperta (da decidere prima di distribuire)
- Strada A: accettare GPLv3 per il firmware che contiene il porting. Coerente con l'ipotesi firmware open-source; il valore resta nel sound design, nel mapping e nel kit.
- Strada B: scrivere un drone originale da zero, senza tradurre il codice di dronecaster riga per riga, se si volesse una licenza diversa. Il confine tra adattamento e ispirazione è una zona grigia: in quel caso serve cautela, e un parere legale se il progetto diventa commerciale.
## Roadmap v1 (ogni step ha un criterio di "fatto")
| Step | Cosa | Hardware | Stato | Fatto quando |
|---|---|---|---|---|
| v0 | Daisy Toolchain e VS Code installati su macOS, firmware di prova (audio in uscita) caricato via USB (DFU) | Pod | da fare (da parte dell'utente) | Senti un tono dal jack |
| v1.0-a | Porting di Eliane così com'è (solo hz e amp), block size 64, intestazione di @sixolet copiata | Pod, alimentato USB | **Chiuso (2026-10-29).** Audio confermato su hardware (dopo fix `hw.StartAdc()`); carico CPU misurato 36-40% medio, 40-41% massimo (sotto soglia gialla 50%); 30 minuti di funzionamento continuo senza interruzioni confermati dall'utente (solo potenziometro volume attivo); ascolto A/B con l'originale confermato dall'utente: non aderenza 1:1 (porting SC -> C++), ma carattere simile e analogo — voci sovrapposte e battimenti coerenti con l'originale | Tutti i criteri soddisfatti |
| v1.0-b | Densità e ruvidità sui due potenziometri, hz sull'encoder | Pod | **Chiuso (2026-10-29).** Confermato all'ascolto sul Pod: controlli reattivi e senza scatti, encoder cambia frequenza, estremi di densità/ruvidità coerenti, mai raggiunta saturazione/"panic". Range lasciati come da prima implementazione | Tutti i criteri soddisfatti |
| v1.1 | Sei parametri via MIDI CC da Max su USB, con slew nel firmware | Pod + Mac | **Chiuso (2026-10-29).** Tutti gli slider rispondono in Max; hz/amp/densità/ruvidità già buoni; coda/battimenti confermati all'ascolto dall'utente | Tutti i criteri soddisfatti |
| v1.2 | Un dato reale (es. pressione barometrica da un'API scelta) letto da Max, mapping in file JSON | Pod + Mac | da fare | Tre sessioni da un'ora registrate; il mapping cambia solo modificando il JSON |
| v1.3 | OLED con sei barre e nome della sorgente | Seed su breadboard + OLED | da fare | Display fluido, nessun glitch audio. Prima di acquistare, controllare il chip dello schermo contro i driver di libDaisy |
| v1.4 | LiPo + boost 5 V (con protezione) | Seed su breadboard | da fare | Autonomia misurata; confronto A/B alla cieca con alimentazione USB per rumore del boost |
| v1.5 | Checkpoint di ascolto | Il tuo device | da fare | 4-5 persone dopo l'ascolto: se qualcuno chiede un device o il firmware, valutare kit/firmware open-source |
 
Fuori dalla v1: wifi a bordo, più sorgenti, preset e salvataggi, bioritmo, PCB, case.
 
## Dopo la v1 (solo se il checkpoint è positivo)
- v2: micro con wifi (modulo certificato) che legge le API e applica il JSON, MIDI CC via UART; provisioning wifi; risparmio energetico (wifi a intervalli). Autonomia da misurare.
- v3: bioritmo come nuova sorgente nello stesso mapping.
- Replicabilità: risolvere prima le voci [NV] di licenza (hvcc generato, se usato), riaprire i punti sui titolari e sul drone, decidere tra strada A e B, poi certificazione e termini API.
## Idee raccolte, non decise (da valutare più avanti, fuori roadmap v1)
- Mappare le variazioni oggi affidate ai potenziometri (densità, ruvidità, ...) su curve LFO invece che su un valore statico — scenario più evoluto, probabilmente dopo v1.1/v1.2 quando ci sono più parametri e il mapping JSON.
- Eliane è il drone di base scelto e approvato, ma all'ascolto prolungato (2026-10-29) l'utente lo descrive, di suo, come "a tratti ossessivo, disturbante e angosciante" — gli piace e lo predispone al focus, ma non è detto che valga per chiunque ascolti il device. Idea per il futuro: una ricerca umana (ascolto dell'utente) di altri drone da affiancare a Eliane, per stati d'animo o sorgenti di dati diversi. Non pianificata, nessun impegno di licenza aggiuntivo preso finché non si sceglie un drone specifico.

## Rischio principale
Scope troppo largo al primo passaggio (tre progetti precedenti arenati per questo). La v1 resta volutamente piccola.
 
## Fonti
- Daisy Seed datasheet v1.0.5: https://www.electrokit.com/upload/product/41020/41020659/Daisy_Seed_datasheet_v1.0.5.pdf
- Daisy Pod databrief: https://www.electrokit.com/upload/quick/fc/11/4c84_Pod_databrief.pdf
- Scheda Seed (AC-coupled, specifiche): https://www.electrokit.com/electrosmith-daisy-seed-embedded-dsp-platform
- Forum Electro-Smith, alimentazione: https://forum.electro-smith.com/t/seed-power-supply/292
- Forum, ADC liberi sul Pod: https://forum.electro-smith.com/t/gpio-for-pd2dsy/3475
- Forum, USB MIDI: https://forum.electro-smith.com/t/usb-capabilities-patch-sm-and-seed/4337/2
- Forum, block size (limite 128): https://forum.electro-smith.com/t/changing-audio-buffer-blocksize-of-the-daisy-seed/283
- Forum, default sample rate e block size: https://forum.electro-smith.com/t/set-new-sample-rate/346
- libDaisy (licenza, MIDI, API Pod, CpuLoadMeter, sdram.h): https://github.com/daisyaudio/libDaisy
- libDaisy release (SH1106, SSD1307, USB MIDI host): https://github.com/daisyaudio/libDaisy/releases
- DaisySP: https://github.com/electro-smith/daisysp
- DaisySP release 1.0.0 (split LGPL): https://github.com/electro-smith/DaisySP/releases
- hvcc, licenze: https://wasted-audio.github.io/hvcc/getting-started
- hvcc, Daisy e licenze dipendenze: https://wasted-audio.github.io/hvcc/0.17.0/getting-started/
- hvcc, oggetti Pd supportati: https://wasted-audio.github.io/hvcc/reference/objects/supported
- Plugdata, compilazione: https://plugdata.org/docs/book/CompilingPatches.html
- Oopsy, MIDI e custom JSON: https://github.com/electro-smith/DaisyWiki/wiki/How-To-Custom-JSON-with-Oopsy
- SuperCollider, LocalIn: https://docs.supercollider.online/Classes/LocalIn.html
- SuperCollider, CombN: https://docs.supercollider.online/Classes/CombN.html
- SuperCollider, SelectX: https://docs.supercollider.online/Classes/SelectX.html
- SuperCollider, XFade2: https://docs.supercollider.online/Classes/XFade2.html
- Raspberry Pi Radio Module 2: https://www.raspberrypi.com/documentation/microcontrollers/radio-modules.html
- ESP32-WROOM-32E, datasheet (certificazioni): https://static.rapidonline.com/pdf/09-1112_v1.pdf
- Certificazione CE di dispositivi con moduli Espressif: https://compliancetesting.com/?p=38763
- Glossario Daisy (Seed/Pod): https://github.com/electro-smith/DaisyWiki/wiki/Glossary
- dronecaster su norns.community (autore, repository): https://norns.community/dronecaster/
- Licenza di dronecaster: testo LICENSE e schermata del pannello licenza di GitHub (northern-information/dronecaster), entrambi forniti dall'utente
- Sorgente di Eliane (@sixolet): incollato dall'utente nella conversazione
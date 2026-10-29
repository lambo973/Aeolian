# Aeolian v1.0-a - Eliane su Daisy Pod

Porting C++ di "Eliane" di @sixolet (dronecaster per norns, GPLv3).
Originale: https://norns.community/dronecaster/

```
// @sixolet
// Eliane
// Feedback, slow beatings, highs and lows
// (seven sines phase-modulating each other in a crosslinked ring)
```

Licenza: GNU GPL v3.0 (opera derivata). Modifiche datate 2026-10-28.

## Contenuto

| File | Funzione |
|---|---|
| `eliane.h` | DSP header-only, senza dipendenze, compilabile anche su host |
| `main.cpp` | Glue per Daisy Pod (libDaisy) |
| `Makefile` | Build con Daisy Toolchain |
| `host_test.cpp` | Render offline su PC per verifiche (non va sul Daisy) |

## Build e flash

Prerequisiti: Daisy Toolchain, libDaisy clonata con `--recurse-submodules` e compilata (`make` nella sua cartella).
Layout atteso: `libDaisy/` accanto a `v1.0-a/` (oppure `make LIBDAISY_DIR=/percorso/libDaisy`).

```
cd v1.0-a
make
# Seed in modalita' DFU (BOOT premuto, poi RESET), USB collegata
make program-dfu
```

## Controlli

| Controllo | Funzione |
|---|---|
| Encoder | hz, un semitono per scatto, 10..1000 Hz, default 60, slew 1 s |
| Knob 1 | amp 0..1, default SC 0.5 |
| LED1 | verde = in esecuzione |
| LED2 | carico CPU: verde < 50%, giallo < 80%, rosso oltre |

## Differenze note rispetto all'originale SC

- LFNoise2 / LFDNoise3 sostituiti da generatori propri (interpolazione quadratica / cubica); stesso ordine di frequenze, non lo stesso algoritmo.
- LocalIn/LocalOut: ritardo di un blocco (64 campioni), come in SC con blocco 64.
- SelectX: equal power (documentazione SC), implementato con cos/sin.
- Valori di controllo (inviluppi, rumori) aggiornati per blocco, come in SC a control rate.

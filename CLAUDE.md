# Project context

Reads zone, arming and trouble state from a **DSC PC1555 (Power632)** alarm panel over
the Keybus using an **ESP32**, and exposes it as a serial stream and a self-hosted LAN
web page. No cloud, no broker, no subscription.

The panel sat unpowered for ~20 years and is currently **mid-diagnosis**. Read
`docs/panel-bringup.md` before touching anything — it is the running record of what has
been tested and ruled out.

## Hardware, confirmed from photos and measurement

| | |
|---|---|
| Panel | `PC1555 UA186 REV A`, firmware **V3.26**, manufactured wk37 **2006** |
| Family | PowerSeries (Keybus) — **not** Classic, so no `dscPC16Pin` |
| Keypads | `PC5508ZT` (8-zone LED) and a Ranger American rebrand, same layout |
| Zones in use | 5 of 8 — see zone map below |
| Battery | Original `CA1240` 12 V 4 Ah dead at 2.4 V; replacement on order |
| Powered devices | **None.** AUX and PGM terminals are empty — no PIRs on this system |

### Zone map (from the keypad label card)

| Zone | Label |
|---|---|
| 1 | Front Door / Rear Door (two contacts, one loop) |
| 2 | Master Bedroom |
| 3 | 2nd Floor |
| 4 | Rear Windows |
| 5 | Front Windows |
| 6–8 | unused |

## Current state

**The panel is electrically healthy but not functioning.**

Measured at the panel: `RED`–`BLK` **13.6 V**, `YEL`–`BLK` **3.95 V** steady (~29%
duty, clock running), `GRN`–`BLK` **6.15 V** (~45% duty, data flowing). Identical
readings at both keypads, so the house wiring is good.

But both keypads show **Trouble lit, Ready off, zero zone LEDs, and accept no
keystrokes**, with a ~1 Hz beep that `[#]` will not silence. Ready off with no zone
LEDs is internally inconsistent — no valid status is reaching them.

**Ruled out:** dead board · dead transformer · bad power supply · halted CPU · dead
clock · dead data line · house wiring · bus slot conflict · either keypad individually ·
every field connection (tested at transformer-plus-one-keypad).

**Remaining:** the panel's internal state — EEPROM contents, or a startup sequence that
never completes.

## Next actions, in order

1. **Install the replacement battery when it arrives** (procedure in
   `docs/panel-bringup.md` → "When the battery arrives"). DSC panels run a startup
   battery test; a panel stuck mid-init would clock, transmit, beep on a cycle, and
   never reach normal operation — which matches exactly.
2. **`pio run -e reader -t upload -t monitor`** with the tap wired. This is the
   diagnostic that matters: raw decoded Keybus traffic, which works even when the panel
   is misbehaving. Look for whether it repeats one command forever, what the command
   byte is, and whether modules are ever acknowledged.
3. **Factory default** if the battery doesn't help — jumper Z1 to PGM1 during power-up,
   no keypad input required. Also resets installer code to `5555`, master to `1234`,
   which solves the unknown-code problem. Verify against manual §5.28 first.
4. **Stopping rule:** if a good battery *and* a factory default both fail, stop. See
   the decision record at the end of `docs/panel-bringup.md`.

## Build targets

```
pio run -e reader -t upload -t monitor   # RAW Keybus dump — use this while diagnosing
pio run -e serial -t upload -t monitor   # decoded status over serial
pio run -e web    -t upload -t monitor   # self-hosted LAN page
```

Serial port on this Mac: `/dev/cu.usbserial-0001`, 115200 baud.

`pio device monitor` runs until interrupted — wrap it (`timeout 30 …`) when capturing
output non-interactively rather than blocking.

## Gotchas that will cost you time

- **Do not bump `platform = espressif32@6.9.0`.** dscKeybusInterface does not compile
  against arduino-esp32 3.x — the timer API changed. See upstream issue #344.
- **ESP32-WROOM-32 only.** The library supports esp32 and esp32-s2. Many boards sold as
  "ESP32" today are S3 or C3 and will not work.
- **ADC2 is unusable with WiFi active** — matters only for `docs/diy-zone-reader.md`,
  the fallback design. Use ADC1 (GPIO 32/33/34/35/36/39).
- **Solder the Keybus tap. Never breadboard it.** Intermittent contacts cause CRC errors
  that look exactly like a protocol fault.
- **Never disconnect a Keybus wire with the panel powered.**
- `KeybusReader` upstream is a `.ino`; `src/keybus_reader.cpp` adds `#include
  <Arduino.h>` and forward declarations so it builds as C++. Don't "fix" those.

## Docs

| File | What it covers |
|---|---|
| `docs/panel-bringup.md` | **Start here.** Staged diagnostics, all results, decision record |
| `docs/multimeter-basics.md` | Procedures P1–P4, written for a beginner; meter is a Southwire 21005N (auto-ranging, two jacks) |
| `docs/wiring.md` | Divider design, pin assignments, BOM |
| `docs/pc1555mx-programming.md` | Installer programming sections (filename says MX; the panel is a plain PC1555, but the manual sections match) |
| `docs/diy-zone-reader.md` | Fallback if the panel is unrecoverable — read zone loops directly, Konnected-style |
| `docs/build-guide.html` | Illustrated permanent install |

## Working style that has been useful here

Measure before replacing. Every hypothesis in this project has been cheap to test and
several confident-sounding ones were wrong — "both keypads lit means the CPU is running"
was wrong (keypads light on 12 V alone), and "a steady voltage means the line is idle"
was wrong (a constant-duty square wave averages to a steady number). State what a
reading would have to show to falsify a theory, then take that reading.

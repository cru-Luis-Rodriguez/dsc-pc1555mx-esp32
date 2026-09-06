/*
 * Milestone 1 — serial status stream
 *
 * Prints DSC PC1555MX status changes to the USB serial monitor.
 * Adapted from the dscKeybusInterface "Status" example (esp32).
 *
 * Build:  pio run -e serial -t upload -t monitor
 *
 * Wiring (see docs/wiring.md):
 *   DSC Yellow (clock) --33k-- GPIO 18, and 10k from GPIO 18 to GND
 *   DSC Green  (data)  --33k-- GPIO 19, and 10k from GPIO 19 to GND
 *   DSC Black  (Aux-)  ------- ESP32 GND
 *   DSC Red    (Aux+)  --- buck converter set to 5.0V --- ESP32 5V (VIN) pin
 *   Virtual keypad (optional): GPIO 21 --1k-- NPN base, emitter GND, collector to DSC Green
 */

#include <Arduino.h>
#include <dscKeybusInterface.h>

#define dscClockPin 18  // usable: 4, 13, 16-39
#define dscReadPin  19  // usable: 4, 13, 16-39
#define dscWritePin 21  // usable: 4, 13, 16-33

dscKeybusInterface dsc(dscClockPin, dscReadPin, dscWritePin);


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println(F("DSC PC1555MX Keybus interface starting..."));

  dsc.begin();
  Serial.println(F("Online. Waiting for Keybus data."));
  Serial.println(F("Type keys here to send them to the panel (needs the write transistor)."));
}


void loop() {

  // Serial input is written to the Keybus as a virtual keypad
  if (Serial.available() > 0) dsc.write(Serial.read());

  dsc.loop();

  if (!dsc.statusChanged) return;
  dsc.statusChanged = false;

  if (dsc.bufferOverflow) {
    Serial.println(F("Keybus buffer overflow"));
    dsc.bufferOverflow = false;
  }

  if (dsc.keybusChanged) {
    dsc.keybusChanged = false;
    Serial.println(dsc.keybusConnected ? F("Keybus connected") : F("Keybus disconnected"));
  }

  // --- Partition status (PC1555MX has one partition, but loop anyway) ---
  for (byte partition = 0; partition < dscPartitions; partition++) {

    if (dsc.disabledChanged[partition]) {
      dsc.disabledChanged[partition] = false;
      if (dsc.disabled[partition]) {
        Serial.print(F("Partition ")); Serial.print(partition + 1);
        Serial.println(F(": Disabled"));
      }
    }
    if (dsc.disabled[partition]) continue;

    if (dsc.readyChanged[partition]) {
      dsc.readyChanged[partition] = false;
      Serial.print(F("Partition ")); Serial.print(partition + 1);
      Serial.println(dsc.ready[partition] ? F(": Ready") : F(": Not ready"));
    }

    if (dsc.armedChanged[partition]) {
      if (dsc.armed[partition]) {
        Serial.print(F("Partition ")); Serial.print(partition + 1);
        Serial.print(F(": Armed "));
        if (dsc.armedAway[partition])      Serial.print(F("away"));
        else if (dsc.armedStay[partition]) Serial.print(F("stay"));
        if (dsc.noEntryDelay[partition]) Serial.println(F(" with no entry delay"));
        else Serial.println();
      }
      else {
        Serial.print(F("Partition ")); Serial.print(partition + 1);
        Serial.println(F(": Disarmed"));
      }
    }

    if (dsc.alarmChanged[partition]) {
      dsc.alarmChanged[partition] = false;
      if (dsc.alarm[partition]) {
        Serial.print(F("Partition ")); Serial.print(partition + 1);
        Serial.println(F(": ALARM"));
      }
      else if (!dsc.armedChanged[partition]) {
        Serial.print(F("Partition ")); Serial.print(partition + 1);
        Serial.println(F(": Disarmed"));
      }
    }
    if (dsc.armedChanged[partition]) dsc.armedChanged[partition] = false;

    if (dsc.exitDelayChanged[partition]) {
      dsc.exitDelayChanged[partition] = false;
      if (dsc.exitDelay[partition]) {
        Serial.print(F("Partition ")); Serial.print(partition + 1);
        Serial.println(F(": Exit delay in progress"));
      }
      else if (!dsc.armed[partition]) {
        Serial.print(F("Partition ")); Serial.print(partition + 1);
        Serial.println(F(": Disarmed"));
      }
    }

    if (dsc.entryDelayChanged[partition]) {
      dsc.entryDelayChanged[partition] = false;
      if (dsc.entryDelay[partition]) {
        Serial.print(F("Partition ")); Serial.print(partition + 1);
        Serial.println(F(": Entry delay in progress"));
      }
    }

    if (dsc.accessCodeChanged[partition]) {
      dsc.accessCodeChanged[partition] = false;
      Serial.print(F("Partition ")); Serial.print(partition + 1);
      Serial.print(dsc.accessCode[partition] == 40 ? F(": Master code ") : F(": Access code "));
      Serial.println(dsc.accessCode[partition]);
    }

    if (dsc.fireChanged[partition]) {
      dsc.fireChanged[partition] = false;
      Serial.print(F("Partition ")); Serial.print(partition + 1);
      Serial.println(dsc.fire[partition] ? F(": FIRE ALARM") : F(": Fire alarm restored"));
    }
  }

  // --- Zones ---
  // openZones[0] bit 0 = zone 1 ... bit 7 = zone 8; openZones[1] bit 0 = zone 9; etc.
  if (dsc.openZonesStatusChanged) {
    dsc.openZonesStatusChanged = false;
    for (byte zoneGroup = 0; zoneGroup < dscZones; zoneGroup++) {
      for (byte zoneBit = 0; zoneBit < 8; zoneBit++) {
        if (bitRead(dsc.openZonesChanged[zoneGroup], zoneBit)) {
          bitWrite(dsc.openZonesChanged[zoneGroup], zoneBit, 0);
          Serial.print(bitRead(dsc.openZones[zoneGroup], zoneBit) ? F("Zone opened: ")
                                                                 : F("Zone restored: "));
          Serial.println(zoneBit + 1 + (zoneGroup * 8));
        }
      }
    }
  }

  if (dsc.alarmZonesStatusChanged) {
    dsc.alarmZonesStatusChanged = false;
    for (byte zoneGroup = 0; zoneGroup < dscZones; zoneGroup++) {
      for (byte zoneBit = 0; zoneBit < 8; zoneBit++) {
        if (bitRead(dsc.alarmZonesChanged[zoneGroup], zoneBit)) {
          bitWrite(dsc.alarmZonesChanged[zoneGroup], zoneBit, 0);
          Serial.print(bitRead(dsc.alarmZones[zoneGroup], zoneBit) ? F("Zone ALARM: ")
                                                                  : F("Zone alarm restored: "));
          Serial.println(zoneBit + 1 + (zoneGroup * 8));
        }
      }
    }
  }

  // --- System ---
  if (dsc.troubleChanged) {
    dsc.troubleChanged = false;
    Serial.println(dsc.trouble ? F("Trouble status on") : F("Trouble status restored"));
  }

  if (dsc.powerChanged) {
    dsc.powerChanged = false;
    Serial.println(dsc.powerTrouble ? F("Panel AC power trouble") : F("Panel AC power restored"));
  }

  if (dsc.batteryChanged) {
    dsc.batteryChanged = false;
    Serial.println(dsc.batteryTrouble ? F("Panel battery trouble") : F("Panel battery restored"));
  }

  if (dsc.keypadFireAlarm)  { dsc.keypadFireAlarm  = false; Serial.println(F("Keypad Fire alarm")); }
  if (dsc.keypadAuxAlarm)   { dsc.keypadAuxAlarm   = false; Serial.println(F("Keypad Aux alarm")); }
  if (dsc.keypadPanicAlarm) { dsc.keypadPanicAlarm = false; Serial.println(F("Keypad Panic alarm")); }

  if (dsc.timestampChanged) {
    dsc.timestampChanged = false;
    Serial.print(F("Panel time: "));
    Serial.print(dsc.year); Serial.print('.');
    if (dsc.month < 10) Serial.print('0');
    Serial.print(dsc.month); Serial.print('.');
    if (dsc.day < 10) Serial.print('0');
    Serial.print(dsc.day); Serial.print(' ');
    if (dsc.hour < 10) Serial.print('0');
    Serial.print(dsc.hour); Serial.print(':');
    if (dsc.minute < 10) Serial.print('0');
    Serial.println(dsc.minute);
  }

  static bool versionPrinted = false;
  if (!versionPrinted && dsc.panelVersion != 0) {
    versionPrinted = true;
    Serial.print(F("Panel version: "));
    Serial.println(dsc.panelVersion);
  }
}

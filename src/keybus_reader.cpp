/*
 *  Keybus raw reader — diagnostic build target.
 *
 *  Adapted from the dscKeybusInterface "KeybusReader 1.3 (esp32)" example,
 *  which its author placed in the public domain:
 *  https://github.com/taligentx/dscKeybusInterface
 *
 *  Two changes from upstream, both required to build as a .cpp under
 *  PlatformIO rather than as an Arduino .ino:
 *    1. #include <Arduino.h>  — .ino files get this implicitly
 *    2. forward declarations for printModule() / printTimestamp(), which
 *       loop() calls before they are defined. The Arduino IDE generates
 *       these automatically; a plain C++ compiler does not.
 *
 *  Purpose here: print RAW decoded Keybus traffic. Unlike status_serial.cpp,
 *  this does not depend on the panel reaching a normal operating state, so it
 *  still tells you something when the panel is misbehaving. Use this first.
 *
 *  Build:  pio run -e reader -t upload -t monitor
 *
 *  Wiring (read-only, per docs/wiring.md):
 *      DSC BLK ----------------------------------- esp32 GND
 *
 *                                         +------- esp32 GPIO 18   (clock)
 *      DSC YEL --- 33k ohm resistor ------|
 *                                         +--- 10k ohm --- GND
 *
 *                                         +------- esp32 GPIO 19   (data)
 *      DSC GRN --- 33k ohm resistor ------|
 *                                         +--- 10k ohm --- GND
 *
 *  The esp32 is powered over USB for bench work. Nothing connects to DSC RED.
 *  dscWritePin is declared below but the transistor is NOT wired in this
 *  build, so the virtual keypad is inert — typing in the serial monitor does
 *  nothing. That is intentional: this target only listens.
 */

#include <Arduino.h>
#include <dscKeybusInterface.h>

#define dscClockPin 18  // 4,13,16-39
#define dscReadPin  19  // 4,13,16-39
#define dscWritePin 21  // 4,13,16-33  (not wired — read-only tap)

dscKeybusInterface dsc(dscClockPin, dscReadPin, dscWritePin);

// Forward declarations — see note 2 in the header comment.
void printModule();
void printTimestamp();


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println();

  dsc.hideKeypadDigits    = false;  // Hide keypad digits in publicly posted logs
  dsc.processModuleData   = true;   // Process and display keypad/module data
  dsc.displayTrailingBits = false;  // Bits read as the clock resets; usually spurious

  dsc.begin();
  Serial.println(F("DSC Keybus Interface is online."));
}


void loop() {

  // Virtual keypad write-back. Inert without the NPN transistor wired.
  if (Serial.available() > 0) dsc.write(Serial.read());

  if (dsc.loop()) {

    if (dsc.statusChanged) {
      dsc.statusChanged = false;

      if (dsc.keybusChanged) {
        dsc.keybusChanged = false;
        if (dsc.keybusConnected) Serial.println(F("Keybus connected"));
        else                     Serial.println(F("Keybus disconnected"));
      }
    }

    // Buffer exceeded: the sketch is too busy to process all Keybus commands.
    // Call loop() more often, or raise dscBufferSize in src/dscKeybus.h.
    if (dsc.bufferOverflow) {
      Serial.println(F("Keybus buffer overflow"));
      dsc.bufferOverflow = false;
    }

    if (dsc.keybusConnected) {
      printTimestamp();
      Serial.print(" ");
      dsc.printPanelBinary();
      Serial.print(" [");
      dsc.printPanelCommand();   // panel command, hex
      Serial.print("] ");
      dsc.printPanelMessage();   // decoded message
      Serial.println();

      if (dsc.handleModule()) printModule();
    }
  }

  else if (dsc.keybusConnected && dsc.handleModule()) printModule();
}


void printModule() {
  printTimestamp();
  Serial.print(" ");
  dsc.printModuleBinary();
  Serial.print(" ");
  dsc.printModuleMessage();
  Serial.println();
}


// Timestamp in seconds, 2dp — makes it obvious when the panel emits a burst
// of messages together in response to an event.
void printTimestamp() {
  float timeStamp = millis() / 1000.0;
  if      (timeStamp < 10)    Serial.print("    ");
  else if (timeStamp < 100)   Serial.print("   ");
  else if (timeStamp < 1000)  Serial.print("  ");
  else if (timeStamp < 10000) Serial.print(" ");
  Serial.print(timeStamp, 2);
  Serial.print(F(":"));
}

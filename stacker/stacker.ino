/* Sergey Mashchenko 2015, 2016
   RAMPS port D. Wolfenbarger 2020
   Fast Stacker: an automated macro rail for focus stacking

   Online tutorial: http://pulsar124.wikia.com/wiki/Fast_Stacker

   The hardware used:
   RAMPS/Mega/Full Graphic LCD
   Use Arduino IDE library manager to install u8g2lib before compiling
*/
#include <EEPROM.h>
#include <math.h>
#include <SPI.h>
#include "Keypad.h"
#include <U8g2lib.h>
#include "stacker.h"
#include "stdio.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void setup() {
  // Should be the first line in setup():
  g.setup_flag = 1;

  // Setting pins for Stepper Driver to OUTPUT:
#ifndef DISABLE_MOTOR
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_STEP, OUTPUT);
#endif
  pinMode(PIN_ENABLE, OUTPUT);
  digitalWrite(PIN_ENABLE, HIGH);
  pinMode(PIN_LIMITER_MIN, INPUT_PULLUP);
  pinMode(PIN_LIMITER_MAX, INPUT_PULLUP);
  pinMode(PIN_SHUTTER, OUTPUT);
  digitalWrite(PIN_SHUTTER, LOW);
  pinMode(PIN_AF, OUTPUT);
  digitalWrite(PIN_AF, LOW);
  pinMode(PIN_BATTERY, INPUT);
  analogReference(INTERNAL2V56); //use the internal 2.56V reference so battery voltage measurements not impacted by 5V variations

 //Some LCD setup
  lcd.begin();  // Always call lcd.begin() first.
  lcd.setFont(u8g2_font_t0_11_mr);
  lcd.setFontRefHeightExtendedText();
  lcd.setDrawColor(1);
  lcd.setFontPosTop();
  lcd.setFontDirection(0);
  lcd.setFontMode(0); //ensure background is overwritten

  // Checking if EEPROM was never used:
  if (EEPROM.read(0) == 255 && EEPROM.read(1) == 255)
  {
    // Initializing with a factory reset (setting EEPROM values to the initial state):
    initialize(1);
  }
  else
  {
    // Initializing, with EEPROM data read from the device
    initialize(0);
  }

  // This should not be done in initialize():
  keypad.key[0].kstate = (KeyState)0;
  keypad.key[1].kstate = (KeyState)0;

#ifdef ROUND_OFF
  // Rounding off small values of MM_PER_FRAME to the nearest whole number of microsteps:
  for (int i = 0; i < N_PARAMS; i++)
  {
    float fsteps = MM_PER_FRAME[i] / MM_PER_MICROSTEP;
    short steps = (short)nintMy(fsteps);
    if (steps < 20)
      MM_PER_FRAME[i] = ((float)steps) * MM_PER_MICROSTEP;
  }
#endif

    g.error=0;
  // Should be the last line in setup:
    g.setup_flag = 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void loop()
{

    // Performing backlash compensation after bad direction moves:
  backlash();

  // Display related regular activities:
  display_stuff();
   
  // Processing the keypad:
  process_keypad();

  // All the processing related to the two extreme limits for the macro rail movements:
  limiters();

  // Perform calibration of the limiters if requested (only when the rail is at rest):
  calibration();

  // Camera control:
  camera();

  // Issuing write to stepper motor driver pins if/when needed:
  motor_control();

#ifdef TIMING
  timing();
#endif

}

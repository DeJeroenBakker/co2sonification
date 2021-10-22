#include <Wire.h>
#include "Adafruit_SGP30.h"
#include <LiquidCrystal.h>
#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();
LiquidCrystal lcd(8,9,4,5,6,7);
Adafruit_SGP30 sgp;

/* return absolute humidity [mg/m^3] with approximation formula
* @param temperature [°C]
* @param humidity [%RH]
*/
uint32_t getAbsoluteHumidity(float temperature, float humidity) {
    // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
    const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
    const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
    return absoluteHumidityScaled;
}

void setup() {
  lcd.begin(16,2);
  lcd.setCursor(0,0);
  MIDI.begin();
  Serial.begin(115200);
  while (!Serial) { delay(10); } // Wait for serial console to open!

  Serial.println("SGP30 test");

  if (! sgp.begin()){
    Serial.println("Sensor not found :(");
    while (1);
  }
  Serial.print("Found SGP30 serial #");
  Serial.print(sgp.serialnumber[0], HEX);
  Serial.print(sgp.serialnumber[1], HEX);
  Serial.println(sgp.serialnumber[2], HEX);

  // If you have a baseline measurement from before you can assign it to start, to 'self-calibrate'
  //sgp.setIAQBaseline(0x8E68, 0x8F41);  // Will vary for each sensor!
}

int counter = 0;
void loop() {

  if (! sgp.IAQmeasure()) {
    Serial.println("Measurement failed");
    return;
  }

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("TVOC "); lcd.print(sgp.TVOC); lcd.print(" ppb");
  lcd.setCursor(0,1);
  lcd.print("eCO2 "); lcd.print(sgp.eCO2); lcd.print(" ppm");

//map eCO2 to MIDI values
int co2note = sgp.eCO2;
int co2vol = sgp.eCO2;
co2note = map(co2note, 400, 1400, 24, 100);
co2vol  = map(co2vol, 400, 1400, 80, 127);

//steady beat
MIDI.sendNoteOn(36, co2vol, 1);
delay(2500);
MIDI.sendNoteOff(36, 100, 1);

  if (sgp.eCO2 > 700) {
  MIDI.sendNoteOn(37, co2vol, 1);
  MIDI.sendNoteOn(36, co2vol, 2);
  MIDI.sendNoteOn(co2note, 110, 2);
  }
  
  if (sgp.eCO2 > 900) {
  lcd.setCursor(15,1);
  lcd.blink();
  MIDI.sendNoteOn(co2note, 127, 3);
  }
  else
  {lcd.noBlink();}

  if (! sgp.IAQmeasureRaw()) {
    lcd.println("Raw Measurement failed");
    return;
  }

  delay(2500);
  MIDI.sendNoteOff(co2note, 127, 3);
  MIDI.sendNoteOff(37, co2vol, 1);
  MIDI.sendNoteOff(36, co2vol, 2);
  MIDI.sendNoteOff(co2note, 110, 2);
  
  counter++;
  if (counter == 30) {
    counter = 0;

    uint16_t TVOC_base, eCO2_base;
    if (! sgp.getIAQBaseline(&eCO2_base, &TVOC_base)) {
      Serial.println("Failed to get baseline readings");
      return;
    }
    Serial.print("****Baseline values: eCO2: 0x"); Serial.print(eCO2_base, HEX);
    Serial.print(" & TVOC: 0x"); Serial.println(TVOC_base, HEX);
  }
}

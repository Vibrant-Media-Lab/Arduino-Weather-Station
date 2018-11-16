#include <LiquidCrystal_I2C.h>

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_Si7021.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

Adafruit_Si7021 ths = Adafruit_Si7021();
Adafruit_BMP280 tbps; // I2C
long startTime = 0;

void setup() {
  Serial.begin(115200);
  Serial.println(F("Joint T/P and T/Hu. test"));
  
  if (!tbps.begin()) {  
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1000);
  }

  //if (!ths.begin()) {
    //Serial.println("Did not find Si7021 Temperature/Humidity sensor!");
    //while (true);
  //}
  lcd.begin();

  // Turn on the blacklight and print a message.
  lcd.backlight();

  startTime = millis() ;
}

void loop() {
  Serial.print("Humidity:    "); Serial.print(ths.readHumidity(), 2);
  Serial.print("\tTemperature: "); Serial.println(ths.readTemperature(), 2);

  Serial.print("Temperature = ");
    Serial.print(tbps.readTemperature());
    Serial.println(" *C");
    
    Serial.print("Pressure = ");
    Serial.print(tbps.readPressure() /1000.0);
    Serial.println(" kPa");

    Serial.print("Approx altitude = ");
    Serial.print(tbps.readAltitude(1013.25)); // this should be adjusted to your local forcase
    Serial.println(" m");
    
    Serial.println();
    delay(2000);

    lcd.clear();
    lcd.print(millis() - startTime);

}

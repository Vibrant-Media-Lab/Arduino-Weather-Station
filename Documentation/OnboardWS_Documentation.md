# Unified Arduino Weather Station Documentation

## Dependencies/Inclusions

- LiquidCrystal_I2C.h
- Wire.h
- SPI.h
- math.h
- Adafruit_Sensor.h
- Adafruit_BMP280.h
- Adafruit_Si7021.h

## Components

### LCD Display
##### Model: ADAFRUIT HD44780
##### Pinouts:
- GND
- VCC
- SDA
- SCL

Display is delcared by:
```C++
LiquidCrystal_I2C lcd(0x27, 20, 4);
```
Where the arguments, in order, specify its memory location, the number of columns, and the number of rows on the display

### Sensors
#### System Temperature and Barometric Pressure
##### Model: ADAFRUIT BMP280
##### Onboard Reference Variable: tbps
##### Pinouts:
- VIN
- GND
- SCK
- SDI

tbps is declared by:
```C++
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

Adafruit_BMP280 tbps;
```

#### External Temperature and Relative Humidity
##### Model: ADAFRUIT Si7021
##### Onboard Reference Variable: ths
##### Pinouts:
- VIN
- GND
- SDA
- SCL

ths is declared by:
```C++
#include <Adafruit_Sensor.h>
#include <Adafruit_Si7021.h>

Adafruit_Si7021 ths = Adafruit_Si7021();
```


#### Anemometer and Wind Direction
##### Model: DAVIS ANEMOMETER FOR VANTAGE PRO2
##### Pinouts:
- Speed (BLK)
- Ground (RED)
- Direction (GRN)
- Power (YLW)

WIND is declared by:
```C++
#include <Wire.h>
#include <SPI.h>

#define windSpeedPin (2) //D2 is an interrupt pin reserved for determining wind speed
#define windDirectionPin (A4) //A4 is an analog pin used in determining wind direction

const int windBounceMin = 25; //Specifies the minimum amount of time, in milliseconds, that a wind cycle must take to be counted
#define windOffset (0); //Speciifes the clockwise angle, in degrees, between the support of the anemometer and the wind vane's resting direction
```

#### Rain Rate
##### DAVIS AEROCONE COLLECTOR FOR VANTAGE PRO2
##### Pinouts:
- Ground (RED)
- Rain Interrupt (GRN and YLW)

RAIN is declared by:
```C++
#include <Wire.h>
#include <SPI.h>

#define rainRatePin (18) //D18 is an interrupt pin reserved for rain rate calculations

const float volumePerRainTick = .00787401575; //Specifies the volume of water which will cause a tip in the rain sensor, should be calibrated
const int rainBounceMin = 500; //Specifies the minimum amount of time, in milliseconds, that a rain tick must wait after its predecessor to be counted
```

## Functions
### Setup

```C++
void prepareSensors()
```
 Determines whether boarded sensors are attached correctly to the main board, and properly specifies the pin setups for interrupt pin sensors, displaying pertinent information to the serial display.

```C++
void prepareLCD()
```
Initializes, clears, and backlights the LCD. To the user, this should appear as if the LCD turns off for about 2 seconds.

### Updates

```C++
void updateDisplay()
```
Sets the LCD to display the current screen, which cycles on a roughly 5 second timer. Default Screens include general data, rain trackers, and fire danger.

```C++
void updateData()
```
Collects current readings for the attached sensors. This function is called once during setup, then intermittently during running at a rate specified by dataUpdateDelay. It is the most time intensive of the processes run by this sketch, and may cause visul artifacts on the LCD display as it runs. This is due to the fact that it enables interrupts for a total of ten seconds, during which time other functions may not run properly.

```C++
void getFireSafetyRating()
```
Calculates the fire safety rating and ignition component based on the data collected this cycle. The fire safety rating is defined by the following:


###### < 10 - VERY LOW
###### < 25 - LOW
###### < 40 - MODERATE
###### < 70 - HIGH
###### > 70 - VERY HIGH

### Interrupt Functions

```C++
void rainTick()
```
Tick function for the rain sensor interrupt pin. When this function is called, it checks to see if the current time is long enough after the last tick time to count as a tick. If this is not true, the function returns. If it is, a tick is recorded and the proper volumes are added to both rain24Hrs and rainYear. The rain rate is then set to the frequency of the last two ticks, and converted to in/hr.

```C++
void windTick()
```
Tick function for the wind sensor interrupt pin. When this function is called, it checks to see if the current time is long enough after the last tick time to count as a tick. If this is not true, the function returns. If it is, a tick is recorded. After the read period ends, a separate function calculates the winds speed in mph based on the number of cycles in 5 seconds.

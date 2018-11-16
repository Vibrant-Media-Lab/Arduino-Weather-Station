#include <LiquidCrystal_I2C.h>

#include <Wire.h>
#include <SPI.h>

#include <math.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_Si7021.h>

/**
 * Combined Arduino Weather Station
 * Onboard Sketch
 * Version: 2.0.0
 * 
 * Joshua Arabia
 */

// DISPLAY
LiquidCrystal_I2C lcd(0x27, 20, 4); //Specifies the display to have 4 rows with 20 characters per row. These numbers should not be reduced.

// SENSORS
Adafruit_Si7021 ths = Adafruit_Si7021(); // Environtment Temperature and Humidity Sensor
Adafruit_BMP280 tbps; //System Temperature and Pressure Sensor
#define windSpeedPin (2) //D2 is an interrupt pin reserved for determining wind speed
#define windDirectionPin (A4) //A4 is an analog pin used in determining wind direction
#define rainRatePin (18) //D18 is an interrupt pin reserved for rain rate calculations

//TIMING
const unsigned long long startTime = millis();
unsigned long cycleStart;
unsigned long long currentTime;
const int displayChangeDelay = 5000; //Specifies the duration of each data sequence
const float dataUpdateDelay = 300000; //Specifies the time between data update calls
int timeUntilDisplayChange = displayChangeDelay;
float timeUntilDataUpdate = dataUpdateDelay;
unsigned int daysActive = 0;
unsigned int yearsActive = 0;

//DISPLAY
int currentScreen = 0;
const int displayScreens = 3; //Specifies the number of different display screens to cycle through

//EXTERIOR TEMPERATURE + HUMIDITY SENSOR
float exteriorTemp;
float humidity;

//SYSTEM TEMPERATURE + BAROMETRIC PRESSURE + ALTITUDE SENSOR
float sysTemp;
float pressure;
float altitude;

//WIND SPEED + WIND DIRECTION SENSOR
float windSpeed;
volatile unsigned long windCycleRotations;
volatile unsigned long lastWindTick;
const int windBounceMin = 25; //Specifies the minimum amount of time, in milliseconds, that a wind cycle must take to be counted

#define windOffset (0); //Speciifes the clockwise angle, in degrees, between the support of the anemometer and the wind vane's resting direction
int vaneValue;
int windDirection;
volatile int CalDirection = windDirection + windOffset;
int lastWindDirection;
String windHeading;

//RAIN RATE SENSOR
double rainRate;
const float volumePerRainTick = .00787401575; //Specifies the volume of water which will cause a tip in the rain sensor, should be calibrated
unsigned int rainTicksThisFrame = 0;
const int rainBounceMin = 500; //Specifies the minimum amount of time, in milliseconds, that a rain tick must wait after its predecessor to be counted
float lastRainTick;
float rain24Hrs;
float rainYear;

//FIRE SAFETY READING
String fireSafetyRating; //Categorizes the risk of a fire based on the ignition component
float ignitionComponent; //Approximates the likelihood of a fire igniting


void setup() {
  prepareSensors();
  prepareLCD();
  rollCredits();
  updateData();
  currentTime = millis();
}

void loop() {
  cycleStart = millis();

  if(timeUntilDataUpdate <= 0) {
    updateData();
    timeUntilDataUpdate = dataUpdateDelay;
  }
  
  if(timeUntilDisplayChange <= 0) {
    currentScreen++;
    currentScreen = currentScreen % displayScreens;
    updateDisplay();
    timeUntilDisplayChange = displayChangeDelay;
  }

 //Include functions here that should update daily
  if((currentTime / 86400000) > daysActive) {
    rain24Hrs = 0;
    daysActive++;
  }

 //Include functions here that should update yearly
  if(((currentTime / 86400000) / 365) > yearsActive) {
    rainYear = 0;
    yearsActive++;
  }
  
  timeUntilDataUpdate -= millis() - cycleStart;
  timeUntilDisplayChange -= millis() - cycleStart;
  currentTime += millis() - cycleStart;
}

/**
 * Determines whether boarded sensors are attached correctly to the main board, and properly specifies the pin setups for interrupt pin sensors
 */
void prepareSensors() {
  
  //Check the int. temp./pressure/altitude sensor for proper wirings
    if(!tbps.begin()) {
    Serial.println("Could not find a valid BMP280 sensor, check the following connections:\nVIN thru 10k pullup resistor to 5V\nGND to common ground\nSCL to D21\nSDA to D20\n");
    }
    
  //Check the ext. temp./humidity sensor for the proper wirings
    if(!ths.begin()) {
    Serial.println("Could not find a valid Si7021 sensor, check the following connections:\nVIN thru 10k pullup resistor to 5V\nGND to common ground\nSCL to D21\nSDA to D20\n");
    }

  //Properly specifies the pins to be used for rain ticks and wind ticks, and reserves them for interrupts
    pinMode(rainRatePin, INPUT_PULLUP);
    pinMode(windSpeedPin, INPUT_PULLUP); 
}

/**
 * Initializes, clears, and backlights the LCD. To the user, this should appear as if the LCD turns off for about 2 seconds.
 */
void prepareLCD() {
    lcd.init();
    lcd.init();
    lcd.clear();
    lcd.backlight();
}

/**
 * Collects current readings for the attached sensors. This function is called once during setup, then intermittently during running at a rate specified
 * by dataUpdateDelay. It is the most time intensive of the processes run by this sketch, and may cause visul artifacts on the LCD display as it runs.
 * This is due to the fact that it enables interrupts for a total of ten seconds, during which time other functions may not run properly.
 */
void updateData() {
  
  //Read data from the temperature and humidity sensor
    exteriorTemp = ths.readTemperature() * 1.8 + 32;
    humidity = ths.readHumidity();
    
  //Read data from the temperature and pressure sensor
    sysTemp = tbps.readTemperature() * 1.8 + 32;
    pressure = tbps.readPressure() / 1000;
    altitude = tbps.readAltitude(1013.25);

  //Enable interrupts for the rain guage, then wait 5 seconds to calculate a value
    lastRainTick = millis();
    attachInterrupt(digitalPinToInterrupt(rainRatePin),rainTick,FALLING);
    delay(5000);
    detachInterrupt(digitalPinToInterrupt(rainRatePin));

  //Enable interrupts for the wind speed sensor, then wait five seconds to calculate a value
    lastWindTick = millis();
    attachInterrupt(digitalPinToInterrupt(windSpeedPin),windTick,FALLING);
    delay(5000);
    detachInterrupt(digitalPinToInterrupt(windSpeedPin));
    windSpeed = windCycleRotations * (3600.0/8000.0);
    windCycleRotations = 0;

  //Based on these readings, calculate the ignition component and fire safety rating
    getFireSafetyRating();
    
}

/**
 * Tick function for the rain sensor interrupt pin. When this function is called, it checks to see if the current time is long enough after the last
 * tick time to count as a tick. If this is not true, the function returns. If it is, a tick is recorded and the proper volumes are added to both
 * rain24Hrs and rainYear. The rain rate is then set to the frequency of the last two ticks, and converted to in/hr.
 */
void rainTick() {
    if((millis() - lastRainTick) > rainBounceMin) {
      return;
    }
    
    rainRate = volumePerRainTick / ((millis() - lastRainTick) / 3600000);

    rain24Hrs += volumePerRainTick;
    rainYear += volumePerRainTick;
      
    lastRainTick = currentTime;
    
}

/**
 * Tick function for the wind sensor interrupt pin. When this function is called, it checks to see if the current time is long enough after the last
 * tick time to count as a tick. If this is not true, the function returns. If it is, a tick is recorded. After the read period ends, a separate
 * function calculates the winds speed in mph based on the number of cycles in 5 seconds.
 */
void windTick() {
  if((millis() - lastWindTick) > windBounceMin) {
    return;
  }

  windCycleRotations++;
  lastWindTick = currentTime;
}

/**
 * Calculates the fire safety rating and ignition component based on the data collected this cycle. The fire safety rating is defined by the following:
 * Ignition Component (%)    Rating
 *      <10                 VERY LOW
 *      <25                   LOW
 *      <40                 MODERATE
 *      <70                   HIGH
 *      >70                 VERY HIGH
 */
void getFireSafetyRating() {
  long rh = humidity / 100.0;
  long temp = exteriorTemp;
  long emc;
  if(rh < 10) {
    emc = .03229 + .281073*rh - .000578*rh*temp;
  } else if(rh < 50) {
    emc = 2.22749 + .160107*rh - .014784*temp;
  } else {
    emc = 21.0606 + .005565*(pow(rh,2)) - .00035*rh*temp - .483119*rh;
  }

 long mc1 = 1.03*emc;
 long qign = 144.5 - (.266*temp) - (.00058*(pow(temp,2))) - (.01*temp*mc1) + (18.54*(1.0 - exp(-.151*mc1)) + 6.4*mc1);
 long chi = (344.0 - qign) / 10.0;
 int P_I = round((3.6*chi*.0000185) - 100/.99767);
 int P_FI = round(sqrt(windSpeed*rh)/windSpeed) ;
 int IC = round(.1*P_I*P_FI);
 ignitionComponent = IC;
 
 if(IC < .1) {
  fireSafetyRating = "VERY LOW";
 } else if(IC < .25){
  fireSafetyRating = "LOW";
 } else if(IC < .4){
  fireSafetyRating = "MODERATE";
 } else if(IC < .7) {
  fireSafetyRating = "HIGH";
 } else {
  fireSafetyRating = "VERY HIGH";
 }
}

/**
 * Sets the LCD to display the current screen, which cycles on a roughly 5 second timer. Default Screens include general data, rain trackers, and fire danger,
 *
 */

 void updateDisplay() {
  
   lcd.clear();
   lcd.setCursor(0,0);

   switch(currentScreen) {
     //General display, with ext. temp., rel. humidity, rain rate, and wind speed/heading
      case 0:
        lcd.print("Temp: "); lcd.print(exteriorTemp); lcd.print(" F");
        lcd.setCursor(0,1);
        lcd.print("Humidity: "); lcd.print(humidity); lcd.print(" %");
        lcd.setCursor(0,2);
        lcd.print("Rain Rate: "); lcd.print(rainRate); lcd.print("in/hr");
        lcd.setCursor(0,3);
        lcd.print("Wind: "); lcd.print(windSpeed); lcd.print(" MPH "); lcd.print(windHeading);
        break;
        
     //Rain in last 24 hrs and last year
      case 1:
        lcd.print("RAIN IN LAST 24 HRS:"); lcd.setCursor(0,1); lcd.print(rain24Hrs);
        lcd.setCursor(0,2); lcd.print("RAIN IN LAST YEAR:"); lcd.setCursor(0,3); lcd.print(rainYear);
        break;
        
     //Fire safety and ignition component
      case 2:
        lcd.print("FIRE DANGER RATING:"); lcd.setCursor(0,1); lcd.print(fireSafetyRating);
        lcd.setCursor(0,2); lcd.print("IGNITION COMP.: "); lcd.setCursor(0,3); lcd.print(ignitionComponent); lcd.print("%");
        break;
        
      default:
        break;
   }
 }

 void rollCredits() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("UNIFIED WS PITT 2018");
  lcd.setCursor(0,1);
  lcd.print("JOSHUA ARABIA");
  lcd.setCursor(0,2);
  lcd.print("ALANA DEE");
  lcd.setCursor(0,3);
  lcd.print("LOGAN FRYE");
 }

# Unified-Arduino-Weather-Station
The Unified Arduino Weather Station is a self contained weather monitor that tracks metrics about its environment. The standard group of metrics includes:
  
    - Environmental Temperature
    - System Temperature
    - Relative Humidity
    - Barometric Pressure
    - Approximate Altitude
    - Wind Speed
    - Wind Heading
    - Rain Rate
    - Rain in Last 24 Hours
    - Rain in Last Year
    - Current Fire Danger Rating
    
## Arduino
The Weather Station is based on a single Arduino Mega board, which connects each of the attached sensors and writes their data using I2C communication protocol. The Arduino itself contains a version of the sketch `~/Arduino/OnboardWS_2/OnboardWS2.ino`, for which all necessary libraries are stored under `~/Arduino/libraries`. Test code for major sensors can be found in `~/Arduino/testCode`.

### OnboardWS_2
The second and most current version of the Weather Station's software is the sketch OnboardWS_2.ino., which is a 'C++' based implementation of the system's functionality. This _may_ recieve later update improving its funtionality, specifically adding the abilities to write data to a local SD card and upload data via an attached ethernet connection.



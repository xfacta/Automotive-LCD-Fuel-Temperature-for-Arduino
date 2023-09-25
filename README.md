# Automotive LCD Fuel Temperature for Arduino
## Fuel and temperature gauge with other warnings

- AT Mega2560 and 3.5" 320x480 LCD display
- Fuel Level barmeter with litres indication
- Temperature barmeter with degrees celcius indication
- Voltage indication
- Oil Pressure warning
- Fuel range warning
- Fan control
- Alternator warning, uses a 2nd voltage input
- Dim on low-beam
- Headlights indication
- Unified warning message area
- Shiftlight input from Tacho via serial
- Shiflight Neopixel LED output
- Last LED for status, or whole strip for oil pressure warning
- Offloaded sounds to external Leonardo Tiny
- Uses multimap instead of formulas for temperature and fuel level calibration
- Dummy analog reads discarded before real read to allow ADC to settle

- Using Nissan SR20 temperature gauge sensor by default (NOT the coolant sensor for the ECU)
  - DS18B20 onewire temperature sensor support commented out
  - normal NTC resistor sensor support commended out
  
- Fuel level input is set for Datsun 1600 standard sender
  - approx 8ohm to 80ohm in 45 litre tank
  - needs some input circuitry to give a good usable voltage swing


![Fuel-Temp](https://user-images.githubusercontent.com/41600026/235334865-11315358-4a72-44a8-93c0-82f2b89d5b6b.PNG)


### Uses 

This is a cheap ILI9481 480 x 320 LCD display that happens to suppot the portrait format I've chosen.
Not all displays do support portrait and there might be quite some rework of X Y coordinates to use a different display in landscape mode.

https://www.auselectronicsdirect.com.au/3.2-inch-lcd-screen-shield-for-arduino-mega


### It is required to check and adjust

```
int Fan_On_Hyst = 20000;         // msec Hysteresis, minimum run time of fan
const int Fan_On_Temp = 89;      // degrees C fan on
const int Alert_Temp = 98;       // degrees C alert level
const int Low_Temp = 71;         // degree C for low temperatures
const int Volts_Low = 133;       // low volts warning level x10
const int Volts_High = 144;      // high volts warning level x10
const bool Bad_Oil_Press = LOW;  // set whether the oil pressure sensor is Low or High for Bad
const bool Valid_Warning = LOW;  // set high or low for valid warnings to be passed to external processing
const bool Fan_On = HIGH;        // set high or low for operating the fan relay

const float vcc_ref = 4.92;  // measure the Arduino 5 volts DC and set it here
const float R1 = 1200.0;     // measure and set the voltage divider values
const float R2 = 3300.0;     // for accurate voltage measurements

int LED_Count = 8;  // set the length of the NeoPixel shiftlight strip
const int LED_Dim = 10;
const int LED_Bright = 80;
```
These may need modification if the temperature or fuel range changes significantly
```
const int Meter_Max = 360;
const int Meter_Min = 0;
```

The multimap calibration data must be gathered using Calibration mode, and raw values noted alongside corresponding real-world values
Example -
- Arduino analogRead values in temp_cal_in[]
- Measured temperature values in temp_cal_out[]
- remember to set the sample size value
```
// SR20 temp sender Celcius
const int temp_sample_size = 11;
int temp_cal_in[] = { 15, 20, 25, 30, 33, 44, 61, 85, 109, 198, 332 };
int temp_cal_out[] = { 120, 108, 100, 94, 90, 80, 70, 60, 53, 36, 22 };
```

The range of RPM on the neopixel strip is dictated by the serial output and settings on the Tachometer module (another Arduino)

### You can also set
- `Demo_Mode` = true or false for display of random values
- `Calibration_Mode` = true or false for display of some raw analogRead data
- `Debug_Mode` = true or false for serial output of some data

Pressing the button at any time also toggles calibration mode or normal mode.
Some debouncing in hardware is assumed.

The sounds are offloaded to a Leonardo Tiny to avoid delays and allow one Tiny and speaker to srvice multiple other functions such as Speedo and Fuel/Temperature/OilPressure gauge warning sounds.

The dimming function works by changing to using light grey and dark grey instead of white and colours, since there is no backlight control on the LCD panel I used.


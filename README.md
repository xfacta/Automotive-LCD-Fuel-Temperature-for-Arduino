# Automotive LCD Fuel Temperature for Arduino
## Fuel and temperature gauge with other warnings

- AT Mega2560 and 3.5" 320x480 LCD display
- Fuel Level barmeter with litres indication
- Temperature barmeter with degrees celcius indication
- Voltage indication
- Oil Pressure warning
- Fuel range warning and fan control
- Alternator warning, requires 2nd voltage input
- Dim on low-beam
- Headlights indication
- Unified warning message area
- RPM PWM input for shiftlight
- Shiflight Neopixel LED output
- Last LED for status, or whole strip for oil pressure warning
- Offloaded sounds to external Leonardo Tiny

- Using Nissan SR20 temperature gauge sensor by default (NOT the coolant sensor for the ECU)
  - DS18B20 onewire temperature sensor support commented out
  - normal NTC resistor sensor support commended out
  
- Fuel level input is set for Datsun 1600 standard sender
  - approx 8ohm to 80ohm in 45 litre tank
  - needs some input circuitry to give a good usable voltage swing

- Can use one analog pin for all headlight input via resistor ladder
  - commented out, but can be used if required
  - Parkers = 1.8v , Low Beam = 2.7v , High Beam = 4.2v


![Fuel-Temp](https://user-images.githubusercontent.com/41600026/235334865-11315358-4a72-44a8-93c0-82f2b89d5b6b.PNG)


### Uses 

This is a cheap ILI9481 480 x 320 LCD display that happens to suppot the portrait format I've chosen.
Not all displays do support portrait and there might be quite some rework of X Y coordinates to use a different display in lnadscape mode.

https://www.auselectronicsdirect.com.au/3.2-inch-lcd-screen-shield-for-arduino-mega


### It is required to check and adjust

```
Fan_On_Hyst = 20000;        // msec hysteresis, minimum time the fan will stay on
Fan_On_Temp = 89;           // degrees C fan on
Alert_Temp = 98;            // degrees C alert level
Warning_Fuel = 10;          // remaining fuel warning level
Volts_Low = 133;            // low volts warning level x10
Volts_High = 144;           // high volts warning level x10
Bad_Oil_Press = LOW;        // set whether the oil pressure sensor is Low or High for Bad

vcc_ref = 4.92;             // measure the 5 volts DC and set it here
R1 = 1200.0;                // measure and set the voltage divider values
R2 = 3300.0;                // for accurate voltage measurements

LED_Count = 8;              // set the length of the NeoPixel shiftlight strip
LED_Dim = 10;               // low brightness level
LED_Bright = 80;            // high brightness level
```

The range of RPM on the neopixel strip is dictated by the PWM output and settings on the RPM module (another Arduino)

### You can also set
- `Demo_Mode` = true or false for display of random values
- `Calibration_Mode` = true or false for display of some raw data like input frequency in Hz

Pressing the button at any time also toggles calibration mode or normal mode.
Some debouncing in hardware is assumed.

The sounds are offloaded to a Leonardo Tiny to avoid delays and allow one Tiny and speaker to srvice multiple other functions such as Speedo and Fuel/Temperature/OilPressure gauge warning sounds.

The dimming function works by changing to using light grey and dark grey instead of white and colours, since there is no backlight control on the LCD panel I used.


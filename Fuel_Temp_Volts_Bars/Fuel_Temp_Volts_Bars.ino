/*
  FuelLevel Temperature and Voltage barmeters
  including range warning and fan control
  Oil Pressure warning
  Alternator warning, requires 2nd voltage input
  Dim on low-beam
  Headlights indication
  unified warning message area
  RPM PWM input
  Shiflight Neopixel LED output
  Last LED for status
  DS18B20 temperature sensor support commented out
  NTC resistor support commented out
  Using Nissan SR20 temperature sensor by default
  Offloaded sounds to external Leonardo Tiny
  Uses multimap instead of formulas for temperature and fuel level
  Dummy analog reads discarded before real read to allow ADC to settle
*/


#define Version "Fuel Temp Bar V16"



//========================================================================
//========================== Set These Manually ==========================
//========================================================================

// set true for using Onewire DS18B20 temperature sensor
const bool Use_DS18B20 = true;

int Fan_On_Hyst = 20000;                 // msec Hysteresis, minimum run time of fan
const int Fan_On_Temp = 89;              // degrees C fan on
const int Alert_Temp = 98;               // degrees C alert level
const int Low_Temp = 71;                 // degree C for low temperatures
const int Volts_Low = 133;               // low volts warning level x10
const int Volts_High = 144;              // high volts warning level x10

const bool Valid_Warning = LOW;          // set high or low for valid warnings to be passed to external processing
const bool Fan_On = HIGH;                // set high or low for operating the fan relay
const bool Digitial_Input_Active = LOW;  // set whether digitial inputs are Low or High for active

const float vcc_ref = 4.92;  // measure the 5 volts DC and set it here
const float R1 = 1200.0;     // measure and set the voltage divider values
const float R2 = 3300.0;     // for accurate voltage measurements

// The range of RPM on the neopixel strip is dictated by the output from the RPM module
// set the length of the NeoPixel shiftlight strip
int LED_Count = 8;
const int LED_Dim = 10;
const int LED_Bright = 80;

//========================================================================



//========================================================================
//========================== Calibration mode ============================
//========================================================================

// Demo = true gives random speed values
// Calibration = true displays some calculated and raw values
// Pressing the button changes to Caibration mode
bool Calibration_Mode = false;
bool Demo_Mode = true;
bool Debug_Mode = false;

//========================================================================



//================== Multimap calibration table ==========================
/*
  Use calbration mode and measured real values to populate the arrays
  measure a real value and record matching the arduino calibration value

    output = Multimap<datatype>(input, inputArray, outputArray, size)

  Multimap includes a "-1" in the code with respect to the sample size
  Input array must have increasing values of the analog input
  Output array is the converted human readable value
  Output is constrained to the range of values in the arrays
*/

#include <MultiMap.h>

// Datsun fuel tank sender Litres
const int fuel_sample_size = 10;
int fuel_cal_in[] = { 42, 110, 151, 185, 199, 216, 226, 240, 247, 255 };
int fuel_cal_out[] = { 45, 40, 35, 30, 25, 20, 15, 20, 5, 0 };

// SR20 temp sender Celcius
const int temp_sample_size = 11;
int temp_cal_in[] = { 15, 20, 25, 30, 33, 44, 61, 85, 109, 198, 332 };
int temp_cal_out[] = { 120, 108, 100, 94, 90, 80, 70, 60, 53, 36, 20 };

// NTC temp sender Celcius
//const int temp_sample_size = 11;
//int temp_cal_in[] = { 89, 107, 128, 152, 177, 203, 228, 250, 269, 284, 296 };
//int temp_cal_out[] = { 120, 110, 100, 90, 80, 70, 60, 50, 40, 30, 20 };

//========================================================================


//================= Using DS8B20 temperature sesnor ======================
/*
  -10°C to +85°C ±0.5°C
  -30°C to +100°C ±1°C
  -55°C to +125°C ±2°C

  Pullup Resistor guide
    Length       5.0 Volt  3.3 Volt
    10cm (4")     10K0      6K8
    20cm (8")     8K2       4K7
    50cm (20")    4K7       3K3
    100cm (3'4")  3K3       2K2
    200cm (6'8")  2K2       1K0

  A result of -127 degrees indicates a sensor error

  The DS18B20_INT library is a minimalistic library for a DS18B20 sensor
  and will give only temperatures in whole degrees C
*/

// Comment out if not used to save memory
#include <OneWire.h>
#include <DS18B20_INT.h>
#define ONE_WIRE_BUS 14  // Specify the OneWire bus pin
OneWire oneWire(ONE_WIRE_BUS);
DS18B20_INT sensor(&oneWire);

//========================================================================


#include <Adafruit_NeoPixel.h>
#include <LCDWIKI_GUI.h>  //Core graphics library
#include <LCDWIKI_KBV.h>  //Hardware-specific library

// Meter colour schemes
#define RED2RED 0
#define GREEN2GREEN 1
#define BLUE2BLUE 2
#define BLUE2RED 3
#define GREEN2RED 4
#define RED2GREEN 5
#define RED2BLUE 6

#define METER_BLACK 0x0000       /*   0,   0,   0 */
#define METER_NAVY 0x000F        /*   0,   0, 128 */
#define METER_DARKGREEN 0x1684   /*   0, 128,   0 */
#define METER_DARKCYAN 0x03EF    /*   0, 128, 128 */
#define METER_MAROON 0x7800      /* 128,   0,   0 */
#define METER_PURPLE 0x780F      /* 128,   0, 128 */
#define METER_OLIVE 0x7BE0       /* 128, 128,   0 */
#define METER_LIGHTGREY 0xC618   /* 192, 192, 192 */
#define METER_DARKGREY 0x4A69    /*  80,  80,  80 */
#define METER_BLUE 0x001F        /*   0,   0, 255 */
#define METER_GREEN 0x07E0       /*   0, 255,   0 */
#define METER_CYAN 0x07FF        /*   0, 255, 255 */
#define METER_RED 0xF800         /* 255,   0,   0 */
#define METER_MAGENTA 0xF81F     /* 255,   0, 255 */
#define METER_YELLOW 0xFFE0      /* 255, 255,   0 */
#define METER_WHITE 0xFFFF       /* 255, 255, 255 */
#define METER_ORANGE 0xFD20      /* 255, 165,   0 */
#define METER_GREENYELLOW 0xAFE5 /* 173, 255,  47 */
#define METER_PINK 0xF81F
#define METER_GREY 0x2104  // Dark grey 16 bit colour

// Change some colours because backlight control is not available
bool Dim_Mode = false;
int Text_Colour1 = METER_WHITE;      // or VGA_SILVER
int Text_Colour2 = METER_LIGHTGREY;  // or VGA_GRAY
int Block_Fill_Colour = METER_GREY;  // or VGA_BLACK



/*
  analogReference(DEFAULT);
  DEFAULT: the default analog reference of 5 volts (on 5V Arduino boards) or 3.3 volts (on 3.3V Arduino boards)
  INTERNAL: a built-in reference, equal to 1.1 volts on the ATmega168 or ATmega328P and 2.56 volts on the ATmega32U4 and ATmega8 (not available on the Arduino Mega)
  INTERNAL1V1: a built-in 1.1V reference (Arduino Mega only)
  INTERNAL2V56: a built-in 2.56V reference (Arduino Mega only)
  EXTERNAL: the voltage applied to the AREF pin (0 to 5V only) is used as the reference
  #define aref_voltage 4.096
*/

// Voltage calculations
/*
   Read the Analog Input
   adc_value = analogRead(ANALOG_IN_PIN);

   Determine voltage at ADC input
   adc_voltage  = (adc_value+0.5) * ref_voltage / 1024.0 / (voltage divider)

   Calculate voltage at divider input
   in_voltage = adc_voltage / (R2/(R1+R2));

   R1 = 3300
   R2 = 1200
   vref = default = ~5.0 volts
   Vin = analogRead(ANALOG_IN_PIN) * 5 / 1024 / (3300/(3300+1200))
   Vin = analogRead(ANALOG_IN_PIN) * 0.00665838
*/
const float Input_Multiplier = vcc_ref / 1024.0 / (R2 / (R1 + R2));

// Common pin definitions
#define SD_Select 53

// Pin definitions for digital inputs
#define Oil_Press_Pin 0     // Oil pressure digital input pin
#define Parker_Light_Pin 1  // Parker lights digital input pin
#define Low_Beam_Pin 2      // Low beam digital input pin
#define High_Beam_Pin 3     // High beam digital input pin
#define Pbrake_Input_Pin 4  // Park brake input pin
#define VSS_Input_Pin 5     // Speed frequency input pin
#define RPM_Input_Pin 6     // RPM frequency INPUT pin
#define Button_Pin 7        // Button momentary input
#define RPM_PWM_In_Pin 8    // Input PWM signal representing RPM

// Pin definitions for analog inputs
#define Temp_Pin A0          // Temperature analog input pin - OneWire sensor on pin 14
#define Fuel_Pin A1          // Fuel level analog input pin
#define Batt_Volt_Pin A2     // Voltage analog input pin
#define Alternator_Pin A3    // Alternator indicator analog input pin
#define Head_Light_Input A4  // Headlights via resistor ladder

// Pin definitions for outputs
#define RPM_PWM_Out_Pin 9  // Output of RPM as a PWM signal for shift light
#define LED_Pin 10         // NeoPixel LED pin
#define Warning_Pin 11     // Link to external Leonardo for general warning sounds
#define OP_Warning_Pin 12  // Link to external Leonardo for oil pressure warning sound
#define Relay_Pin 13       // Relay for fan control

// Define the NeoPixel strip object:
Adafruit_NeoPixel strip(LED_Count, LED_Pin, NEO_GRB + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

// Define the TFT LCD module
LCDWIKI_KBV my_lcd(ILI9481, 40, 38, 39, -1, 41);  //model,cs,cd,wr,rd,reset

// Input variables
int Temp_Celsius, Fuel_Litres, Last_Fuel_Litres;
int Battery_Volts, Alternator_Volts;
int Raw_Battery_Volts, Raw_Alternator_Volts;
int Raw_Value, Dummy;

// Headligh Indicator status
// variables for state machine
// and set startup values
const int Lights_NotChanged = 0;
const int Lights_Off = 1;
const int Lights_Park = 2;
const int Lights_Low = 3;
const int Lights_High = 4;
int Light_Status = Lights_Off;
int Light_Last_Status = Lights_High;

// Event timing and status
const uint32_t Long_Loop_Interval = 4000;   // 4 seconds between linear bar meter updates
const uint32_t Short_Loop_Interval = 1000;  // 1 second between more important loops
uint32_t Long_Loop_Time, Short_Loop_Time;
uint32_t Fan_On_Time, Status_Change_Time;
bool Startup_Mode = true;
bool Missing_DS18B20 = false;

// Position of entire display area
const int LCD_Offset_X = 40;
const int LCD_Offset_Y = 50;

// Position of Headlight status
const int Light_Status_X = LCD_Offset_X + 25;
const int Light_Status_Y = LCD_Offset_Y + 0;
const int Light_Status_Length = 180;
const int Light_Status_Height = 50;

// Position of Status text
const int Status_Text_X = LCD_Offset_X + 22;
const int Status_Text_Y = LCD_Offset_Y + 300;
String Status_Text = "ComingReadyOrNot";
int Status_Priority = 0;

// Meter variables
// bar X and Y are from bottom left of each bar
const int Fuel_Bar_X = LCD_Offset_X + 40, Fuel_Bar_Y = LCD_Offset_Y + 280;
const int Temp_Bar_X = LCD_Offset_X + 135, Temp_Bar_Y = LCD_Offset_Y + 280;
const int Bar_Height = 200;
const int Bar_Width = 60;
int new_val;
String the_string;
uint16_t Warn_Text_Colour = METER_WHITE;

// get valid ranges to scale for the bar meter
const int Fuel_Min = fuel_cal_out[fuel_sample_size - 1];
const int Fuel_Max = fuel_cal_out[0];
const int Temp_Min = temp_cal_out[temp_sample_size - 1];
const int Temp_Max = temp_cal_out[0];

// Low fuel warning level
const int Warning_Fuel = (Fuel_Max - Fuel_Min) / 4;

// **********************************************************************************
// These may need modification if the temperature or fuel range changes significantly
const int Meter_Max = 360;
const int Meter_Min = 0;
// **********************************************************************************
// Using the same sized barmeters for two different readings
/*
  Example:
  fuel range 0 - 45
  temp range 30 - 120
  Lowest common multiplier = 360(max)
  map(value, fromLow, fromHigh, toLow, toHigh)
  Fuel:
  new_val = map(value, 0, 45, 0, 360)
  Temp:
  new_val = map(value, 20, 120, 0, 360)
*/

// NeoPixel shiftlight variables
int PWM_high, PWM_low, PWM_duty, LED_pos;
// For demo PWM values
int Old_PWM_duty;
bool Count_Up = true;

// Create a colour scheme for number of LEDs
// with off (0) as the first value
// INVERSE HSV blue/light blue/white
// uint32_t LED_Colour[] = {0x000000, 0x0000FF, 0x284BFF, 0x508AFF, 0x78BBFF, 0xA0DFFF, 0xC8F5FF, 0xF0FFFF, 0xFFFFFF};
// RGB red/orange/yellow/white
uint32_t LED_Colour[] = { 0x000000, 0xE64C00, 0xF27500, 0xFA9B00, 0xFFBF00, 0xFFD470, 0xFFE9B8, 0xFFFFFF, 0xFFFFFF };
// INVERSE HSV green/yellow/white
// uint32_t LED_Colour[] = {0x000000, 0x00FF00, 0x05FFD6, 0x0A59FF, 0x8A0FFF, 0xFF14AC, 0xFF4519, 0xF8FF1F, 0xFFFFFF};


// ##################################################################################################################################################


void setup() {



  if (Debug_Mode) Serial.begin(9600);

  analogReference(DEFAULT);

  //Improved randomness for testing
  unsigned long seed = 0, count = 32;
  while (--count)
    seed = (seed << 1) | (analogRead(A6) & 1);
  randomSeed(seed);

  // INITIALIZE NeoPixel strip
  // before initialising the LCD panel
  strip.begin();
  strip.clear();
  strip.setBrightness(LED_Dim);
  strip.show();  // Turn OFF all pixels ASAP

  // Outputs
  pinMode(Relay_Pin, OUTPUT);
  digitalWrite(Relay_Pin, !Fan_On);  // ensure fan relay is OFF at least until temperature is measured
  pinMode(Warning_Pin, OUTPUT);
  digitalWrite(Warning_Pin, !Valid_Warning);
  pinMode(OP_Warning_Pin, OUTPUT);
  digitalWrite(OP_Warning_Pin, !Valid_Warning);

  // Digital inputs
  pinMode(Oil_Press_Pin, INPUT_PULLUP);
  pinMode(Parker_Light_Pin, INPUT_PULLUP);
  pinMode(Low_Beam_Pin, INPUT_PULLUP);
  pinMode(High_Beam_Pin, INPUT_PULLUP);
  pinMode(Button_Pin, INPUT_PULLUP);
  pinMode(RPM_PWM_In_Pin, INPUT);

  // Analog inputs
  pinMode(Temp_Pin, INPUT);
  pinMode(Fuel_Pin, INPUT);
  pinMode(Batt_Volt_Pin, INPUT);
  pinMode(Alternator_Pin, INPUT);

  // Display startup text
  my_lcd.Init_LCD();
  my_lcd.Set_Rotation(4);
  my_lcd.Fill_Screen(METER_BLACK);
  my_lcd.Set_Text_Back_colour(METER_BLACK);
  my_lcd.Set_Text_colour(METER_LIGHTGREY);
  my_lcd.Set_Text_Size(2);
  my_lcd.Print_String(Version, CENTER, LCD_Offset_Y + 160);
  delay(2000);

  // Using DS8B20 for temperature
  if (Use_DS18B20) {
    if (!sensor.begin()) {
      Missing_DS18B20 = true;
      // Display warning for missing DS8B20 sensor
      my_lcd.Fill_Screen(METER_BLACK);
      my_lcd.Set_Text_colour(METER_YELLOW);
      my_lcd.Set_Text_Size(4);
      my_lcd.Print_String("MISSING", CENTER, LCD_Offset_Y + 100);
      my_lcd.Print_String("TEMP", CENTER, LCD_Offset_Y + 140);
      my_lcd.Print_String("SENSOR", CENTER, LCD_Offset_Y + 180);
      delay(10000);
    } else Missing_DS18B20 = false;
  }

  // Display static text
  my_lcd.Fill_Screen(METER_BLACK);
  my_lcd.Set_Text_Back_colour(METER_BLACK);
  my_lcd.Set_Text_Size(3);
  my_lcd.Set_Text_colour(METER_WHITE);
  my_lcd.Print_String("F", Fuel_Bar_X - 25, Fuel_Bar_Y - (Bar_Height / 2) - 40);
  my_lcd.Print_String("T", Temp_Bar_X + Bar_Width + 10, Temp_Bar_Y - (Bar_Height / 2) - 40);

  // Cylon eye effect during startup
  strip.setBrightness(LED_Bright);
  for (int i = 0; i < LED_Count + 3; i++) {
    delay(50);
    //strip.setPixelColor(i + 1, strip.Color(24, 0, 0));
    strip.setPixelColor(i, strip.Color(255, 0, 0));
    strip.setPixelColor(i - 1, strip.Color(50, 0, 0));
    strip.setPixelColor(i - 2, strip.Color(12, 0, 0));
    strip.setPixelColor(i - 3, strip.Color(0, 0, 0));
    strip.show();
  }
  for (int i = LED_Count + 3; i > -4; i--) {
    //strip.setPixelColor(i - 1, strip.Color(24, 0, 0));
    strip.setPixelColor(i, strip.Color(255, 0, 0));
    strip.setPixelColor(i + 1, strip.Color(50, 0, 0));
    strip.setPixelColor(i + 2, strip.Color(12, 0, 0));
    strip.setPixelColor(i + 3, strip.Color(0, 0, 0));
    strip.show();
    delay(50);
  }
  strip.clear();
  // actual number of LEDs is 0 to LED_Count
  LED_Count = LED_Count - 1;

  // Check - cant have both modes at once
  if (Calibration_Mode) Demo_Mode = false;
  if (Demo_Mode) Fan_On_Hyst = Fan_On_Hyst / 3;

  if (Debug_Mode) {
    Serial.print("Fuel_Min ");
    Serial.println(Fuel_Min);
    Serial.print("Fuel_Max ");
    Serial.println(Fuel_Max);
    Serial.print("Temp_Min ");
    Serial.println(Temp_Min);
    Serial.print("Temp_Max ");
    Serial.println(Temp_Max);
    Serial.print("Warning_Fuel ");
    Serial.println(Warning_Fuel);
    Serial.print("Meter_Max ");
    Serial.println(Meter_Max);
    Serial.print("Meter_Min ");
    Serial.println(Meter_Min);
    Serial.print("Fan_On_Hyst  ");
    Serial.println(Fan_On_Hyst);
  }


  Long_Loop_Time = millis();
  Short_Loop_Time = millis();


}  // end void setup


// ##################################################################################################################################################
// ##################################################################################################################################################


void loop() {


  // =======================================================
  // Always do these every loop
  // =======================================================

  Check_Oil();
  if (!Demo_Mode) ShiftLight_Strip();


  // =======================================================
  // Do these items at a short interval
  // =======================================================

  if ((millis() >= (Short_Loop_Time + Short_Loop_Interval)) || Startup_Mode == true) {
    Headlight_Status();
    Display_Warning_Text();
    Check_Button();
    if (Demo_Mode) ShiftLight_Strip();

    Short_Loop_Time = millis();
  }  // end short loop


  // =======================================================
  // Do these items at a long interval
  // =======================================================

  if ((millis() >= (Long_Loop_Time + Long_Loop_Interval)) || Startup_Mode == true) {
    Check_Voltages();
    Update_Fuel();
    Update_Temp();
    Update_Dim_Status();
    if (!Calibration_Mode) Control_Fan();

    // More than one long loop completed so change startup flag
    if (Startup_Mode) Startup_Mode = false;

    if (Debug_Mode) {
      Serial.print("Calibration Mode ");
      Serial.println(Calibration_Mode);
    }

    Long_Loop_Time = millis();
  }  // end long loop


}  // end void loop


// ##################################################################################################################################################
// ##################################################################################################################################################


void Check_Button() {


  // =======================================================
  // Check for button press
  // =======================================================

  // Toggle the calibration mode
  // delay allows for debounce
  if (!Startup_Mode) {
    if (digitalRead(Button_Pin) == Digitial_Input_Active) {
      // Allow time for the button pin to settle
      // this assumes some electronic/external debounce
      delay(10);
      while (digitalRead(Button_Pin) == Digitial_Input_Active) {
        // just wait until button released
        Calibration_Mode = true;
      }
    }
  }
  // cant have both demo mode and calibration mode at once
  if (Calibration_Mode) Demo_Mode = false;


}  // End void Check_Button


// ##################################################################################################################################################


void Update_Dim_Status() {


  // =======================================================
  // Dim display when headlights on
  // =======================================================

  if (Light_Status < 3)
  // Normal colours when headlights are off
  {
    Dim_Mode = false;
    Text_Colour1 = METER_WHITE;
    Text_Colour2 = METER_LIGHTGREY;
    Block_Fill_Colour = METER_GREY;
  } else {
    Dim_Mode = true;
    Text_Colour1 = METER_LIGHTGREY;
    Text_Colour2 = METER_DARKGREY;
    Block_Fill_Colour = METER_BLACK;
  }


}  // End void Update_Dim_Status


// ##################################################################################################################################################


void Check_Oil() {

  // =======================================================
  // Process the oil pressure here
  // =======================================================

  if (digitalRead(Oil_Press_Pin) == Digitial_Input_Active) {
    Status_Priority = 6;
    Status_Change_Time = millis();
    // Immediately call the routines to update NeoPixels and status text
    ShiftLight_Strip();
    Display_Warning_Text();
  } else {
    // Unset the Oil Pressure warning sound and text if Oil Pressure becomes good
    if (Status_Priority == 6) Status_Priority = 0;
  }


}  // end void Check_Oil


// ##################################################################################################################################################


void Update_Fuel() {

  // =======================================================
  // Get the Fuel level and display it
  // =======================================================

  // read the analog pin
  Dummy = analogRead(Fuel_Pin);  // do a read and ignore it, allowing the true value to settle
  Raw_Value = analogRead(Fuel_Pin);

  if (Calibration_Mode)
  // show raw calibration values
  {
    Fuel_Litres = Raw_Value;
    the_string = String(Fuel_Litres);
    if (Fuel_Litres < 10) the_string = the_string + " ";
    if (Fuel_Litres < 100) the_string = the_string + " ";
    if (Fuel_Litres < 1000) the_string = the_string + " ";
    my_lcd.Set_Text_Back_colour(METER_BLACK);
    my_lcd.Set_Text_Size(3);
    my_lcd.Set_Text_colour(METER_WHITE);
    my_lcd.Print_String(the_string, Fuel_Bar_X - 30, Fuel_Bar_Y - (Bar_Height / 2));
  }

  if (Demo_Mode) {
    // ----------------- FOR TESTING -----------------
    //Fuel_Litres = 20;
    Fuel_Litres = 22 + random(0, 25) - random(0, 25);
    Fuel_Litres = constrain(Fuel_Litres, Fuel_Min, Fuel_Max);
    // -----------------------------------------------
  }

  if (!Demo_Mode && !Calibration_Mode) {
    // ----------------- FOR REAL -----------------

    // Fuel level formula for standard Datsun 1600 tank sender
    // using this formula
    // Litres = -0.00077 * X^2 + 0.12 * X + 41.4
    // the formual uses the raw analog read pin value, NOT a calculated voltage
    //Fuel_Litres = int(-0.00077 * pow(Fuel_Average, 2) + 0.12 * Fuel_Average + 41.4);
    //Fuel_Litres = constrain(Fuel_Litres, Fuel_Min, Fuel_Max);

    Fuel_Litres = multiMap<int>(Raw_Value, fuel_cal_in, fuel_cal_out, fuel_sample_size);

    // -----------------------------------------------
  }

  if (!Calibration_Mode) {
    // Limit the change between readings
    // to combat fuel sloshing
    Fuel_Litres = (Fuel_Litres + Last_Fuel_Litres) / 2;
    Last_Fuel_Litres = Fuel_Litres;

    // Draw fuel meter
    new_val = map(Fuel_Litres, Fuel_Min, Fuel_Max, Meter_Min, Meter_Max);
    Bar_Meter(new_val, Meter_Min, Meter_Max, Fuel_Bar_X, Fuel_Bar_Y, Bar_Width, Bar_Height, RED2BLUE);

    // Change text colours depending on remaining fuel
    Warn_Text_Colour = METER_WHITE;
    if (Fuel_Litres < Warning_Fuel * 1.4) Warn_Text_Colour = METER_YELLOW;
    if (Fuel_Litres < Warning_Fuel) {
      Status_Change_Time = millis();
      Status_Priority = 4;
      Warn_Text_Colour = METER_RED;
    }

    if (Fuel_Litres >= Warning_Fuel) {
      // Unset the fuel warning status if it was already set
      if (Status_Priority == 4) Status_Priority = 0;
    }

    // Print value using warning text colour
    the_string = String(Fuel_Litres);
    if (Fuel_Litres < 10) the_string = the_string + " ";
    my_lcd.Set_Text_Back_colour(METER_BLACK);
    my_lcd.Set_Text_Size(2);
    my_lcd.Set_Text_colour(Warn_Text_Colour);
    my_lcd.Print_String(the_string, Fuel_Bar_X - 30, Fuel_Bar_Y - (Bar_Height / 2));
  }


}  // End void Update_Fuel


// ##################################################################################################################################################


void Update_Temp() {


  // =======================================================
  // Get the Temperature and display it
  // =======================================================

  // Using DS8B20 for temperature
  if (Use_DS18B20 && !Missing_DS18B20) {
    sensor.requestTemperatures();
    while (!sensor.isConversionComplete())
      ;  // just wait until a valid temperature
    Raw_Value = sensor.getTempC();
  } else {
    // Using any other sensor
    // read the analog pin
    Dummy = analogRead(Temp_Pin);
    Raw_Value = analogRead(Temp_Pin);
  }

  if (Calibration_Mode) {
    // show raw calibration values
    Temp_Celsius = Raw_Value;

    the_string = String(Temp_Celsius);
    if (Temp_Celsius < 10) the_string = the_string + " ";
    if (Temp_Celsius < 100) the_string = the_string + " ";
    if (Temp_Celsius < 1000) the_string = the_string + " ";
    my_lcd.Set_Text_Back_colour(METER_BLACK);
    my_lcd.Set_Text_Size(3);
    my_lcd.Set_Text_colour(METER_WHITE);
    my_lcd.Print_String(the_string, Temp_Bar_X + Bar_Width - 30, Temp_Bar_Y - (Bar_Height / 2));
  }

  if (Demo_Mode) {
    // ----------------- FOR TESTING -----------------
    //Temp_Celsius = 15;
    Temp_Celsius = 80 + random(0, 30) - random(0, 30);
    Temp_Celsius = constrain(Temp_Celsius, Temp_Min, Temp_Max);
    // -----------------------------------------------
  }

  if (!Demo_Mode && !Calibration_Mode) {
    // ----------------- FOR REAL -----------------
    // Comment or uncomment different sections here
    // for different sensors

    // Formula for modified sender using 10k NTC, 1k reference resistor to 5.0v
    // the formula uses the raw analog read pin value, NOT a calculated voltage
    // -0.00001246X^3+0.00675X^2-1.577X+217.136
    //Temp_Celsius = int(-0.00001246 * pow(Temp_Float, 3) + 0.00675 * pow(Temp_Float, 2) - 1.577 * Temp_Float + 217.136);

    // Formula for standard SR20 temperature gauge sender, 1k reference resistor to 5.0v
    //Temp_Celsius = int(-32.36 * log(Temp_Float) + 203.82);
    //Temp_Celsius = constrain(Temp_Celsius, Temp_Min, Temp_Max);

    if (Use_DS18B20) {
      // even though a DS18B20 has a wider range
      // this keeps the tmep within bounds of the meter
      Temp_Celsius = constrain(Raw_Value, Temp_Min, Temp_Max);
      if (Missing_DS18B20) Temp_Celsius = Alert_Temp;
    } else {
      // and this has the effect of being a Constraint
      Temp_Celsius = multiMap<int>(Raw_Value, temp_cal_in, temp_cal_out, temp_sample_size);
    }

    // ----------------- End real measurement ------
  }

  if (!Calibration_Mode) {
    // Draw temperature meter
    new_val = map(Temp_Celsius, Temp_Min, Temp_Max, Meter_Min, Meter_Max);
    Bar_Meter(new_val, Meter_Min, Meter_Max, Temp_Bar_X, Temp_Bar_Y, Bar_Width, Bar_Height, BLUE2RED);

    // Change text colours depending on temperature
    if (Status_Priority == 5) Status_Priority = 0;
    Warn_Text_Colour = METER_WHITE;
    if (Temp_Celsius < Low_Temp || Temp_Celsius >= Fan_On_Temp) Warn_Text_Colour = METER_YELLOW;
    if (Temp_Celsius >= Alert_Temp) {
      Status_Priority = 5;
      Status_Change_Time = millis();
      Warn_Text_Colour = METER_RED;
    }

    // Print value using warning text colour
    the_string = String(Temp_Celsius);
    if (Temp_Celsius < 10) the_string = the_string + " ";
    if (Temp_Celsius < 100) the_string = the_string + " ";
    my_lcd.Set_Text_Back_colour(METER_BLACK);
    my_lcd.Set_Text_Size(2);
    my_lcd.Set_Text_colour(Warn_Text_Colour);
    my_lcd.Print_String(the_string, Temp_Bar_X + Bar_Width + 10, Temp_Bar_Y - (Bar_Height / 2));
  }


}  // End void Update_Temp


// ##################################################################################################################################################


void Control_Fan() {


  // =======================================================
  // Turn the fan on or off
  // =======================================================

  // Use the temperature to control fan
  // skip the fan control in calibration mode and leave it off

  my_lcd.Set_Text_Size(2);
  my_lcd.Set_Text_Back_colour(METER_BLACK);

  // Turn on the fan and remember the time whenever temp is too high
  if (Temp_Celsius >= Fan_On_Temp) {
    // set the Fan_On_Time every loop until the temp drops
    // ensure the relay is on
    digitalWrite(Relay_Pin, Fan_On);
    Fan_On_Time = millis();
    if (Status_Priority != 5) Status_Priority = 3;
    Status_Change_Time = millis();
  }

  // Turn off the fan when lower than Fan_On_Temp degrees and past the Hysteresis time
  if ((Temp_Celsius < Fan_On_Temp) && (millis() >= (Fan_On_Time + Fan_On_Hyst))) {
    // ensure the relay is off
    digitalWrite(Relay_Pin, !Fan_On);
    if (Status_Priority == 3) Status_Priority = 0;
  }


}  // end void Control_Fan


// ##################################################################################################################################################


void Check_Voltages() {


  // =======================================================
  // Get voltage and alternator indicator for comparison
  // =======================================================

  // read analog pins
  Dummy = analogRead(Batt_Volt_Pin);
  Raw_Battery_Volts = analogRead(Batt_Volt_Pin);
  Dummy = analogRead(Alternator_Pin);
  Raw_Alternator_Volts = analogRead(Alternator_Pin);

  // Use raw calibration values
  if (Calibration_Mode) Battery_Volts = Raw_Battery_Volts;

  if (Demo_Mode) {
    // ----------------- FOR TESTING -----------------
    //Battery_Volts = 138;
    Battery_Volts = 140 + random(0, 20) - random(0, 20);
    Alternator_Volts = Battery_Volts + random(0, 5) - random(0, 20);
    // -----------------------------------------------
  }

  if (!Demo_Mode && !Calibration_Mode) {
    // ----------------- FOR REAL -----------------
    // all volts here multiplied by 10 so it can still be used in integer form
    // and divided by 10 for display with 2 decimal places
    Battery_Volts = round(Raw_Battery_Volts * Input_Multiplier * 10.0);
    Alternator_Volts = round(Raw_Alternator_Volts * Input_Multiplier * 10.0);
    // -----------------------------------------------
    Battery_Volts = constrain(Battery_Volts, 80, 160);
    Alternator_Volts = constrain(Alternator_Volts, 60, 160);
  }



  if ((millis() - Status_Change_Time) > (Long_Loop_Interval * 2)) {
    // wait until other warnings have had a chance to be displayed
    // voltages all good
    if ((Alternator_Volts >= (Battery_Volts * 0.9)) && (Battery_Volts < Volts_High)) {
      Status_Priority = 0;
    }
    // battery voltage out of range
    if (Battery_Volts <= Volts_Low || Battery_Volts >= Volts_High) {
      Status_Priority = 1;
    }
    // alternator not working
    if ((Alternator_Volts < (Battery_Volts * 0.9)) && Startup_Mode == false) {
      Status_Priority = 2;
    }
  }


}  // End void Check_Voltages


// ##################################################################################################################################################


void Display_Warning_Text() {


  // =======================================================
  // Display warning text and initiate warning sounds
  // =======================================================

  // Status_Priority values
  /*
    0 default (good volts)
    1 volts (bad)
    2 alternator
    3 fan
    4 fuel
    5 temperature alert
    6 oil press alert

    maximum 8 chars
  */

  my_lcd.Set_Text_Size(4);

  if (Calibration_Mode) {
    my_lcd.Set_Text_colour(METER_WHITE);
    my_lcd.Set_Text_Back_colour(METER_BLACK);
    the_string = String(Battery_Volts) + " V    ";
    my_lcd.Print_String(the_string, Status_Text_X, Status_Text_Y);
  } else {
    switch (Status_Priority) {
      // start of the switch statement
      // in reverse order so highest priority is executed first
      case 6:
        // Bad oil pressue
        digitalWrite(OP_Warning_Pin, Valid_Warning);
        my_lcd.Set_Text_colour(METER_RED);
        my_lcd.Set_Text_Back_colour(METER_YELLOW);
        my_lcd.Print_String("   OIL  ", Status_Text_X, Status_Text_Y);
        break;
      case 5:
        // Over temperature
        digitalWrite(OP_Warning_Pin, Valid_Warning);
        my_lcd.Set_Text_colour(METER_RED);
        my_lcd.Set_Text_Back_colour(METER_BLACK);
        my_lcd.Print_String("  TEMP! ", Status_Text_X - 8, Status_Text_Y);
        break;
      case 4:
        // Low fuel
        digitalWrite(Warning_Pin, Valid_Warning);
        my_lcd.Set_Text_colour(METER_CYAN);
        my_lcd.Set_Text_Back_colour(METER_BLACK);
        my_lcd.Print_String("  FUEL  ", Status_Text_X, Status_Text_Y);
        break;
      case 3:
        // Fan is on
        my_lcd.Set_Text_colour(METER_MAGENTA);
        my_lcd.Set_Text_Back_colour(METER_BLACK);
        my_lcd.Print_String(" FAN ON ", Status_Text_X, Status_Text_Y);
        break;
      case 2:
        // Bad alternator
        digitalWrite(Warning_Pin, !Valid_Warning);
        my_lcd.Set_Text_colour(METER_ORANGE);
        my_lcd.Set_Text_Back_colour(METER_BLACK);
        my_lcd.Print_String(" CHARGE ", Status_Text_X, Status_Text_Y);
        break;
      case 1:
        // Low voltage
        digitalWrite(Warning_Pin, Valid_Warning);
        my_lcd.Set_Text_colour(METER_YELLOW);
        my_lcd.Set_Text_Back_colour(METER_BLACK);
        the_string = " " + String((float(Battery_Volts) / 10.0), 1) + " v  ";
        if (Battery_Volts < 100) the_string = " " + the_string;
        my_lcd.Print_String(the_string, Status_Text_X, Status_Text_Y);
        break;
      case 0:
        // All good
        // unset warning sound pins
        digitalWrite(Warning_Pin, !Valid_Warning);
        digitalWrite(OP_Warning_Pin, !Valid_Warning);
        my_lcd.Set_Text_colour(Text_Colour2);
        my_lcd.Set_Text_Back_colour(METER_BLACK);
        the_string = " " + String((float(Battery_Volts) / 10.0), 1) + " v  ";
        my_lcd.Print_String(the_string, Status_Text_X, Status_Text_Y);
        break;
      default:
        // dont do anything
        break;
    }  // the end of the switch statement.
  }


}  // end void Display_Warning_Text


// ##################################################################################################################################################


void ShiftLight_Strip() {


  // =======================================================
  // OPerate the NeoPixel strip
  // =======================================================

  // Set low brightness if headlights are on
  if (Dim_Mode) strip.setBrightness(LED_Dim);
  else strip.setBrightness(LED_Bright);

  if (Demo_Mode) {
    // ----------------- FOR TESTING -----------------
    //Old_PWM_duty = PWM_duty;
    if (Count_Up && PWM_duty < 110) {
      PWM_duty = PWM_duty + 10;
      if (PWM_duty > 100) Count_Up = !Count_Up;
    }
    if (!Count_Up && PWM_duty > -20) {
      PWM_duty = PWM_duty - 10;
      if (PWM_duty <= -10) Count_Up = !Count_Up;
    }
    //PWM_duty = random(-200, 120);
    //PWM_duty = 10;
    //PWM_duty = constrain(PWM_duty, Old_PWM_duty - 50, Old_PWM_duty + 50);
    // -----------------------------------------------
  } else {
    // ----------------- FOR REAL -----------------
    // read the PWM input and derive a number of LEDs to light
    PWM_high = pulseIn(RPM_PWM_In_Pin, HIGH);
    PWM_low = pulseIn(RPM_PWM_In_Pin, LOW);
    PWM_duty = (PWM_high / (PWM_high + PWM_low)) * 100;
    // -----------------------------------------------
  }

  PWM_duty = constrain(PWM_duty, 0, 100);

  // Display RPM on the WS2812 LED strip for shiftlight function
  // Set the colour based on how many LEDs will be illuminated
  // Colour is chosen from the array
  LED_pos = map(PWM_duty, 0, 100, 0, LED_Count);
  strip.fill(LED_Colour[LED_pos], 0, LED_pos);

  // Set unused pixels to off (black)
  // Unless one of the following sections sets the last LED it will remain off
  if (LED_pos < LED_Count) strip.fill(0, LED_pos, LED_Count);

  /*
      Lights_NotChanged = 0;
      Lights_Off  = 1;
      Lights_Park = 2;
      Lights_Low  = 3;
      Lights_High = 4;
    */

  // Display headlight status on the last LED
  //if (PL_CurrentStatus == HIGH || LB_CurrentStatus == HIGH)
  if (Light_Status > 1 && Light_Status < 4) {
    // Low beam or Parkers
    // Set end LED to dark green
    strip.fill(0x009900, LED_Count, LED_Count);
  }
  //if (HB_CurrentStatus == HIGH)
  if (Light_Status > 3) {
    // High beam
    // Set end LED to dark blue
    strip.fill(0x000099, LED_Count, LED_Count);
  }

  // Display any warnings on the last LED
  // using the same system as the text warnings
  // This overwrites headlight status
  /*
    0 default (good volts)
    1 volts (bad)
    2 alternator
    3 fan
    4 fuel
    5 temperature alert
    6 oil press alert
  */
  switch (Status_Priority) {
    // start of the switch statement
    // in reverse order so highest priority is executed first
    case 6:
      // Oil Pressure
      // Set all LEDs to red
      strip.fill(0xFF0000, 0, LED_Count + 1);
      break;
    case 5:
      // Over temperature
      // Set end LED to red
      strip.fill(0xBF0000, LED_Count, LED_Count);
      break;
    case 4:
      // Fuel Low
      // Set end LED to cyan
      strip.fill(0x30CFCF, LED_Count, LED_Count);
      break;
    case 3:
      // Fan on
      // Set end LED to dark magenta
      strip.fill(0x690069, LED_Count, LED_Count);
      break;
    case 2:
      // Alternator fault
      // Set end LED to dark orange
      strip.fill(0xCF4C00, LED_Count, LED_Count);
      break;
    case 1:
      // Bad voltage
      // Set end LED to yellow
      strip.fill(0xBDBD00, LED_Count, LED_Count);
      break;
    case 0:
      // nothing wrong
      // last LED should already be black unless it was specifically set
      break;
    default:
      // do nothing
      break;
  }  // the end of the switch statement.

  // Send the updated pixel info to the hardware
  strip.show();

  if (Debug_Mode) {
    Serial.print("PWM_duty ");
    Serial.println(PWM_duty);
    Serial.print("LED_pos ");
    Serial.println(LED_pos);
    // Slow down the PWM readingss for debug mode
    delay(400);
  }


}  // end void ShiftLight_Strip


// ##################################################################################################################################################


void Headlight_Status() {


  // =======================================================
  // Display the headlights status
  // =======================================================

  // only draw over the graphic indicators on screen if the status changes
  /*
    Lights_NotChanged = 0;
    Lights_Off  = 1;
    Lights_Park = 2;
    Lights_Low  = 3;
    Lights_High = 4;
  */

  // Using individual digital inputs
  // get all the current headlight inputs
  // the parker lights input is disabled during demo mode due to sensitivity
  if (Calibration_Mode) {
    my_lcd.Set_Draw_color(METER_GREY);
    my_lcd.Fill_Round_Rectangle(Light_Status_X, Light_Status_Y, Light_Status_X + Light_Status_Length, Light_Status_Y + Light_Status_Height, 5);
    my_lcd.Set_Text_Size(4);
    my_lcd.Set_Text_colour(METER_WHITE);
    my_lcd.Set_Text_Back_colour(METER_GREY);
    my_lcd.Print_String("CALIBR", Light_Status_X + 22, Light_Status_Y + 12);
  } else {
    if (Demo_Mode) Light_Status = Lights_Off;
    else {
      if (digitalRead(Parker_Light_Pin) == !Digitial_Input_Active) Light_Status = Lights_Off;
      if (digitalRead(Parker_Light_Pin) == Digitial_Input_Active) Light_Status = Lights_Park;
    }
    if (digitalRead(Low_Beam_Pin) == Digitial_Input_Active) Light_Status = Lights_Low;
    if (digitalRead(High_Beam_Pin) == Digitial_Input_Active) Light_Status = Lights_High;

    // Draw the lights indicator
    // Only overwrite it if there has been a change
    if (Light_Status != Light_Last_Status) {
      switch (Light_Status) {  // start of the switch statement
        case Lights_NotChanged:
          // do nothing
          break;
        case Lights_Off:
          my_lcd.Set_Draw_color(METER_GREY);
          my_lcd.Fill_Round_Rectangle(Light_Status_X, Light_Status_Y, Light_Status_X + Light_Status_Length, Light_Status_Y + Light_Status_Height, 5);
          my_lcd.Set_Text_Size(4);
          my_lcd.Set_Text_colour(METER_BLACK);
          my_lcd.Set_Text_Back_colour(METER_GREY);
          my_lcd.Print_String("LIGHTS", Light_Status_X + 22, Light_Status_Y + 12);
          break;
        case Lights_Park:
          my_lcd.Set_Draw_color(METER_YELLOW);
          my_lcd.Fill_Round_Rectangle(Light_Status_X, Light_Status_Y, Light_Status_X + Light_Status_Length, Light_Status_Y + Light_Status_Height, 5);
          my_lcd.Set_Text_Size(4);
          my_lcd.Set_Text_colour(METER_BLACK);
          my_lcd.Set_Text_Back_colour(METER_YELLOW);
          my_lcd.Print_String("PARK", Light_Status_X + 50, Light_Status_Y + 12);
          break;
        case Lights_Low:
          my_lcd.Set_Draw_color(METER_GREEN);
          my_lcd.Fill_Round_Rectangle(Light_Status_X, Light_Status_Y, Light_Status_X + Light_Status_Length, Light_Status_Y + Light_Status_Height, 5);
          my_lcd.Set_Text_Size(4);
          my_lcd.Set_Text_colour(METER_BLACK);
          my_lcd.Set_Text_Back_colour(METER_GREEN);
          my_lcd.Print_String("LOW", Light_Status_X + 60, Light_Status_Y + 12);
          break;
        case Lights_High:
          my_lcd.Set_Draw_color(METER_BLUE);
          my_lcd.Fill_Round_Rectangle(Light_Status_X, Light_Status_Y, Light_Status_X + Light_Status_Length, Light_Status_Y + Light_Status_Height, 5);
          my_lcd.Set_Text_Size(4);
          my_lcd.Set_Text_colour(METER_BLACK);
          my_lcd.Set_Text_Back_colour(METER_BLUE);
          my_lcd.Print_String("HIGH", Light_Status_X + 50, Light_Status_Y + 12);
          break;
        default:
          // do nothing
          break;
      }  // the end of the switch statement.
    }
    Light_Last_Status = Light_Status;
  }

}  // end void Headlight_Status


// ##################################################################################################################################################
// ##################################################################################################################################################


void Bar_Meter(int value, int vmin, int vmax, int x, int y, int w, int h, byte scheme) {

  int block_colour;
  int y1, y2;
  int segs = (vmax - vmin) / 20;
  int g = h / segs;

  int v = map(value, vmin, vmax, 0, segs);

  // Draw "segs" colour blocks
  for (int b = 0; b <= segs; b++) {

    // Calculate pair of coordinates for segment start and end
    int y1 = y - b * g;
    int y2 = y - g - b * g;

    if (v > 0 && b <= v) {
      if (!Dim_Mode) {
        // Choose colour from scheme
        block_colour = 0;
        switch (scheme) {
          case 0: block_colour = METER_RED; break;                          // Fixed colour
          case 1: block_colour = METER_GREEN; break;                        // Fixed colour
          case 2: block_colour = METER_BLUE; break;                         // Fixed colour
          case 3: block_colour = rainbow(map(b, 0, segs, 0, 127)); break;   // Blue to red
          case 4: block_colour = rainbow(map(b, 0, segs, 63, 127)); break;  // Green to red
          case 5: block_colour = rainbow(map(b, 0, segs, 127, 63)); break;  // Red to green
          case 6: block_colour = rainbow(map(b, 0, segs, 127, 0)); break;   // Red to blue
          default: block_colour = METER_BLUE; break;                        // Fixed colour
        }
      } else {
        block_colour = Text_Colour2;
      }

      // Fill in coloured blocks
      my_lcd.Set_Draw_color(block_colour);
      my_lcd.Fill_Rectangle(x, y1, x + w, y2);
    }
    // Fill in blank segments
    else {
      my_lcd.Set_Draw_color(Block_Fill_Colour);
      my_lcd.Fill_Rectangle(x, y1, x + w, y2);
    }
  }


}  // End void barmeter


// #########################################################################


// Function to return a 16 bit rainbow colour

unsigned int rainbow(byte value) {
  // Value is expected to be in range 0-127
  // The value is converted to a spectrum colour from 0 = blue through to 127 = red

  byte red = 0;    // Red is the top 5 bits of a 16 bit colour value
  byte green = 0;  // Green is the middle 6 bits
  byte blue = 0;   // Blue is the bottom 5 bits

  byte quadrant = value / 32;

  if (quadrant == 0) {
    blue = 31;
    green = 2 * (value % 32);
    red = 0;
  }
  if (quadrant == 1) {
    blue = 31 - (value % 32);
    green = 63;
    red = 0;
  }
  if (quadrant == 2) {
    blue = 0;
    green = 63;
    red = value % 32;
  }
  if (quadrant == 3) {
    blue = 0;
    green = 63 - 2 * (value % 32);
    red = 31;
  }
  return (red << 11) + (green << 5) + blue;
}


// ##################################################################################################################################################
// ##################################################################################################################################################

/***************************************************
*                                                  *
*   Arduino Mega2560 based LCD automotive gauge    *
*   for displaying temperature, fuel and voltage   *
*   Temperature is used to control a fan           *
*   A LED strip is used for some warnings          *
*   and a shiftlight function                      *
*   This gauge is used in portrait mode            *
*                                                  *
***************************************************/


// Just in case you are not using the Arduino IDE
//#include <arduino.h>


/*
  FuelLevel Temperature and Voltage barmeters
  including range warning and fan control
  Oil Pressure warning
  Alternator warning, requires 2nd voltage input
  Dim on low-beam
  Headlights indication
  unified warning message area
  RPM Serial2 input
  Shiflight Neopixel LED output
  Last LED for status
  DS18B20 temperature sensor support commented out
  NTC resistor support commented out
  Using Nissan SR20 temperature sensor by default
  Offloaded sounds to external Leonardo Tiny
  Uses multimap instead of formulas for temperature and fuel level
  Changed to using c-strings
  Added proper switch debouncing
*/


#define Version "Fuel Temp Bar V18"



//========================================================================
//-------------------------- Set These Manually --------------------------
//========================================================================

// set true for using Onewire DS18B20 temperature sensor
const bool  Use_DS18B20           = false;

int         Fan_On_Hyst           = 20000;    // msec Hysteresis, minimum run time of fan
const int   Fan_On_Temp           = 90;       // degrees C fan on
const int   Fan_Off_Temp          = 87;       // degrees C fan off
const int   Alert_Temp            = 98;       // degrees C alert level
const int   Low_Temp              = 71;       // degree C for low temperatures
const float Volts_Low             = 13.3;     // low volts warning level
const float Volts_High            = 14.4;     // high volts warning level

const bool  Valid_Warning         = LOW;     // set high or low for valid warnings to be passed to external processing
const bool  Fan_On                = HIGH;    // set high or low for operating the fan relay
const bool  Digitial_Input_Active = LOW;     // set whether digitial inputs are Low or High for active

// Set these to ensure correct voltage readings of analog inputs
const float vcc_ref  = 4.92;       // measure the 5 volts DC and set it here
const float R1       = 15000.0;    // measure and set the voltage divider values
const float R2       = 39000.0;    // for accurate voltage measurements

// The range of RPM on the neopixel strip is dictated by the output from the RPM module
// set the length of the NeoPixel shiftlight strip
const int LED_Count       = 8;
const int LED_Dim         = 10;
const int LED_Bright      = 80;

const int Sensor_Readings = 5;    // number of repeated sensor readings to average
const int Sensor_Delay    = 5;    // delay between repeated sensor readings

//========================================================================



//========================================================================
//-------------------------- Calibration mode ----------------------------
//========================================================================

// Demo = true gives random speed values
// Calibration = true displays some calculated and raw values
// Pressing the button changes to Caibration mode
bool Calibration_Mode = false;
bool Demo_Mode        = false;
bool Debug_Mode       = false;

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
int       fuel_cal_in[]    = { 42, 110, 151, 185, 199, 216, 226, 240, 247, 255 };
int       fuel_cal_out[]   = { 45, 40, 35, 30, 25, 20, 15, 20, 5, 0 };

// SR20 temp sender Celcius
const int temp_sample_size = 11;
int       temp_cal_in[]    = { 15, 20, 25, 30, 33, 44, 61, 85, 109, 198, 332 };
int       temp_cal_out[]   = { 120, 108, 100, 94, 90, 80, 70, 60, 53, 36, 20 };

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

// Onewire and DS18B20 libraries
// Using a cut-down integer-only version to
// improve read times of the DS18B20 sensor
#include <OneWire.h>
#include <DS18B20_INT.h>
#define ONE_WIRE_BUS 14    // Specify the OneWire bus pin
OneWire     oneWire(ONE_WIRE_BUS);
DS18B20_INT sensor(&oneWire);

//========================================================================


#include <Adafruit_NeoPixel.h>
#include <LCDWIKI_GUI.h>    //Core graphics library
#include <LCDWIKI_KBV.h>    //Hardware-specific library

// Meter colour schemes
// LCD colours are 16bit
#define RED2RED         0
#define GREEN2GREEN     1
#define BLUE2BLUE       2
#define BLUE2RED        3
#define GREEN2RED       4
#define RED2GREEN       5
#define RED2BLUE        6

#define LCD_BLACK       0x0000 /*   0,   0,   0 */
#define LCD_NAVY        0x000F /*   0,   0, 128 */
#define LCD_DARKGREEN   0x1684 /*   0, 128,   0 */
#define LCD_DARKCYAN    0x03EF /*   0, 128, 128 */
#define LCD_MAROON      0x7800 /* 128,   0,   0 */
#define LCD_PURPLE      0x780F /* 128,   0, 128 */
#define LCD_OLIVE       0x7BE0 /* 128, 128,   0 */
#define LCD_LIGHTGREY   0xC618 /* 192, 192, 192 */
#define LCD_DARKGREY    0x4A69 /*  80,  80,  80 */
#define LCD_BLUE        0x001F /*   0,   0, 255 */
#define LCD_GREEN       0x07E0 /*   0, 255,   0 */
#define LCD_CYAN        0x07FF /*   0, 255, 255 */
#define LCD_RED         0xF800 /* 255,   0,   0 */
#define LCD_MAGENTA     0xF81F /* 255,   0, 255 */
#define LCD_YELLOW      0xFFE0 /* 255, 255,   0 */
#define LCD_WHITE       0xFFFF /* 255, 255, 255 */
#define LCD_ORANGE      0xFD20 /* 255, 165,   0 */
#define LCD_GREENYELLOW 0xAFE5 /* 173, 255,  47 */
#define LCD_PINK        0xF81F
#define LCD_GREY        0x2104    // Dark grey 16 bit colour

// Change some colours because backlight control is not available
bool Dim_Mode          = false;
int  Text_Colour1      = LCD_WHITE;        // or VGA_SILVER
int  Text_Colour2      = LCD_LIGHTGREY;    // or VGA_GRAY
int  Block_Fill_Colour = LCD_GREY;         // or VGA_BLACK



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
   or
   Vin = analogRead(ANALOG_IN_PIN) * Input_Multiplier
*/
const float Input_Multiplier = vcc_ref / 1024.0 / (R2 / (R1 + R2));

// Common pin definitions
const int SD_Select = 53;

// Pin definitions for digital inputs
// Mega2560 Serial2 pins 17(RX), 16(TX)
const int Button_Pin       = 0;    // Button momentary input
const int Pbrake_Input_Pin = 1;    // Park brake input pin
const int Oil_Press_Pin    = 2;    // Oil pressure digital input pin, needs to be 2 or 3 for ISR
const int Parker_Light_Pin = 3;    // Parker lights digital input pin
const int Low_Beam_Pin     = 4;    // Low beam digital input pin
const int High_Beam_Pin    = 5;    // High beam digital input pin
const int VSS_Input_Pin    = 6;    // Speed frequency input pin
const int RPM_Input_Pin    = 7;    // RPM frequency INPUT pin

// Pin definitions for analog inputs
const int Temp_Pin       = A0;    // Temperature analog input pin - OneWire sensor on pin 14
const int Fuel_Pin       = A1;    // Fuel level analog input pin
const int Batt_Volt_Pin  = A2;    // Voltage analog input pin
const int Alternator_Pin = A3;    // Alternator indicator analog input pin

// Pin definitions for outputs
// Mega2560 Serial2 pins 17(RX), 16(TX)
const int LED_Pin        = 10;    // NeoPixel LED pin
const int Warning_Pin    = 11;    // Link to external Leonardo for general warning sounds
const int OP_Warning_Pin = 12;    // Link to external Leonardo for oil pressure warning sound
const int Relay_Pin      = 13;    // Relay for fan control

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
LCDWIKI_KBV my_lcd(ILI9481, 40, 38, 39, -1, 41);    //model,cs,cd,wr,rd,reset

// Input variables
float Sensor_Average;
int   Temp_Celsius, Fuel_Litres, Last_Fuel_Litres;
float Battery_Volts, Alternator_Volts;
int   Raw_Battery_Volts, Raw_Alternator_Volts;
int   Raw_Value;
int   i;    // used in for loops

// Headligh Indicator status
// variables for state machine
// and set startup values
const int Lights_NotChanged = 0;
const int Lights_Off        = 1;
const int Lights_Park       = 2;
const int Lights_Low        = 3;
const int Lights_High       = 4;
int       Light_Status      = Lights_Off;
int       Light_Last_Status = Lights_High;

// Event timing and status
const uint32_t    Long_Loop_Interval  = 4000;    // 4 seconds between linear bar meter updates
const uint32_t    Short_Loop_Interval = 1000;    // 1 second between more important loops
uint32_t          Long_Loop_Time, Short_Loop_Time, Fan_On_Time;
volatile uint32_t Status_Change_Time;
bool              Startup_Mode    = true;
bool              Missing_DS18B20 = false;

// Position of entire display area
const int LCD_Offset_X = 40;
const int LCD_Offset_Y = 50;

// Position of Headlight status
const int Light_Status_X      = LCD_Offset_X + 25;
const int Light_Status_Y      = LCD_Offset_Y + 0;
const int Light_Status_Length = 180;
const int Light_Status_Height = 50;

// Position of Status text
const int    Status_Text_X   = LCD_Offset_X + 22;
const int    Status_Text_Y   = LCD_Offset_Y + 300;
char         Status_Text[20] = "ComingReadyOrNot";    // a long string so we never overflow
volatile int Status_Priority = 0;

// Meter variables
// bar X and Y are from bottom left of each bar
const int Fuel_Bar_X = LCD_Offset_X + 40, Fuel_Bar_Y = LCD_Offset_Y + 280;
const int Temp_Bar_X = LCD_Offset_X + 135, Temp_Bar_Y = LCD_Offset_Y + 280;
const int Bar_Height = 200;
const int Bar_Width  = 60;
int       new_val;
char      reusable_string[20] = "AreWeThereYet";    // a long string so we never overflow
uint16_t  Warn_Text_Colour    = LCD_WHITE;

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
int      RPM_LED_Pos;
bool     Count_Up       = true;
uint32_t Initial_Colour = 0x0000FF;
uint32_t LED_Colour[LED_Count];



// ##################################################################################################################################################



void setup()
{

    // Start a serial port for sending RPM LED shift light position
    // Serial2 17(RX), 16(TX)
    Serial2.begin(57600);

    analogReference(DEFAULT);

    // Improved randomness for testing
    // Choose an unused analog pin
    unsigned long seed = 0, count = 32;
    while (--count)
        seed = (seed << 1) | (analogRead(A6) & 1);
    randomSeed(seed);

    // Create a colour scheme for number of Neopixel LEDs
    // with black as the first value and white as the last value
    // LED colours are 32 bit
    LED_Colour[0]         = 0x000000;
    LED_Colour[1]         = Initial_Colour;
    LED_Colour[LED_Count] = 0xFFFFFF;
    uint32_t colour_step  = (LED_Colour[LED_Count] - Initial_Colour) / LED_Count;
    for (int i = 2; i < LED_Count; i++)
    {
        LED_Colour[i] = Initial_Colour + (i - 1) * colour_step;
    }

    // Manually created colour schemes
    // INVERSE HSV blue/light blue/white
    //uint32_t LED_Colour[] = { 0x000000, 0x0000FF, 0x0000FF, 0x508AFF, 0x508AFF, 0xA0DFFF, 0xA0DFFF, 0xFFFFFF, 0xFFFFFF };
    // ---> use this one
    // Pink, light Blue, White
    //uint32_t LED_Colour[] = { 0x000000, 0xFF00FF, 0x3AA1FF, 0x00D3FF, 0x00EEFF, 0x8FF4FF, 0xCCFAFF, 0xFFFFFF, 0xFFFFFF };
    // RGB red/orange/yellow/white
    //uint32_t LED_Colour[] = { 0x000000, 0xE64C00, 0xF27500, 0xFA9B00, 0xFFBF00, 0xFFD470, 0xFFE9B8, 0xFFFFFF, 0xFFFFFF };
    //  green/yellow/orange/red
    //uint32_t LED_Colour[] = { 0x000000, 0x00FF00, 0x00FF00, 0xFFFF00, 0xFFFF00, 0xFFAA00, 0xFFAA00, 0xFF4800, 0xFF4800 };
    //uint32_t LED_Colour[] = { 0x000000, 0xFFE500, 0xFFCA00, 0xFFAD00, 0xFF9000, 0xFF7000, 0xFF4B00, 0xFF0000, 0xFF0000 };

    // INITIALIZE NeoPixel strip
    // before initialising the LCD panel
    strip.begin();
    strip.clear();
    strip.setBrightness(LED_Dim);
    strip.show();    // Turn OFF all pixels ASAP

    // Outputs
    pinMode(Relay_Pin, OUTPUT);
    digitalWrite(Relay_Pin, !Fan_On);    // ensure fan relay is OFF at least until temperature is measured
    pinMode(Warning_Pin, OUTPUT);
    digitalWrite(Warning_Pin, !Valid_Warning);
    pinMode(OP_Warning_Pin, OUTPUT);
    digitalWrite(OP_Warning_Pin, !Valid_Warning);

    // Digital inputs
    // remove input_pullup's after testing
    // since pullups are handled by external hardware
    pinMode(Oil_Press_Pin, INPUT_PULLUP);
    pinMode(Parker_Light_Pin, INPUT_PULLUP);
    pinMode(Low_Beam_Pin, INPUT_PULLUP);
    pinMode(High_Beam_Pin, INPUT_PULLUP);
    pinMode(Button_Pin, INPUT_PULLUP);

    // Analog inputs
    pinMode(Temp_Pin, INPUT);
    pinMode(Fuel_Pin, INPUT);
    pinMode(Batt_Volt_Pin, INPUT);
    pinMode(Alternator_Pin, INPUT);

    // Display startup text
    my_lcd.Init_LCD();
    my_lcd.Set_Rotation(4);
    my_lcd.Fill_Screen(LCD_BLACK);
    my_lcd.Set_Text_Back_colour(LCD_BLACK);
    my_lcd.Set_Text_colour(LCD_LIGHTGREY);
    my_lcd.Set_Text_Size(2);
    my_lcd.Print_String(Version, CENTER, LCD_Offset_Y + 160);
    delay(1000);

    // Using DS8B20 for temperature
    if (Use_DS18B20)
    {
        if (!sensor.begin())
        {
            Missing_DS18B20 = true;
            // Display warning for missing DS8B20 sensor
            my_lcd.Fill_Screen(LCD_BLACK);
            my_lcd.Set_Text_colour(LCD_ORANGE);
            my_lcd.Set_Text_Size(4);
            my_lcd.Print_String("MISSING", CENTER, LCD_Offset_Y + 100);
            my_lcd.Print_String("TEMP", CENTER, LCD_Offset_Y + 140);
            my_lcd.Print_String("SENSOR", CENTER, LCD_Offset_Y + 180);
            delay(10000);
        }
        else
            Missing_DS18B20 = false;
    }

    // Display static text
    my_lcd.Fill_Screen(LCD_BLACK);
    my_lcd.Set_Text_Back_colour(LCD_BLACK);
    my_lcd.Set_Text_Size(3);
    my_lcd.Set_Text_colour(LCD_WHITE);
    my_lcd.Print_String("F", Fuel_Bar_X - 25, Fuel_Bar_Y - (Bar_Height / 2) - 40);
    my_lcd.Print_String("T", Temp_Bar_X + Bar_Width + 10, Temp_Bar_Y - (Bar_Height / 2) - 40);

    // Cylon eye effect during startup
    strip.setBrightness(LED_Bright);
    for (int i = 0; i < LED_Count + 3; i++)
    {
        delay(50);
        strip.setPixelColor(i, strip.Color(255, 0, 0));
        strip.setPixelColor(i - 1, strip.Color(50, 0, 0));
        strip.setPixelColor(i - 2, strip.Color(12, 0, 0));
        strip.setPixelColor(i - 3, strip.Color(0, 0, 0));
        strip.show();
    }
    for (int i = LED_Count + 3; i > -4; i--)
    {
        strip.setPixelColor(i, strip.Color(255, 0, 0));
        strip.setPixelColor(i + 1, strip.Color(50, 0, 0));
        strip.setPixelColor(i + 2, strip.Color(12, 0, 0));
        strip.setPixelColor(i + 3, strip.Color(0, 0, 0));
        strip.show();
        delay(50);
    }
    strip.clear();

    // Check calibration or demo mode status - cant have both modes at once
    if (Calibration_Mode)
        Demo_Mode = false;
    if (Demo_Mode)
        Fan_On_Hyst = Fan_On_Hyst / 3;

    //attachInterrupt(digitalPinToInterrupt(Oil_Press_Pin), Check_Oil, RISING);

    Long_Loop_Time  = millis();
    Short_Loop_Time = millis();


}    // end void setup



// ##################################################################################################################################################
// ##################################################################################################################################################



void loop()
{


    // =======================================================
    // Always do these every loop
    // =======================================================

    Check_Oil();
    if (!Demo_Mode)
        ShiftLight_Strip();


    // =======================================================
    // Do these items at a short interval
    // =======================================================

    if ((millis() >= (Short_Loop_Time + Short_Loop_Interval)) || Startup_Mode == true)
    {
        Headlight_Status();
        Display_Warning_Text();
        Check_Button();
        // In demo mode ony update the neopixels periodically
        if (Demo_Mode)
            ShiftLight_Strip();

        Short_Loop_Time = millis();
    }    // end short loop


    // =======================================================
    // Do these items at a long interval
    // =======================================================

    if ((millis() >= (Long_Loop_Time + Long_Loop_Interval)) || Startup_Mode == true)
    {
        Check_Voltages();
        Update_Fuel();
        Update_Temperature();
        Update_Dim_Status();
        // Only operate the fan in normal mode
        if (!Calibration_Mode)
            Control_Fan();

        // More than one long loop completed so change startup flag
        if (Startup_Mode)
            Startup_Mode = false;

        Long_Loop_Time = millis();
    }    // end long loop


}    // end void loop


// ##################################################################################################################################################
// ##################################################################################################################################################


void Check_Button()
{


    // =======================================================
    // Check for button press
    // =======================================================

    // Toggle the calibration mode
    // delay allows for debounce
    if (!Startup_Mode)
    {
        if (digitalRead(Button_Pin) == Digitial_Input_Active)
        {
            while (digitalRead(Button_Pin) == Digitial_Input_Active)
            {
                // just wait until button released
                Calibration_Mode = true;
            }
        }
    }
    // cant have both demo mode and calibration mode at once
    if (Calibration_Mode)
        Demo_Mode = false;


}    // End void Check_Button


// ##################################################################################################################################################


void Update_Dim_Status()
{


    // =======================================================
    // Dim display when headlights on
    // =======================================================

    if (Light_Status < 3)
    // Normal colours when headlights are off
    {
        Dim_Mode          = false;
        Text_Colour1      = LCD_WHITE;
        Text_Colour2      = LCD_LIGHTGREY;
        Block_Fill_Colour = LCD_GREY;
    }
    else
    {
        Dim_Mode          = true;
        Text_Colour1      = LCD_LIGHTGREY;
        Text_Colour2      = LCD_DARKGREY;
        Block_Fill_Colour = LCD_BLACK;
    }


}    // End void Update_Dim_Status


// ##################################################################################################################################################


void Check_Oil()
{

    // =======================================================
    // Process the oil pressure here
    // =======================================================

    if (digitalRead(Oil_Press_Pin) == Digitial_Input_Active)
    {
        Status_Priority    = 6;
        Status_Change_Time = millis();
        // Immediately call the routines to update NeoPixels and status text
        ShiftLight_Strip();
        Display_Warning_Text();
    }
    else
    {
        // Unset the Oil Pressure warning sound and text if Oil Pressure becomes good
        if (Status_Priority == 6)
            Status_Priority = 0;
    }


}    // end void Check_Oil


// ##################################################################################################################################################


void Update_Fuel()
{

    // =======================================================
    // Get the Fuel level and display it
    // =======================================================

    // Read the analog pin
    Raw_Value = Get_Sensor_Val(Fuel_Pin);

    if (Calibration_Mode)
    // show raw calibration values
    {
        Fuel_Litres = Raw_Value;
        itoa(Fuel_Litres, reusable_string, 10);
        if (Fuel_Litres < 10)
            strcat(reusable_string, " ");
        if (Fuel_Litres < 100)
            strcat(reusable_string, " ");
        if (Fuel_Litres < 1000)
            strcat(reusable_string, " ");
        my_lcd.Set_Text_Back_colour(LCD_BLACK);
        my_lcd.Set_Text_Size(3);
        my_lcd.Set_Text_colour(LCD_WHITE);
        my_lcd.Print_String(reusable_string, Fuel_Bar_X - 30, Fuel_Bar_Y - (Bar_Height / 2));
    }

    if (Demo_Mode)
    {
        // ----------------- FOR TESTING -----------------
        //Fuel_Litres = 20;
        Fuel_Litres = random(Fuel_Min - 5, Fuel_Max + 10);
        Fuel_Litres = constrain(Fuel_Litres, Fuel_Min, Fuel_Max);
        // -----------------------------------------------
    }

    if (!Demo_Mode && !Calibration_Mode)
    {
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

    if (!Calibration_Mode)
    {
        // Limit the change between readings
        // to combat fuel sloshing
        Fuel_Litres      = constrain(Fuel_Litres, Last_Fuel_Litres - 5, Last_Fuel_Litres + 5);
        Last_Fuel_Litres = Fuel_Litres;

        // Change text colours depending on remaining fuel
        Warn_Text_Colour = LCD_WHITE;
        if (Fuel_Litres < Warning_Fuel * 1.4)
            Warn_Text_Colour = LCD_YELLOW;
        if (Fuel_Litres < Warning_Fuel)
        {
            Status_Change_Time = millis();
            Status_Priority    = 4;
            Warn_Text_Colour   = LCD_RED;
        }

        if (Fuel_Litres >= Warning_Fuel)
        {
            // Unset the fuel warning status if it was already set
            if (Status_Priority == 4)
                Status_Priority = 0;
        }

        // Print value using warning text colour
        itoa(Fuel_Litres, reusable_string, 10);
        if (Fuel_Litres < 10)
            strcat(reusable_string, " ");
        my_lcd.Set_Text_Back_colour(LCD_BLACK);
        my_lcd.Set_Text_Size(2);
        my_lcd.Set_Text_colour(Warn_Text_Colour);
        my_lcd.Print_String(reusable_string, Fuel_Bar_X - 30, Fuel_Bar_Y - (Bar_Height / 2));

        // Draw fuel meter
        new_val = map(Fuel_Litres, Fuel_Min, Fuel_Max, Meter_Min, Meter_Max);
        Bar_Meter(new_val, Meter_Min, Meter_Max, Fuel_Bar_X, Fuel_Bar_Y, Bar_Width, Bar_Height, RED2BLUE);
    }


}    // End void Update_Fuel



// ##################################################################################################################################################



void Update_Temperature()
{


    // =======================================================
    // Get the Temperature and display it
    // =======================================================

    // Using DS8B20 for temperature
    if (Use_DS18B20 && !Missing_DS18B20)
    {
        sensor.requestTemperatures();
        while (!sensor.isConversionComplete())
            ;    // just wait until a valid temperature
        Raw_Value = sensor.getTempC();
        if (Raw_Value == -127)
        {
            // Sensor fault
            Missing_DS18B20 = true;
        }
    }
    else
    {
        // Using any other sensor
        // Read the analog pin
        Raw_Value = Get_Sensor_Val(Temp_Pin);
    }

    if (Calibration_Mode)
    {
        // show raw calibration values
        Temp_Celsius = Raw_Value;

        itoa(Temp_Celsius, reusable_string, 10);
        if (Temp_Celsius < 10)
            strcat(reusable_string, " ");
        if (Temp_Celsius < 100)
            strcat(reusable_string, " ");
        if (Temp_Celsius < 1000)
            strcat(reusable_string, " ");
        my_lcd.Set_Text_Back_colour(LCD_BLACK);
        my_lcd.Set_Text_Size(3);
        my_lcd.Set_Text_colour(LCD_WHITE);
        my_lcd.Print_String(reusable_string, Temp_Bar_X + Bar_Width - 30, Temp_Bar_Y - (Bar_Height / 2));
    }

    if (Demo_Mode)
    {
        // ----------------- FOR TESTING -----------------
        //Temp_Celsius = 15;
        Temp_Celsius = random(Temp_Min - 10, Temp_Max + 10);
        Temp_Celsius = constrain(Temp_Celsius, Temp_Min, Temp_Max);
        // -----------------------------------------------
    }

    if (!Demo_Mode && !Calibration_Mode)
    {
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

        if (Use_DS18B20)
        {
            // stay within 0 to 125 celcius, so we dont have to process negative numbers
            Temp_Celsius = constrain(Raw_Value, 0, 125);
            if (Missing_DS18B20)
                Temp_Celsius = Alert_Temp;
        }
        else
        {
            // Scale the anolog reading to a real celcius value
            // this also has the effect of being a Constraint
            Temp_Celsius = multiMap<int>(Raw_Value, temp_cal_in, temp_cal_out, temp_sample_size);
        }

        // ----------------- End real measurement ------
    }

    if (!Calibration_Mode)
    {

        // Change text colours depending on temperature
        if (Status_Priority == 5)
            Status_Priority = 0;
        Warn_Text_Colour = LCD_WHITE;
        if (Temp_Celsius <= Low_Temp)
            Warn_Text_Colour = LCD_CYAN;
        if (Temp_Celsius >= Fan_On_Temp)
            Warn_Text_Colour = LCD_YELLOW;
        if (Temp_Celsius >= Alert_Temp)
        {
            Status_Priority    = 5;
            Status_Change_Time = millis();
            Warn_Text_Colour   = LCD_RED;
        }

        // Print value using warning text colour
        itoa(Temp_Celsius, reusable_string, 10);
        if (Temp_Celsius < 10)
            strcat(reusable_string, " ");
        if (Temp_Celsius < 100)
            strcat(reusable_string, " ");
        my_lcd.Set_Text_Back_colour(LCD_BLACK);
        my_lcd.Set_Text_Size(2);
        my_lcd.Set_Text_colour(Warn_Text_Colour);
        my_lcd.Print_String(reusable_string, Temp_Bar_X + Bar_Width + 10, Temp_Bar_Y - (Bar_Height / 2));

        // Draw temperature meter
        // and keep within a reasonable range
        // even though DS18B20 digits may be outside that range
        new_val = map(Temp_Celsius, Temp_Min, Temp_Max, Meter_Min, Meter_Max);
        Bar_Meter(new_val, Meter_Min, Meter_Max, Temp_Bar_X, Temp_Bar_Y, Bar_Width, Bar_Height, BLUE2RED);
    }


}    // End void Update_Temperature



// ##################################################################################################################################################



void Control_Fan()
{


    // =======================================================
    // Turn the fan on or off
    // =======================================================

    // Use the temperature to control fan
    // skip the fan control in calibration mode and leave it off

    my_lcd.Set_Text_Size(2);
    my_lcd.Set_Text_Back_colour(LCD_BLACK);

    // Turn on the fan and remember the time whenever temp is too high
    if (Temp_Celsius >= Fan_On_Temp)
    {
        // set the Fan_On_Time every loop until the temp drops
        // ensure the relay is on
        digitalWrite(Relay_Pin, Fan_On);
        Fan_On_Time = millis();
        if (Status_Priority != 5)
            Status_Priority = 3;
        Status_Change_Time = millis();
    }

    // Turn off the fan when lower than Fan_On_Temp degrees and past the Hysteresis time
    if ((Temp_Celsius <= Fan_Off_Temp) && (millis() >= (Fan_On_Time + Fan_On_Hyst)))
    {
        // ensure the relay is off
        digitalWrite(Relay_Pin, !Fan_On);
        if (Status_Priority == 3)
            Status_Priority = 0;
    }


}    // end void Control_Fan



// ##################################################################################################################################################



void Check_Voltages()
{


    // =======================================================
    // Get voltage and alternator indicator for comparison
    // =======================================================

    // Read analog pins
    // do a read and ignore it, allowing the true value to settle
    Raw_Battery_Volts    = Get_Sensor_Val(Batt_Volt_Pin);
    Raw_Alternator_Volts = Get_Sensor_Val(Alternator_Pin);

    // Use raw calibration values
    if (Calibration_Mode)
        Battery_Volts = Raw_Battery_Volts;

    if (Demo_Mode)
    {
        // ----------------- FOR TESTING -----------------
        //Battery_Volts = 13.8;
        Battery_Volts    = (140 + random(0, 20) - random(0, 20)) / 10.0;
        Alternator_Volts = Battery_Volts + random(0, 2) - random(0, 10);
        // -----------------------------------------------
    }

    if (!Demo_Mode && !Calibration_Mode)
    {
        // ----------------- FOR REAL -----------------
        Battery_Volts    = float(Raw_Battery_Volts) * Input_Multiplier;
        Alternator_Volts = float(Raw_Alternator_Volts) * Input_Multiplier;
        // -----------------------------------------------
        Battery_Volts    = constrain(Battery_Volts, 5.0, 18.0);
        Alternator_Volts = constrain(Alternator_Volts, 5.0, 18.0);
    }



    if ((millis() - Status_Change_Time) > (Long_Loop_Interval * 2))
    {
        // wait until other warnings have had a chance to be displayed
        // voltages all good
        if ((Alternator_Volts >= (Battery_Volts * 0.9)) && (Battery_Volts < Volts_High))
        {
            Status_Priority = 0;
        }
        // battery voltage out of range
        if (Battery_Volts <= Volts_Low || Battery_Volts >= Volts_High)
        {
            Status_Priority = 1;
        }
        // alternator not working
        if ((Alternator_Volts < (Battery_Volts * 0.9)) && Startup_Mode == false)
        {
            Status_Priority = 2;
        }
    }


}    // End void Check_Voltages


// ##################################################################################################################################################


void Display_Warning_Text()
{


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

    if (Calibration_Mode)
    {
        my_lcd.Set_Text_colour(LCD_WHITE);
        my_lcd.Set_Text_Back_colour(LCD_BLACK);
        itoa(Battery_Volts, reusable_string, 10);
        strcat(reusable_string, " V    ");
        my_lcd.Print_String(reusable_string, Status_Text_X, Status_Text_Y);
    }
    else
    {
        switch (Status_Priority)
        {
            // start of the switch statement
            // in reverse order so highest priority is executed first
            case 6:
                // Bad oil pressue
                digitalWrite(OP_Warning_Pin, Valid_Warning);
                my_lcd.Set_Text_colour(LCD_RED);
                my_lcd.Set_Text_Back_colour(LCD_YELLOW);
                my_lcd.Print_String("   OIL  ", Status_Text_X, Status_Text_Y);
                break;
            case 5:
                // Over temperature
                digitalWrite(OP_Warning_Pin, Valid_Warning);
                my_lcd.Set_Text_colour(LCD_RED);
                my_lcd.Set_Text_Back_colour(LCD_BLACK);
                my_lcd.Print_String("  TEMP! ", Status_Text_X - 8, Status_Text_Y);
                break;
            case 4:
                // Low fuel
                digitalWrite(Warning_Pin, Valid_Warning);
                my_lcd.Set_Text_colour(LCD_YELLOW);
                my_lcd.Set_Text_Back_colour(LCD_BLACK);
                my_lcd.Print_String("  FUEL  ", Status_Text_X, Status_Text_Y);
                break;
            case 3:
                // Fan is on
                my_lcd.Set_Text_colour(Text_Colour2);
                my_lcd.Set_Text_Back_colour(LCD_BLACK);
                my_lcd.Print_String(" FAN ON ", Status_Text_X, Status_Text_Y);
                break;
            case 2:
                // Bad alternator
                digitalWrite(Warning_Pin, !Valid_Warning);
                my_lcd.Set_Text_colour(LCD_ORANGE);
                my_lcd.Set_Text_Back_colour(LCD_BLACK);
                my_lcd.Print_String(" CHARGE ", Status_Text_X, Status_Text_Y);
                break;
            case 1:
                // Low voltage
                digitalWrite(Warning_Pin, Valid_Warning);
                my_lcd.Set_Text_colour(LCD_YELLOW);
                my_lcd.Set_Text_Back_colour(LCD_BLACK);
                dtostrf(Battery_Volts, 5, 1, reusable_string);
                strcat(reusable_string, " v  ");
                my_lcd.Print_String(reusable_string, Status_Text_X, Status_Text_Y);
                break;
            case 0:
                // All good
                // unset warning sound pins
                digitalWrite(Warning_Pin, !Valid_Warning);
                digitalWrite(OP_Warning_Pin, !Valid_Warning);
                my_lcd.Set_Text_colour(Text_Colour2);
                my_lcd.Set_Text_Back_colour(LCD_BLACK);
                dtostrf(Battery_Volts, 5, 1, reusable_string);
                strcat(reusable_string, " v  ");
                my_lcd.Print_String(reusable_string, Status_Text_X, Status_Text_Y);
                break;
            default:
                // dont do anything
                break;
        }    // the end of the switch statement.
    }


}    // end void Display_Warning_Text



// ##################################################################################################################################################



void ShiftLight_Strip()
{


    // =======================================================
    // Operate the NeoPixel strip
    // =======================================================

    // Usage
    // strip.fill(Colour, Position, Count);

    // Set low brightness if headlights are on
    if (Dim_Mode)
    {
        strip.setBrightness(LED_Dim);
    }
    else
    {
        strip.setBrightness(LED_Bright);
    }

    if (Demo_Mode)
    {
        // ----------------- FOR TESTING ----------------

        if (Count_Up)
        {
            RPM_LED_Pos = RPM_LED_Pos + 1;
            if (RPM_LED_Pos > LED_Count)
            {
                Count_Up = false;
            }
        }
        if (!Count_Up)
        {
            RPM_LED_Pos = RPM_LED_Pos - 1;
            if (RPM_LED_Pos < 0)
            {
                Count_Up = true;
            }
        }

        //RPM_LED_Pos = random(-LED_Count, LED_Count);
        // ----------------------------------------------
    }
    else
    {
        // ------------------ FOR REAL ------------------
        // Read the serial input for number of LEDs to light
        // 0 = no LEDs lit
        // 1 -> {LED_Count} = number of LEDs lit
        if (Serial2.available() > 0)
        {
            RPM_LED_Pos = Serial2.read();
        }
        // ----------------------------------------------
    }

    RPM_LED_Pos = constrain(RPM_LED_Pos, 0, LED_Count);

    // display LED_Pos for debugging
    /*
    my_lcd.Set_Text_Back_colour(LCD_BLACK);
    my_lcd.Set_Text_colour(LCD_WHITE);
    my_lcd.Set_Text_Size(2);
    my_lcd.Print_Number_Int(RPM_LED_Pos, CENTER, LCD_Offset_Y + 400, 1, ' ', 10);
    */

    // Display RPM on the WS2812 LED strip for shiftlight function
    // Set the colour based on how many LEDs will be illuminated
    // Colour is chosen from the array
    strip.fill(LED_Colour[RPM_LED_Pos], 0, RPM_LED_Pos);

    // Set unused pixels to off (black)
    // Unless one of the following sections sets the last LED it will remain off
    if (RPM_LED_Pos < LED_Count)
        strip.fill(0, RPM_LED_Pos, LED_Count - RPM_LED_Pos - 1);

    /*
      Lights_NotChanged = 0;
      Lights_Off  = 1;
      Lights_Park = 2;
      Lights_Low  = 3;
      Lights_High = 4;
    */

    // Display headlight status on the last LED
    //if (PL_CurrentStatus == HIGH || LB_CurrentStatus == HIGH)
    if (Light_Status > 1 && Light_Status < 4)
    {
        // Low beam or Parkers
        // Set end LED to dark green
        strip.fill(0x009900, LED_Count - 1, 1);
    }
    //if (HB_CurrentStatus == HIGH)
    if (Light_Status > 3)
    {
        // High beam
        // Set end LED to dark blue
        strip.fill(0x000099, LED_Count - 1, 1);
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
    switch (Status_Priority)
    {
        // start of the switch statement
        // in reverse order so highest priority is executed first
        case 6:
            // Oil Pressure
            // Set all LEDs to Red
            // with blinking
            strip.fill(0xFF0000, 0, LED_Count);
            strip.show();
            delay(200);
            strip.clear();
            strip.show();
            delay(200);
            strip.fill(0xFF0000, 0, LED_Count);
            strip.show();
            break;
        case 5:
            // Over temperature
            // Set all LEDs to Orange
            // with blinking
            strip.fill(0xCF4C00, 0, LED_Count);
            strip.show();
            delay(200);
            strip.clear();
            strip.show();
            delay(200);
            strip.fill(0xCF4C00, 0, LED_Count);
            strip.show();
            break;
        case 4:
            // Fuel Low
            // Set end LED to cyan
            //strip.fill(0x30CFCF, LED_Count - 1, 1);
            break;
        case 3:
            // Fan on
            // Set end LED to dark magenta
            //strip.fill(0x690069, LED_Count - 1, 1);
            break;
        case 2:
            // Alternator fault
            // Set end LED to dark orange
            //strip.fill(0xCF4C00, LED_Count - 1, 1);
            break;
        case 1:
            // Bad voltage
            // Set end LED to yellow
            //strip.fill(0xBDBD00, LED_Count - 1, 1);
            break;
        case 0:
            // nothing wrong
            // last LED should already be black unless it was specifically set
            break;
        default:
            // do nothing
            break;
    }    // the end of the switch statement.

    // Send the updated pixel info to the hardware
    strip.show();


}    // end void ShiftLight_Strip



// ##################################################################################################################################################



void Headlight_Status()
{


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
    if (Calibration_Mode)
    {
        // ignore the digital inputs
        Light_Status = Lights_Off;
        my_lcd.Set_Draw_color(LCD_GREY);
        my_lcd.Fill_Round_Rectangle(Light_Status_X, Light_Status_Y, Light_Status_X + Light_Status_Length, Light_Status_Y + Light_Status_Height, 5);
        my_lcd.Set_Text_Size(4);
        my_lcd.Set_Text_colour(LCD_WHITE);
        my_lcd.Set_Text_Back_colour(LCD_GREY);
        my_lcd.Print_String("CALIBR", Light_Status_X + 22, Light_Status_Y + 12);
    }
    else
    {
        // Get the light status from digital inputs
        if (digitalRead(Parker_Light_Pin) == Digitial_Input_Active)
        {
            Light_Status = Lights_Park;
        }
        else
        {
            // cant have any lights on at all without the parker lights being on
            Light_Status = Lights_Off;
        }
        if (digitalRead(Low_Beam_Pin) == Digitial_Input_Active)
            Light_Status = Lights_Low;
        if (digitalRead(High_Beam_Pin) == Digitial_Input_Active)
            Light_Status = Lights_High;

        // Draw the lights indicator
        // Only overwrite it if there has been a change
        if (Light_Status != Light_Last_Status)
        {
            switch (Light_Status)
            {    // start of the switch statement
                case Lights_NotChanged:
                    // do nothing
                    break;
                case Lights_Off:
                    my_lcd.Set_Draw_color(LCD_GREY);
                    my_lcd.Fill_Round_Rectangle(Light_Status_X, Light_Status_Y, Light_Status_X + Light_Status_Length, Light_Status_Y + Light_Status_Height, 5);
                    my_lcd.Set_Text_Size(4);
                    my_lcd.Set_Text_colour(LCD_BLACK);
                    my_lcd.Set_Text_Back_colour(LCD_GREY);
                    my_lcd.Print_String("LIGHTS", Light_Status_X + 22, Light_Status_Y + 12);
                    break;
                case Lights_Park:
                    my_lcd.Set_Draw_color(LCD_YELLOW);
                    my_lcd.Fill_Round_Rectangle(Light_Status_X, Light_Status_Y, Light_Status_X + Light_Status_Length, Light_Status_Y + Light_Status_Height, 5);
                    my_lcd.Set_Text_Size(4);
                    my_lcd.Set_Text_colour(LCD_BLACK);
                    my_lcd.Set_Text_Back_colour(LCD_YELLOW);
                    my_lcd.Print_String("PARK", Light_Status_X + 50, Light_Status_Y + 12);
                    break;
                case Lights_Low:
                    my_lcd.Set_Draw_color(LCD_GREEN);
                    my_lcd.Fill_Round_Rectangle(Light_Status_X, Light_Status_Y, Light_Status_X + Light_Status_Length, Light_Status_Y + Light_Status_Height, 5);
                    my_lcd.Set_Text_Size(4);
                    my_lcd.Set_Text_colour(LCD_BLACK);
                    my_lcd.Set_Text_Back_colour(LCD_GREEN);
                    my_lcd.Print_String("LOW", Light_Status_X + 60, Light_Status_Y + 12);
                    break;
                case Lights_High:
                    my_lcd.Set_Draw_color(LCD_BLUE);
                    my_lcd.Fill_Round_Rectangle(Light_Status_X, Light_Status_Y, Light_Status_X + Light_Status_Length, Light_Status_Y + Light_Status_Height, 5);
                    my_lcd.Set_Text_Size(4);
                    my_lcd.Set_Text_colour(LCD_BLACK);
                    my_lcd.Set_Text_Back_colour(LCD_BLUE);
                    my_lcd.Print_String("HIGH", Light_Status_X + 50, Light_Status_Y + 12);
                    break;
                default:
                    // do nothing
                    break;
            }    // the end of the switch statement.
        }        // end of light status changed
        Light_Last_Status = Light_Status;
    }    // end of calibration mode choice


}    // end void Headlight_Status


// ##################################################################################################################################################
// ##################################################################################################################################################


void Bar_Meter(int value, int vmin, int vmax, int x, int y, int w, int h, byte scheme)
{

    /*
    const int Bar_Height = 200;
    const int Bar_Width  = 60;
    */

    int block_colour;

    int v  = map(value, vmin, vmax, 0, Bar_Height);

    int y1 = y - v;
    int y2 = y - Bar_Height;

    if (!Dim_Mode)
    {
        // Choose colour from scheme
        switch (scheme)
        {
            case 0: block_colour = LCD_RED; break;                                     // Fixed colour
            case 1: block_colour = LCD_GREEN; break;                                   // Fixed colour
            case 2: block_colour = LCD_BLUE; break;                                    // Fixed colour
            case 3: block_colour = rainbow(map(value, vmin, vmax, 0, 127)); break;     // Blue to red
            case 4: block_colour = rainbow(map(value, vmin, vmax, 63, 127)); break;    // Green to red
            case 5: block_colour = rainbow(map(value, vmin, vmax, 127, 63)); break;    // Red to green
            case 6: block_colour = rainbow(map(value, vmin, vmax, 127, 0)); break;     // Red to blue
            default: block_colour = LCD_BLUE; break;                                   // Fixed colour
        }
    }
    else
    {
        block_colour = Text_Colour2;
    }

    // Fill in coloured blocks
    my_lcd.Set_Draw_color(block_colour);
    my_lcd.Fill_Rectangle(x, y, x + w, y1);

    // Fill in blank segments

    my_lcd.Set_Draw_color(Block_Fill_Colour);
    my_lcd.Fill_Rectangle(x, y1, x + w, y2);


}    // End void barmeter


// #########################################################################


int Get_Sensor_Val(byte Sensor_Pin)
{

    // Arithmetic average
    Sensor_Average = 0;
    for (i = 1; i <= Sensor_Readings; i++)
    {
        Sensor_Average += analogRead(Sensor_Pin);
        delay(Sensor_Delay);
    }
    Sensor_Average = Sensor_Average / float(Sensor_Readings);

    return int(Sensor_Average + 0.5);

}    // end void Get_Sensor_Val


// ##########################################################################


unsigned int rainbow(byte value)
{
    // Value is expected to be in range 0-127
    // The value is converted to a spectrum colour from 0 = blue through to 127 = red

    byte red      = 0;    // Red is the top 5 bits of a 16 bit colour value
    byte green    = 0;    // Green is the middle 6 bits
    byte blue     = 0;    // Blue is the bottom 5 bits

    byte quadrant = value / 32;

    if (quadrant == 0)
    {
        blue  = 31;
        green = 2 * (value % 32);
        red   = 0;
    }
    if (quadrant == 1)
    {
        blue  = 31 - (value % 32);
        green = 63;
        red   = 0;
    }
    if (quadrant == 2)
    {
        blue  = 0;
        green = 63;
        red   = value % 32;
    }
    if (quadrant == 3)
    {
        blue  = 0;
        green = 63 - 2 * (value % 32);
        red   = 31;
    }
    return (red << 11) + (green << 5) + blue;
}


// ##################################################################################################################################################
// ##################################################################################################################################################

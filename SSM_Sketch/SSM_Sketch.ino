/* Solar Scintillation Seeing Monitor Sketch
 *  
 *  (Putative V1.0) Copyright (C) E.J Seykora, Department of Physics, East Carolina University, Greenville, NC, USA (Original Sketch, Mode 1 output)
 *  (Putative V1.1) Copyright (C) Joachim Stehle, info@jahreslauf.de (Mode 2 output, Firecapture Addin compatibility, LCD output)
 *  V1.2 Copyright (C) 2016 Ian Lauwerys, http://www.blackwaterskies.co.uk/ (Mode 3 output, OLED output, refactoring)
 *  
 *  Changelog:
 *    V1.0 - Vanilla version
 *    V1.1 - Firecapture addin and LCD shield output
 *    V1.2 - Refactored code, OLED output
 *  
 *  Features:
 *  - Reads NUM_SAMPLES of analog input readings from ADC1 and ADC2 to find  4.46 * RMS(Intensity_ADC2) / AVERAGE(Intensity_ADC1)   
 *  - Mode 1: Result is sent to serial monitor in plain text mode
 *  - Mode 2: Result is sent to serial monitor in format compatible with the Firecapture addin (http://www.joachim-stehle.de/sssm_eng.html)
 *  - Mode 3: (Not available) Reserved for bidirectional communications including client side configuration changes
 *  
 *  Notes:
 *  - Use with the hardware described in the paper linked above
 *  - External Scintillation Monitor circuit gain set to 425.5X
 *  - Adjust intensity value to approximately 1.0 in by changing feedback resistor on U1 substitute a 2K variable resistor for the 220R resistor in the paper linked above
 *  - For more information see the following references:
 *    http://www.joachim-stehle.de/sssm_eng.html
 *    http://solarchatforum.com/viewtopic.php?f=9&t=16746
 *  
*/

// *************************************** Begin user modifiable defines *************************************

#define NUM_SAMPLES 2000          // Number of samples used to find the RMS value, default 2000 for about 2 readings per second on 16MHz device, decrease to output more readings per second

#define ADC1 A0                   // Arduino analog input pin, intensity from LMC6484 pin 14
#define ADC2 A1                   // Arduino analog input pin, variation from LMC6484 pin 8

#define CLOUD_DISCRIMINATE false  // Discriminate for clouds if true, default false
#define DISCRIMINATE_LOW 0.5      // Set variation value to zero if intensity is too low, default 0.5
#define DISCRIMINATE_HIGH 10.0    // Set variation value to zerio if variation is too high, default 10

#define INTENSITY_OFFSET 0.0      // Intensity dc offset, default 0.0, should not need to change if resistor on U1 can be adjusted to keep value between 0.5V and 1.0V
#define VARIATION_OFFSET -0.5     // Variation dc offset, default -0.5, may need to adjust to keep variation output the the range > 0.0 and < 10.0

#define MODE 2                    // Select mode as described in features above, default 2, comment out if serial output not required (e.g. stand-alone device using LED/OLED for output)
#ifdef MODE
  #define SERIAL_RATE 115200      // Set the serial communications rate (9600 or 115200 are good values to try), default 115200
#endif

// #define LED_OUTPUT             // Define this constant to enable LED shield support, otherwise comment it out

#define OLED_OUTPUT               // Define this constant to enable OLED module support, otherwise comment it out
                                  // N.B. You must edit Adafruit_SSD1306.h at comment "SSD1306 Displays" to choose a display size
                                  // SSD1306_LCDWIDTH and SSD1306_LCDHeight will then be defined by the .h with display size
                                  // Current W x H options are: 128 x 64 | 128 x 32 | 96 x 16
#ifdef OLED_OUTPUT
  // #define SOFTWARE_SPI         // Define this constant if your OLED module is wired to use software SPI
  #define HARDWARE_SPI            // Define this constant if your OLED module is wired to use hardware SPI
  #if defined SOFTWARE_SPI && defined HARDWARE_SPI
    #error "Only one OLED SPI mode can be defined!"
  #endif 
  #ifdef SOFTWARE_SPI
    #define OLED_MOSI   9         // For software SPI only, define digital pin wired to OLED MOSI
    #define OLED_CLK   10         // For software SPI only, define digital pin wired to OLED CLK (or SLCK)
  #endif
  #define OLED_RESET  -1          // For software and hardware SPI, define digital pin wired to OLED RST (not available on my OLED module!)
  #define OLED_DC     10          // For software and hardware SPI, define digital pin wired to OLED DC (or D/C)
  #define OLED_CS     21          // For software and hardware SPI, define digital pin wired to OLED CS (21 is A3 on Pro Micro!)
  // Note that SCLK (Pro Micro pin 15) and MOSI (Pro Micro pin 16) are specific pins in hardware SPI mode so don't need defining
  #define IG_WIDTH 44             // Intensity graph width (pixels), default 44 (sized for 128 pixel wide display), comment out to hide graph
  #define VG_WIDTH 120            // Variation graph width (pixels), default 120 (sized for 128 pixel wide display), comment out to hide graph
                                  // N.B. Enabling variation graph requires display to be taller than 16 pixels
#endif

// *************************************** End user modifiable defines ***************************************

// *************************************** Begin private defines *********************************************

#define SSM_VERSION "V1.2"      // SSM sketch version number
#define SSM_DATE "02 Oct 2016"  // SSM sketch date
#define WHITE 1                 // For OLED output
#define BLACK 0                 // For OLED output

// *************************************** End private defines ***********************************************

#ifdef LED_OUTPUT
  include <LiquidCrystal.h>             // Library available in standard Arduino IDE
  LiquidCrystal lcd(8, 9, 4, 5, 6, 7);  // Set up LCD shield
#endif

#ifdef OLED_OUTPUT
  #include <SPI.h>              // Library available in standard Arduino IDE
  #include <Wire.h>             // Library available in standard Arduino IDE
  #include <Adafruit_GFX.h>     // https://github.com/adafruit/Adafruit-GFX-Library/archive/master.zip
  #include <Adafruit_SSD1306.h> // https://github.com/adafruit/Adafruit_SSD1306/archive/master.zip
  #ifdef SOFTWARE_SPI
    Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
  #endif
  #ifdef HARDWARE_SPI
    Adafruit_SSD1306 display(OLED_DC, OLED_RESET, OLED_CS);
  #endif
  #ifdef IG_WIDTH
    float intensityBuffer[IG_WIDTH] = { 0.0 };  // Buffer to hold intensity readings
    int intensityOldest = 0;                    // Current position of oldest (left hand of graph) entry in intensity buffer
  #endif
  ifdef VG_WIDTH
    float variationBuffer[VG_WIDTH] = { 0.0 };  // Buffer to hold variation readings
    int variationOldest = 0;                    // Current position of oldest (left hand of graph) entry in variation buffer
  #endif
#endif

void setup()
{
  #ifdef LED_OUTPUT
    lcd.begin(16, 2); // Initialise LCD shield
  #endif
  
  #ifdef OLED_OUTPUT
    display.begin(SSD1306_SWITCHCAPVCC);               // Generate high voltage.
    #define SSM_LOGO_WIDTH  56                         // SSM splash screen logo width
    #define SSM_LOGO_HEIGHT 16                         // SSM splash screen logo height
    static const unsigned char PROGMEM SSM_LOGO [] = { // SSM Splash screen logo
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x0F, 0x80, 0x7E, 0x0E, 0x07, 0x31, 0x8C,
      0x3F, 0xE1, 0xFF, 0x0F, 0x0F, 0x3B, 0xDC, 0x70, 0x63, 0x83, 0x0F, 0x0F, 0x1F, 0xF8, 0x60, 0x03,
      0x00, 0x0D, 0x9B, 0x1E, 0x78, 0x60, 0x03, 0x00, 0x0D, 0x9B, 0x0C, 0x30, 0x70, 0x03, 0x80, 0x0D,
      0x9B, 0x3C, 0x3C, 0x3F, 0x81, 0xFC, 0x0C, 0xF3, 0xFC, 0x3F, 0x0F, 0xE0, 0x7F, 0x0C, 0xF3, 0x7E,
      0x7E, 0x00, 0x70, 0x03, 0x8C, 0x63, 0x07, 0xE0, 0x00, 0x30, 0x01, 0x8C, 0x63, 0x07, 0xE0, 0x00,
      0x30, 0x01, 0x8C, 0x03, 0x0E, 0x70, 0x60, 0x73, 0x03, 0x8C, 0x03, 0x0C, 0x30, 0x7F, 0xE3, 0xFF,
      0x0C, 0x03, 0x0C, 0x30, 0x1F, 0x80, 0xFC, 0x0C, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    display.clearDisplay();                                                 // Clear display
    display.drawBitmap(0, 0, SSM_LOGO, SSM_LOGO_WIDTH, SSM_LOGO_HEIGHT, 1); // Draw splash screen logo
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(SSM_LOGO_WIDTH + 6, 0);                               // Draw splash screen sketch version
    display.print(SSM_VERSION);
    #if SSD1306_LCDWIDTH = 128
      #ifdef MODE
        display.print(" Mode ");                                            // Draw splash screen mode if applicable
        display.print(MODE);
      #endif
      display.setCursor(SSM_LOGO_WIDTH + 6 , 8);                            // Draw splash screen sketch date
      display.print(SSM_DATE);
    #endif
    display.display();                                                      // Show splash screen
    delay(2000);
    display.clearDisplay();                                                 // Clear display
    display.display();                                                      // Show cleared display
  #endif
  
  #ifdef MODE
    Serial.begin(115200); // Initialise serial communications
  #endif
}
 
void loop()
{
  // Note: Assign constants to variables so they may be changed on the fly by client software in mode 3
  int sampleCount,
      numSamples = NUM_SAMPLES  
      #ifdef MODE
        ,
        mode = MODE
      #endif
      ;

  float variationValue,
        intensityValue,
        variationOffset = VARIATION_OFFSET,
        intensityOffset = INTENSITY_OFFSET,
        discriminateLow = DISCRIMINATE_LOW,
        discriminateHigh = DISCRIMINATE_HIGH;

  boolean cloudDiscriminate = CLOUD_DISCRIMINATE;
  
  variationValue = intensityValue = 0;
  
  for (sampleCount=0; sampleCount < numSamples; sampleCount++) // Collect samples from the SSM
  {
    intensityValue = intensityValue + (analogRead(ADC1) - 511.) * 2.5 / 511.;     // Accumulate average solar intensity sample
    variationValue = variationValue + sq((analogRead(ADC2) - 511.) * 2.5 / 511.); // Accumulate RMS variation sample
  }
  
  intensityValue = intensityValue / numSamples + intensityOffset;                                 // Find average solar intensity, +/– dc offset
  variationValue = (4.46 * sqrt(variationValue / numSamples) + variationOffset) / intensityValue; // Find RMS variation, +/– small dc offset, normalised for intensity
  
  if (cloudDiscriminate && (intensityValue < discriminateLow || variationValue > discriminateHigh)) 
  {
    variationValue = 0; // Discriminate for clouds by setting variation value to zero
  }

  // http://www.daycounter.com/LabBook/Moving-Average.phtml

  #ifdef LED_OUTPUT // Output to LED shield
    lcd.setCursor(0, 1);
    lcd.print("Input:          ");
    lcd.setCursor(8, 1);
    lcd.print(intensityValue, 2);
    lcd.setCursor(0, 0);
    lcd.print("Seeing:         ");
    lcd.setCursor(8, 0);
    lcd.print(variationValue, 2);
  #endif

  #ifdef OLED_OUTPUT // Output to OLED module
    display.clearDisplay();             // Clear display
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);            // Draw seeing value
    display.print("Seeing: ");
    display.print(variationValue, 2);
    display.setCursor(0 , 8);           // Draw intensity value
    display.print("Input:  ");
    display.print(intensityValue, 2);
    
    #ifdef IG_WIDTH
      intensityBuffer[intensityOldest] = intensityValue;  // Buffer current intensity reading
      intensityOldest = intensityOldest++ % IG_WIDTH;     // Increment left hand of graph, wrap over at end of buffer
      // FIXME: Draw intensity graph
    #endif
    ifdef VG_WIDTH
      variationBuffer[variationOldest] = variationValue;  // Buffer current variation reading
      variationOldest = variationOldest++ % VG_WIDTH;     // Increment left hand of graph, wrap over at end of buffer
      // FIXME: Draw variation graph
    #endif
    display.display();                  // Show results
  #endif
  
  #ifdef MODE // Output to serial port in desired mode
    switch(mode)
    {
      case 1 :  // Basic text output
        Serial.print("Intensity: ");
        Serial.println(intensityValue, 2);
        Serial.print("Variation: ");
        Serial.println(variationValue, 2);
        break;
      
      case 2 :  // Firecapture addin compatible
        Serial.print("A0: ");
        Serial.println(intensityValue, 2);
        Serial.print("A1: ");
        Serial.println(variationValue, 2);
        break;
  
      case 3 :  // FIXME: Bidirectional comms, need to read commands and send data
        Serial.println("Mode 3 not implemented yet!");
        break;
      
      default : // Incorrect mode set
        Serial.println("Invalid output mode selected!");
    }
  #endif
}

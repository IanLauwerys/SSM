/* Solar Scintillation Seeing Monitor Sketch
 *  
 *  (Putative V1.0) Copyright (C) E.J Seykora, Department of Physics, East Carolina University, Greenville, NC, USA (Original Sketch, Mode 1 output)
 *  (Putative V1.1) Copyright (C) Joachim Stehle, info@jahreslauf.de (Mode 2 output, Firecapture Addin compatibility, LCD output)
 *  V1.2 Copyright (C) 2016 Ian Lauwerys, http://www.blackwaterskies.co.uk/ (Mode 3 output, OLED output, refactoring)
 *  
 *  Unfortunately due to the lack of license information in 
 *  
 *  V1.0 - Vanilla version
 *  V1.1 - Firecapture and LCD shield output
 *  V1.2 - Refactored and improved code, OLED output
 *  
 *  Features:
 *  - Reads NUM_SAMPLES of analog input readings from ADC1 and ADC2 to find  4.46 * RMS(Intensity_ADC2) / AVERAGE(Intensity_ADC1)   
 *  - Mode 1: Result is sent to serial monitor in plain text mode
 *  - Mode 2: Result is sent to serial monitor in format compatible with the Firecapture addin (http://www.joachim-stehle.de/sssm_eng.html)
 *  - Mode 3: (Not available) Reserved for bidirectional communications including client side configuration changes
 *  
 *  Notes:
 *  - Use with the hardware described in the paper linked above.  
 *  - External Scintillation Monitor circuit gain set to 425.5X
 *  - Adjust intensity value to approximately 1.0 in by changing feedback resistor on U1 substitute a 2K variable resistor for the 220R resistor in the paper linked above.
 *  - For more information see the following references:
 *  http://www.joachim-stehle.de/sssm_eng.html
 *  http://solarchatforum.com/viewtopic.php?f=9&t=16746
 *  
*/

#define SSM_VERSION "V1.2"        // SSM sketch version number
#define SSM_DATE "02 Oct 2016"   // SSM sketch date
#define NUM_SAMPLES 2000         // Number of samples used to find the RMS value, default 2000 for about 2 readings per second on 16MHz device, decrease to output more readings per second
#define ADC1 A0                  // Arduino analog input pin, intensity from LMC6484 pin 14
#define ADC2 A1                  // Arduino analog input pin, variation from LMC6484 pin 8
#define CLOUD_DISCRIMINATE false // Discriminate for clouds if true
#define DISCRIMINATE_LOW 0.5     // Set variation value to zero if intensity is too low, default 0.5
#define DISCRIMINATE_HIGH 10.0   // Set variation value to zerio if variation is too high, default 10
#define INTENSITY_OFFSET 0.0     // Intensity dc offset, default 0.0, should not need to change if resistor on U1 can be adjusted to keep value between 0.5V and 1.0V
#define VARIATION_OFFSET -0.5    // Variation dc offset, default -0.5, may need to adjust to keep variation output the the range > 0.0 and < 10.0
#define MODE 2                   // Select mode as described in features above, default 2, comment out if serial output not required (e.g. stand-alone device using LED/OLED for output)
// #define LED_OUTPUT true       // Define this constant to enable LED shield support, otherwise comment it out
#define OLED_OUTPUT true         // Define this constant to enable OLED module support, otherwise comment it out
#define WHITE 1                  // For OLED output
#define BLACK 0                  // For OLED output

#ifdef LED_OUTPUT
  include <LiquidCrystal.h>
  LiquidCrystal lcd(8,9,4,5,6,7);  // Set up LCD shield
#endif

#ifdef OLED_OUTPUT
  #include <SPI.h>
  #include <Wire.h>
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h> // You must edit Adafruit_SSD1306.h to choose the appropriate display size for your OLED module

  /*
  // If using software SPI (the default case) uncomment this block and set pins appropriate to your OLED module:
  #define OLED_MOSI   9
  #define OLED_CLK   10
  #define OLED_DC    11
  #define OLED_CS    12
  #define OLED_RESET 13
  Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
  // End of software SPI block
  */
  
  // If using hardware SPI, uncomment this block and set pins appropriate to your OLED module
  // Note that SCLK (Pro Micro pin 15) and MOSI (Pro Micro pin 16) configure themselves in hardware mode:
  #define OLED_DC     10 // Digital pin 10
  #define OLED_CS     21 // Digital pin 21, aka A3 on Pro Micro pinout
  #define OLED_RESET  -1 // RST pin not available on my particular module
  Adafruit_SSD1306 display(OLED_DC, OLED_RESET, OLED_CS);
  // End of hardware SPI block
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
    display.setCursor(SSM_LOGO_WIDTH + 4, 0);                               // Draw splash screen sketch version
    display.print(SSM_VERSION);
    #ifdef MODE
      display.print(" Mode ");                                              // Draw splash screen mode if applicable
      display.print(MODE);
    #endif
    display.setCursor(SSM_LOGO_WIDTH + 4 , 8);                              // Draw splash screen sketch date
    display.print(SSM_DATE);
    display.display();                                                      // Show splash screen
    delay(2000);
    display.clearDisplay();                                                 // Clear display
    display.display();                                                    // Show cleared display
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
    display.display();                  // Show results
  #endif
  
  #ifdef MODE // Output to serial port in desired mode
    switch(mode)
    {
      case 1 :
        Serial.print("Intensity: ");
        Serial.println(intensityValue, 2);
        Serial.print("Variation: ");
        Serial.println(variationValue, 2);
        break;
      
      case 2 :
        Serial.print("A0: ");
        Serial.println(intensityValue, 2);
        Serial.print("A1: ");
        Serial.println(variationValue, 2);
        break;
  
      case 3 :
        Serial.println("Mode 3 not implemented yet!");
        break;
      
      default : /* Optional */
        Serial.println("No output mode selected!");
    }
  #endif
}

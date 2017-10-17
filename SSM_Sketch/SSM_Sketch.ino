/* Solar Scintillation Seeing Monitor Sketch
 *  
 *  (Putative V1.0) E.J Seykora, Department of Physics, East Carolina University, Greenville, NC, USA (Original Sketch, Mode 1 output)
 *  https://www.ecu.edu/cs-cas/physics/upload/An-Inexpensive-Solar-Scintillation-Seeing-Monitor-Circuit-with-Arduino-Interface-final2.pdf
 *  (Putative V1.1) Joachim Stehle, info@jahreslauf.de (Mode 2 output, Firecapture Addin compatibility, LCD output)
 *  http://www.joachim-stehle.de/sssm_eng.html
 *  V1.2 Modifications Copyright (C) 2016 Ian Lauwerys, http://www.blackwaterskies.co.uk/ (Mode 3 output, OLED output, code refactoring)
 *  
 *  The original work ("V1.0") and modified version "V1.1" contain no copyright notices or licences and thus are copyright their
 *  respective authors.
 *  
 *  Modifications (only) in V1.2 are provided without warranty of any kind and may be used freely and for any purpose (insofar as they can be
 *  separated from the original copyrighted works).
 *  
 *  Changelog:
 *    V1.0 - Vanilla version
 *    V1.1 - Firecapture addin and LCD shield output
 *    V1.2 - Refactored code, OLED output
 *  
 *  Features:
 *  - Reads NUM_SAMPLES of analog input readings from ADC1 and ADC2 to find  4.46 * RMS(Intensity_ADC2) / AVERAGE(Intensity_ADC1)   
 *  - Mode 1: Result is sent to serial port in plain text mode
 *  - Mode 2: Result is sent to serial port in format compatible with the Firecapture addin (http://www.joachim-stehle.de/sssm_eng.html)
 *  - Mode 3: (Not available) Reserved for bidirectional communications including client side configuration changes
 *  
 *  Notes:
 *  - Use with the hardware described in the paper linked above
 *  - External Scintillation Monitor circuit gain set to 425.5X
 *  - Adjust intensity value to approximately 1.0 in by changing feedback resistor on U1
 *      You should probably substitute a 2K variable resistor for the 220R resistor in the paper linked above to make this easier!
 *  - For more information see the following references:
 *  
 *    http://solarchatforum.com/viewtopic.php?f=9&t=16746
 *  
*/

// *************************************** Begin user modifiable defines *************************************

#define NUM_SAMPLES 2000      // ** Number of samples used to find the RMS value, default 2000 for about 2 readings per second on 16MHz device, decrease to output more readings per second

#define ADC1 A0               // ** Arduino analog input pin, reads intensity from LMC6484 pin 14
#define ADC2 A1               // ** Arduino analog input pin, reads variation from LMC6484 pin 8

// #define CLOUD_DISCRIMINATE true  // ** Discriminate for clouds if true, default true, comment out to disable
#ifdef CLOUD_DISCRIMINATE
  #define DISCRIMINATE_LOW     0.5  // ** Set variation value to zero if ***intensity*** is too low, default 0.5
  #define DISCRIMINATE_HIGH   10.0  // ** Set variation value to zero if ***variation*** is too high, default 10
#endif

#define INTENSITY_OFFSET  0.0  // ** Intensity dc offset, default 0.0, should not need to change if resistor on U1 can be adjusted to keep value between 0.5V and 1.0V
#define VARIATION_OFFSET -0.05 // ** Variation dc offset, default -0.05, may need to adjust to keep variation output the the range > 0.0 and < 10.0

#define MOVING_AVERAGE_PTS 20 // ** Number of points to use for calculating the variation moving average, default 20, comment out to disable

#define MODE 2                // ** Select mode as described in features above, default 2, comment out if serial output not required (e.g. stand-alone device using LED/OLED for output)
#ifdef MODE
  #define SERIAL_RATE 115200  // ** Set the serial communications rate (9600 or 115200 are good values to try), default 115200
#endif

// #define LED_OUTPUT         // ** Define this to enable LED shield support, otherwise comment it out
// #define OLED_OUTPUT        // ** Define this to enable OLED module support, otherwise comment it out
                              //    N.B. You must edit Adafruit_SSD1306.h at comment "SSD1306 Displays" to choose a display size
                              //    SSD1306_LCDWIDTH and SSD1306_LCDHeight will then be defined by the .h with display size
                              //    Current W x H options are: 128 x 64 | 128 x 32 | 96 x 16
#ifdef OLED_OUTPUT
  // #define SOFTWARE_SPI     // ** Define this if your OLED module is wired to use software SPI
  #define HARDWARE_SPI        // ** Define this if your OLED module is wired to use hardware SPI
  #if defined (SOFTWARE_SPI) && defined (HARDWARE_SPI)
    #error "Only one OLED SPI mode can be defined!"
  #elif !defined (SOFTWARE_SPI) && !defined (HARDWARE_SPI)
    #error "An OLED SPI mode must be defined!"
  #endif 
  #ifdef SOFTWARE_SPI
    #define OLED_MOSI   9     // ** For software SPI only, define digital pin wired to OLED MOSI
    #define OLED_CLK   10     // ** For software SPI only, define digital pin wired to OLED CLK (or SLCK)
  #endif
  #define OLED_RESET   -1     // ** For software and hardware SPI, define digital pin wired to OLED RST (not available on my OLED module, -1 ignores)
  #define OLED_DC      10     // ** For software and hardware SPI, define digital pin wired to OLED DC (or D/C)
  #define OLED_CS      21     // ** For software and hardware SPI, define digital pin wired to OLED CS (21 is A3 on Pro Micro!)
  // Note that SCLK (Pro Micro pin 15) and MOSI (Pro Micro pin 16) are specific pins in hardware SPI mode so don't need defining

  #if SSD1306_LCDHeight != 16 // Can't plot graphs on 96 x 16 screen
    #define IG_WIDTH   45     // ** Intensity graph width (pixels), default 45 (sized for 128 pixel wide display), comment out to hide graph
    #ifdef IG_WIDTH
      #define IG_MAX    1.5   // ** Intensity graph max plottable input value, default 1.5
    #endif

    #define VG_WIDTH  125     // ** Variation graph width (pixels), default 125 (sized for 128 pixel wide display), comment out to hide graph
    #ifdef VG_WIDTH           // N.B. Enabling the variation graph requires display to be taller than 16 pixels
      #define VG_MAX   20     // ** Variation graph max plottable input value, default 20
    #endif
    
    #define SCATTER_PLOT      // ** Define this to graph as a scatter plot (quicker to draw), comment out and a filled graph will be drawn
  #endif
#endif

// *************************************** End user modifiable defines ***************************************

// #define DEBUG_SSM               // Turn on debugging tests

#define SSM_VERSION "V1.2"      // SSM sketch version number
#define SSM_DATE "02 Oct 2016"  // SSM sketch date

// Note: Assign constants to variables so they may be changed on the fly by client software in mode 3
int sampleCount,
    numSamples = NUM_SAMPLES;

float variationValue,
      intensityValue,
      variationOffset = VARIATION_OFFSET,
      intensityOffset = INTENSITY_OFFSET;

#ifdef CLOUD_DISCRIMINATE
  float discriminateLow = DISCRIMINATE_LOW,
        discriminateHigh = DISCRIMINATE_HIGH;

  boolean cloudDiscriminate = CLOUD_DISCRIMINATE;
#endif

#ifdef MOVING_AVERAGE_PTS
  int movingAveragePts = MOVING_AVERAGE_PTS,
      movingAverageCount = 0;
      
  float movingAverageAcc = 0,
        movingAverage = 0;
#endif

#ifdef MODE
  int mode = MODE;
#endif

#ifdef LED_OUTPUT
  include <LiquidCrystal.h>             // Library available in standard Arduino IDE
  LiquidCrystal lcd(8, 9, 4, 5, 6, 7);  // Set up LCD shield
#endif

#ifdef OLED_OUTPUT
  int xPosition,
      yPosition,
      readingPosition;
  
  #include <SPI.h>              // Library available in standard Arduino IDE
  #include <Wire.h>             // Library available in standard Arduino IDE
  #include <Adafruit_GFX.h>     // https://github.com/adafruit/Adafruit-GFX-Library/archive/master.zip
  #include <Adafruit_SSD1306.h> // https://github.com/adafruit/Adafruit_SSD1306/archive/master.zip
  #ifdef SOFTWARE_SPI
    Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);  // Initialise display object
  #endif
  #ifdef HARDWARE_SPI
    Adafruit_SSD1306 display(OLED_DC, OLED_RESET, OLED_CS);                       // Initialise display object
  #endif
  
  #ifdef IG_WIDTH
    int intensityBuffer[IG_WIDTH] = { 0 },  // Buffer to hold scaled and bounded intensity readings
        intensityOldest = 0;                // Current position of oldest (left hand of graph) entry in intensity buffer

    #define IG_HEIGHT 13                    // Intensity graph height (pixels) - 1, default 13
    #define IG_SCALE  (IG_HEIGHT / IG_MAX)  // Intensity graph scale
  #endif
  
  #ifdef VG_WIDTH
    int variationBuffer[VG_WIDTH] = { 0 },  // Buffer to hold scaled and bounded variation readings
        variationOldest = 0;                // Current position of oldest (left hand of graph) entry in variation buffer

    #if SSD1306_LCDHEIGHT == 64
      #define VG_HEIGHT 45                // Variation graph height (pixels) - 1, default 45px for 64px high display
    #elif SSD1306_LCDHEIGHT == 32
      #define VG_HEIGHT 13                // Variation graph height (pixels) - 1, default 15px for 32px high display
    #else
      #error "Unsupported display size!"  // Couldn't find a supported display size
    #endif
    #define VG_SCALE (VG_HEIGHT / VG_MAX) // Variation graph scale
  #endif
#endif

void setup()
{
  #ifdef MODE
    Serial.begin(115200); // Initialise serial communications first in case we need to debug anything
  #endif
  
  #ifdef LED_OUTPUT
    lcd.begin(16, 2); // Initialise LCD shield
  #endif
  
  #ifdef OLED_OUTPUT
    delay(2000);  // Wait a bit for the display to initialise on first power up, otherwise we may get a blank screen instead of output
    
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
    display.setTextWrap(false);
    display.setCursor(SSM_LOGO_WIDTH + 6, 0);                               // Draw splash screen sketch version
    display.print(SSM_VERSION);
    #if SSD1306_LCDWIDTH == 128
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
}
 
void loop()
{
  variationValue = intensityValue = 0;                           // Reset variation and intensity samples

  for (sampleCount = 0; sampleCount < numSamples; sampleCount++) // Collect samples from the SSM
  {
    intensityValue = intensityValue + (analogRead(ADC1) - 511.) * 2.5 / 511.;     // Accumulate average solar intensity sample
    variationValue = variationValue + sq((analogRead(ADC2) - 511.) * 2.5 / 511.); // Accumulate RMS variation sample
  }
  
  intensityValue = intensityValue / numSamples + intensityOffset;                                 // Find average solar intensity, +/– dc offset
  variationValue = (4.46 * sqrt(variationValue / numSamples) + variationOffset) / intensityValue; // Find RMS variation, +/– small dc offset, normalised for intensity
  #ifdef MOVING_AVERAGE_PTS
    movingAverageAcc = movingAverageAcc + variationValue;   // Add the current variation to the moving average accumulator
    if (movingAverageCount = movingAveragePts)
    {
      movingAverageAcc = movingAverageAcc - movingAverage;  // Deduct the previous moving average from the accumulator
      movingAverage = movingAverageAcc / movingAveragePts;  // Calculate the new moving average
    }
    else
    {
      movingAverageCount++;                                 // Not enough data points yet, so keep accumulating
    }
  #endif
  
  #ifdef CLOUD_DISCRIMINATE
    if (cloudDiscriminate && (intensityValue < discriminateLow || variationValue > discriminateHigh)) 
    {
      variationValue = 0; // Discriminate for clouds by setting variation value to zero
    }
  #endif
  
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
      intensityBuffer[intensityOldest] = min(IG_HEIGHT, max(0, (int) round(intensityValue * IG_SCALE)));  // Buffer scaled and bounded intensity reading
      intensityOldest = (intensityOldest + 1) % IG_WIDTH;                                                 // Increment left hand of graph, wrap over at end of buffer
        
      // Draw intensity graph axes
      display.drawFastHLine(82, IG_HEIGHT, IG_WIDTH + 1, WHITE);              // X axis
      display.drawFastVLine(82, 0, IG_HEIGHT, WHITE);                         // Y axis
      display.drawFastHLine(80, IG_HEIGHT - (int) round(IG_SCALE), 2, WHITE); // Y axis "1" marker
      
      if (intensityValue > (IG_MAX))
      {
        display.setCursor(84, 0);
        display.print("HIGH");    // Print "HIGH" instead of graph
      }
      else if (intensityValue < 0)
      {
        display.setCursor(84, 0);
        display.print("LOW");     // Print "LOW" instead of graph
      }
      else
      {
        readingPosition = intensityOldest;                              // Start with the oldest intensity reading
        for (xPosition = 83; xPosition < (82 + IG_WIDTH); xPosition++)  // Plot intensity readings from left to right of graph
        {
          #ifdef SCATTER_PLOT
            display.drawPixel(xPosition, IG_HEIGHT - intensityBuffer[readingPosition], WHITE);                                        // Draw point
          #else
            display.drawFastVLine(xPosition, IG_HEIGHT - intensityBuffer[readingPosition], intensityBuffer[readingPosition], WHITE);  // Draw vertical bar
          #endif
          readingPosition = (readingPosition + 1) % IG_WIDTH;                                                                         // Move to next reading, wrap over at end of buffer
        }
      }
      
      #ifdef DEBUG_SSM // Test Intensity graph plot area limits
        display.drawLine(83, IG_HEIGHT - min(IG_HEIGHT, max(0, (int) round(IG_MAX * IG_SCALE))), (82 + IG_WIDTH), min(IG_HEIGHT, max(0, (int) round(IG_MAX * IG_SCALE))), INVERSE);  // Top left -> bottom right
        Serial.print("IG Top Left X: ");
        Serial.println(83);
        Serial.print("IG Top Left Y: ");
        Serial.println(IG_HEIGHT - min(IG_HEIGHT, max(0, (int) round(IG_MAX * IG_SCALE))));
        Serial.print("IG Bottom Right X: ");
        Serial.println((82 + IG_WIDTH));
        Serial.print("IG Bottom Right Y: ");
        Serial.println(min(IG_HEIGHT, max(0, (int) round(IG_MAX * IG_SCALE))));
      #endif
    #endif
    
    #ifdef VG_WIDTH
      variationBuffer[variationOldest] = min(VG_HEIGHT, max(0, (int) round(variationValue * VG_SCALE)));  // Buffer scaled and bounded variation reading
      variationOldest = (variationOldest + 1) % VG_WIDTH;                                                     // Increment left hand of graph, wrap over at end of buffer
      
      // Draw variation graph axes
      display.drawFastHLine(3, VG_HEIGHT + 18, VG_WIDTH + 1, WHITE); // X axis
      display.drawFastVLine(3, 17, VG_HEIGHT + 1, WHITE);            // Y axis
      #ifdef MOVING_AVERAGE_PTS
        if (movingAverageCount = movingAveragePts)
        {
          display.drawFastHLine(0, min(VG_HEIGHT, max(0, (int) round(movingAverage * VG_SCALE))) + 17, 2, WHITE); // Moving average marker
        }
      #endif
      
      // Plot variation graph
      readingPosition = variationOldest;                            // Start with the oldest variation reading
      for (xPosition = 3; xPosition < (2 + VG_WIDTH); xPosition++)  // Plot variation readings from left to right of graph
      {
        #ifdef SCATTER_PLOT
          display.drawPixel(xPosition, VG_HEIGHT - variationBuffer[readingPosition] + 17, WHITE); // Draw point
        #else
          display.drawFastVLine(xPosition, VG_HEIGHT - variationBuffer[readingPosition] + 17, variationBuffer[readingPosition], WHITE); // Draw vertical bar
        #endif
        readingPosition = (readingPosition + 1) % VG_WIDTH;                                                                                 // Move to next reading, wrap over at end of buffer
      }
    #endif
    
    #ifdef DEBUG_SSM // Test Variation graph plot area limits
      display.drawLine(3, VG_HEIGHT - min(VG_HEIGHT, max(0, (int) round(VG_MAX * VG_SCALE))) + 17, (2 + VG_WIDTH), min(VG_HEIGHT, max(0, (int) round(VG_MAX * VG_SCALE))) + 17, INVERSE);  // Top left -> bottom right
      Serial.print("VG Top Left X: ");
      Serial.println(3);
      Serial.print("VG Top Left Y: ");
      Serial.println(VG_HEIGHT - min(VG_HEIGHT, max(0, (int) round(VG_MAX * VG_SCALE))) + 17);
      Serial.print("VG Bottom Right X: ");
      Serial.println((2 + VG_WIDTH));
      Serial.print("VG Bottom Right Y: ");
      Serial.println(max(0, (int) round(VG_MAX * VG_SCALE)) + 17);
    #endif

    display.display();  // Show results
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

/*
 * Basic example of using an E-Ink display with ESP32
 * Using the GxEPD2 library with a 2.9" black and white display (GDEH029A1)
 */

// Include the correct headers
#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_BW.h> // For black/white displays
#include <Fonts/FreeMonoBold9pt7b.h>

// Uncomment the correct display driver for your E-Ink display
// This example is for GDEH029A1 2.9" b/w display
// See GxEPD2_Example.ino in the GxEPD2 library for more display options

// For 2.9" GDEH029A1 display
GxEPD2_BW<GxEPD2_290, GxEPD2_290::HEIGHT> display(GxEPD2_290(/*CS=*/ 5, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));

// Pin definitions for ESP32
// BUSY -> GPIO4, RST -> GPIO16, DC -> GPIO17, CS -> GPIO5, CLK -> GPIO18, DIN -> GPIO23

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 E-Ink Display Example");
  
  // Initialize the display
  display.init();
  
  // Set text color, size and font
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);
  
  // Display hello world text
  displayText("Hello World!", "ESP32 + E-Ink");
  
  Serial.println("Display updated - going to sleep");
  delay(3000); // Wait for serial output to complete
  
  // Deep sleep to save power - E-Ink retains image without power
  esp_deep_sleep_start();
}

void loop() {
  // Not used since we're going to deep sleep
}

void displayText(const char* line1, const char* line2) {
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  
  // Full screen update
  display.setFullWindow();
  
  // Use firstPage/nextPage for partial refresh displays
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    
    // Position and display first line
    display.getTextBounds(line1, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x = (display.width() - tbw) / 2;
    uint16_t y = (display.height() / 3);
    display.setCursor(x, y);
    display.print(line1);
    
    // Position and display second line
    display.getTextBounds(line2, 0, 0, &tbx, &tby, &tbw, &tbh);
    x = (display.width() - tbw) / 2;
    y = (display.height() * 2 / 3);
    display.setCursor(x, y);
    display.print(line2);
    
  } while (display.nextPage());
  
  Serial.println("Display updated");
}
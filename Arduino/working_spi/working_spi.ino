#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>

// CONSTRUCTOR: Using Page Buffer (_1_) and Hardware SPI (_HW_SPI)
// Pins: CS=10, DC=9, Reset=8. (SCL must be 13, SDA must be 11)
U8G2_SSD1363_256X128_1_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 7, /* dc=*/ 9, /* reset=*/ 8);

void setup() {
  u8g2.begin();
}

int counter = 0;

void loop() {
  // Page Buffer Loop: This saves ~3.5KB of RAM
  u8g2.firstPage();
  
  do {
    // Use a small, efficient font to save Flash space
    u8g2.setFont(u8g2_font_6x10_tf); 
    
    // The F() macro keeps the text out of your precious RAM
    u8g2.setCursor(0, 20);
    u8g2.print(F("System Online"));
    
    u8g2.setCursor(0, 40);
    u8g2.print(F("Resolution: 256x128"));
    u8g2.setCursor(0, 80);
    u8g2.print(counter++);
    
    // Draw a simple line (uses less space than complex shapes)
    u8g2.drawHLine(0, 85, 256);
  
  } while (u8g2.nextPage());
//delay(100);
}
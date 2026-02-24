#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>

// Your specific constructor (Pins 7, 8, 9, 11, 13)
U8G2_SSD1363_256X128_1_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 7, /* dc=*/ 9, /* reset=*/ 8);

char daisyData[256] = "0"; 

void setup() {
  Serial1.begin(115200); // CHANGE: Serial1 is used for Pins 0 and 1
  Serial.begin(115200);  // Optional: For debugging on your PC
  
  u8g2.begin();
  
  // Optional: reduce wait time if Daisy isn't sending data
  Serial1.setTimeout(10); 
}

void loop() {
  // Rapidly empty the serial buffer into our string
  while (Serial1.available() > 0) {
    char c = Serial1.read();
    
    static uint8_t idx = 0;
    if (c == '\n') { // End of message
      daisyData[idx] = '\0';
      idx = 0; 
    } else if (idx < sizeof(daisyData) - 1) {
      daisyData[idx++] = c;
    }
  }

  // Now do the slow drawing process
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_6x10_tf); 
    u8g2.setCursor(0, 20);
    u8g2.print(daisyData);
  } while (u8g2.nextPage());
}
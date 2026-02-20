#include <U8g2lib.h>
#include <SPI.h>

// Nano Constructor: Page Buffer (_1_) is key here.
U8G2_SSD1363_256X128_1_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 7, /* dc=*/ 9, /* reset=*/ 8);

int scene = 0;
unsigned long lastSwitch = 0;

void setup() {
  u8g2.begin();
}

void loop() {
  // Switch scenes every 3 seconds
  if (millis() - lastSwitch > 3000) {
    scene = (scene + 1) % 4;
    lastSwitch = millis();
  }

  u8g2.firstPage();
  do {
    switch (scene) {
      case 0: drawPrimitives(); break;
      case 1: drawTextDemo();   break;
      case 2: drawPatterns();   break;
      case 3: drawUIDemo();     break;
    }
  } while (u8g2.nextPage());
}

// --- SCENE 0: MATHEMATICAL PRIMITIVES ---
void drawPrimitives() {
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "1. PRIMITIVES");
  
  u8g2.drawFrame(10, 20, 40, 30);      // Empty Box
  u8g2.drawBox(60, 20, 40, 30);       // Filled Box
  u8g2.drawCircle(130, 35, 15);       // Circle
  u8g2.drawDisc(180, 35, 15);         // Filled Disc
  u8g2.drawTriangle(220, 50, 250, 50, 235, 20); // Triangle
  u8g2.drawLine(0, 60, 255, 60);      // Horizontal Line
}

// --- SCENE 1: TYPOGRAPHY (TEXT) ---
void drawTextDemo() {
  u8g2.drawStr(0, 10, "2. TYPOGRAPHY");
  
  u8g2.setFont(u8g2_font_ncenB14_tr); // Large Bold
  u8g2.drawStr(10, 40, "Bold 14pt");
  
  u8g2.setFont(u8g2_font_unifont_t_symbols); // Icons/Symbols
  u8g2.drawGlyph(10, 70, 0x2603); // Snowman
  u8g2.drawGlyph(30, 70, 0x2600); // Sun
  u8g2.drawGlyph(50, 70, 0x2615); // Coffee
  
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(10, 90);
  u8g2.print("Print: "); u8g2.print(millis()); // Dynamic values
}

// --- SCENE 2: PIXEL PATTERNS ---
void drawPatterns() {
  u8g2.drawStr(0, 10, "3. PIXEL MANIPULATION");
  
  // Dithered gradient effect (manual pixels)
  for(int x=0; x<256; x+=4) {
    for(int y=20; y<60; y+=4) {
      if ((x + y) % 8 == 0) u8g2.drawPixel(x, y);
    }
  }
  
  // High-speed scanlines
  for(int i=70; i<120; i+=2) u8g2.drawHLine(0, i, 255);
}

// --- SCENE 3: UI ELEMENTS ---
void drawUIDemo() {
  u8g2.drawStr(0, 10, "4. UI ELEMENTS");
  
  // Progress Bar
  u8g2.drawFrame(20, 30, 200, 15);
  int progress = (millis() / 20) % 196;
  u8g2.drawBox(22, 32, progress, 11);
  
  // Scrolling area
  u8g2.drawRFrame(20, 60, 200, 50, 5); // Rounded Frame
  u8g2.drawStr(30, 80, "CPU: 42%");
  u8g2.drawStr(30, 95, "TEMP: 34C");
}
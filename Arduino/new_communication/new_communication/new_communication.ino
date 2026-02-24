#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>

U8G2_SSD1363_256X128_1_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 7, /* dc=*/ 9, /* reset=*/ 8);

// --- Global Data Storage ---
uint8_t current_menu = 0;
uint8_t current_loc  = 0;
uint8_t sub_menu = 0;
uint8_t wave_amp[8], wave_shape[8], wave_phase[8], wave_preset;
bool fx_select; uint8_t fx_type, fx_val[3];
uint8_t adsr[4];

void setup() {
  Serial1.begin(115200); 
  u8g2.begin();
}

void loop() {
  if (Serial1.available() > 0) {
    if (Serial1.peek() == 0xAA) {
      if (Serial1.available() >= 3) {
        Serial1.read(); // Sync
        uint8_t m_id = Serial1.peek();
        int pLen = (m_id == 1) ? 2 : (m_id == 2) ? 28 : (m_id == 3 || m_id == 4) ? 6 : (m_id == 5) ? 3 : 0;

        if (pLen > 0 && Serial1.available() >= pLen) {
          current_menu = Serial1.read();
          current_loc  = Serial1.read();
          if (current_menu == 2) {
            sub_menu = Serial1.read();
            for(int i=0; i<8; i++) wave_amp[i]   = Serial1.read();
            for(int i=0; i<8; i++) wave_shape[i] = Serial1.read();
            for(int i=0; i<8; i++) wave_phase[i] = Serial1.read();
            wave_preset = Serial1.read();
            //wave_preset = 2;
          } 
          else if (current_menu == 3) {
            uint8_t packed = Serial1.read();
            fx_select = (packed >> 7) & 0x01;
            fx_type = packed & 0x7F;
            for(int i=0; i<3; i++) fx_val[i] = Serial1.read();
          }
          else if (current_menu == 4) {
            for(int i=0; i<4; i++) adsr[i] = Serial1.read();
          }
          else if (current_menu == 5) { Serial1.read(); }
        }
      }
    } else { Serial1.read(); }
  }

  // 2. DRAW TO SCREEN
  u8g2.firstPage();
  do {
    /*
    if (current_menu != 1 || current_menu != 4) { //CHANGE LATER
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(0, 12);
    u8g2.print("MENU: "); u8g2.print(current_menu);
    u8g2.print(" | LOC: "); u8g2.print(current_loc);
    u8g2.drawStr(0, 20, "--------------------------------");
    }
    */
    if (current_menu == 1) {
      // --- DRAW MAIN MENU (4 COLUMNS) ---
      uint8_t colW = 64; // 256 / 4 columns
      
      // Draw Column Dividers
      u8g2.setDrawColor(1);
      u8g2.drawLine(64, 0, 64, 128);
      u8g2.drawLine(128, 0, 128, 128);
      u8g2.drawLine(192, 0, 192, 128);

      // Titles
      u8g2.setFont(u8g2_font_7x14_tf); // Slightly bolder for headers
      u8g2.drawStr(14, 20, "WAVES");
      u8g2.drawStr(86, 20, "FX");
      u8g2.drawStr(148, 20, "ENV");
      u8g2.drawStr(210, 20, "STG");

      // Draw Icons (Placeholders based on your image)
      u8g2.drawHLine(10, 64, 44);   // Wave squiggle line
      u8g2.drawStr(82, 64, "[ o * ]"); // FX symbols
      u8g2.drawFrame(144, 58, 24, 14); // Envelope box
      u8g2.drawCircle(224, 64, 8);     // Settings gear center

      // --- Draw Highlight Bar ---
      // current_loc 0=Waves, 1=FX, 2=Env, 3=Stg
      u8g2.drawBox(current_loc * colW + 2, 115, colW - 4, 10);
    }
    
else if (current_menu == 2) {
      // Determine how many waves to show based on preset table
      int numWaves = 0;
      if (wave_preset >= 2 && wave_preset <= 3)      numWaves = 2;
      else if (wave_preset >= 4 && wave_preset <= 5) numWaves = 4;
      else if (wave_preset >= 6 && wave_preset <= 7) numWaves = 8;

      // Header
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.setCursor(0, 10);
      u8g2.print("WAVES: ["); u8g2.print(wave_preset); u8g2.print("]");
      u8g2.drawHLine(0, 13, 256);

      if (numWaves == 0) {
        u8g2.drawStr(80, 64, "NO WAVES ACTIVE");
      } else {
        int colW = 64;
        int rowH = (numWaves > 4) ? 55 : 110; // Split height if 8 waves

        for (int i = 0; i < numWaves; i++) {
          int col = i % 4;
          int row = i / 4;
          int xOff = col * colW;
          int yOff = 15 + (row * rowH);

          // 1. Handle Selection & Inversion
          // current_loc moves the bottom bar; sub_menu inverts the box
          bool isSelected = (sub_menu == i+1);
          if (isSelected) {
            u8g2.setDrawColor(1);
            u8g2.drawBox(xOff, yOff, colW, rowH);
            u8g2.setDrawColor(0); // Text will now be "punched out"
          } else {
            u8g2.setDrawColor(1);
            u8g2.drawFrame(xOff, yOff, colW, rowH);
          }

          // 2. Draw Wave Data
          u8g2.setFont(u8g2_font_t0_11_tf);
          u8g2.setCursor(xOff + 22, yOff + 12);
          u8g2.print("W"); u8g2.print(i);

          u8g2.setCursor(xOff + 5, yOff + 25);
          u8g2.print("A: "); u8g2.print(map(wave_amp[i], 0, 255, 0, 100)); u8g2.print("%");
          
          u8g2.setCursor(xOff + 5, yOff + 34);
          u8g2.print("S: "); 
          // Simple Shape mapping examples
          if(wave_shape[i] < 85) u8g2.print("~");      // Sine
          else if(wave_shape[i] < 170) u8g2.print("^"); // Saw/Tri
          else u8g2.print("#");                        // Square

          u8g2.setCursor(xOff + 5, yOff + 41);
          u8g2.print("P: "); u8g2.print(map(wave_phase[i], 0, 255, 0, 360)); u8g2.print("o");

          // 3. Draw Navigation Bar (only if this column matches current_loc)
          u8g2.setDrawColor(1); // Ensure bar is always visible
          if ((current_loc % 4) == col && (current_loc / 4) == row) {
             u8g2.drawBox(xOff + 5, yOff + rowH - 10, colW - 10, 6);
          }
          u8g2.setDrawColor(1); // Reset to default
        }
      }
    }
    
    else if (current_menu == 3) {
      u8g2.setCursor(0, 40);
      u8g2.print("FX ACTIVE: "); u8g2.print(fx_select ? "YES" : "NO");
      u8g2.setCursor(0, 55);
      u8g2.print("TYPE: "); u8g2.print(fx_type);
      u8g2.setCursor(0, 70);
      u8g2.print("VALS: "); u8g2.print(fx_val[0]); 
      u8g2.print(", "); u8g2.print(fx_val[1]); 
      u8g2.print(", "); u8g2.print(fx_val[2]);
    } 
    
    else if (current_menu == 4) {
      // 1. Setup Centered Large Text
      u8g2.setFont(u8g2_font_10x20_tf); // Larger, clearer font
      
      // Draw labels at the top, spread out to feel centered
      u8g2.setCursor(0, 15);  u8g2.print("Envelope");
      u8g2.setFont(u8g2_font_9x15_tf);
      u8g2.setCursor(10, 30);  u8g2.print("A:"); u8g2.print(adsr[0]);
      u8g2.setCursor(75, 30);  u8g2.print("D:"); u8g2.print(adsr[1]);
      u8g2.setCursor(140, 30); u8g2.print("S:"); u8g2.print(adsr[2]);
      u8g2.setCursor(205, 30); u8g2.print("R:"); u8g2.print(adsr[3]);

      // 2. Geometry for a Full-Screen Graph
      int baseY = 115; // Lowered to give text more room
      int peakY = 40;  // Height of the attack peak
      
      // We use a larger multiplier (0.7) to stretch the segments across the 256px width
      // Total potential width: (255 * 0.7) * 3 + 40 (sustain) ≈ 575 (too big)
      // So we use a factor of 0.6 and a smaller sustain to keep it within 256.
      float scale = 0.6; 
      int aW = (int)(adsr[0] * scale); 
      int dW = (int)(adsr[1] * scale);
      int sW = 30; // Fixed visual width for the sustain portion
      int rW = (int)(adsr[3] * scale);

      // Calculate total width to find the starting offset for centering
      int totalWidth = aW + dW + sW + rW;
      int startX = (256 - totalWidth) / 2; // Center the graph horizontally

      // Calculate Sustain Y level (0-255 mapped to baseY-peakY)
      int sY = map(adsr[2], 0, 255, baseY, peakY);

      // 3. Define Points
      int x0 = startX;           int y0 = baseY;
      int x1 = x0 + aW;          int y1 = peakY;
      int x2 = x1 + dW;          int y2 = sY;
      int x3 = x2 + sW;          int y3 = sY;
      int x4 = x3 + rW;          int y4 = baseY;

      // 4. Draw Graph Lines
      u8g2.setDrawColor(1);
      u8g2.drawLine(x0, y0, x1, y1); // Attack
      u8g2.drawLine(x1, y1, x2, y2); // Decay
      u8g2.drawLine(x2, y2, x3, y3); // Sustain
      u8g2.drawLine(x3, y3, x4, y4); // Release

      // 5. Visual "Nodes" (Makes it look like a pro UI)
      u8g2.drawDisc(x1, y1, 3); // Peak node
      u8g2.drawDisc(x2, y2, 3); // Sustain start node
      u8g2.drawDisc(x3, y3, 3); // Sustain end node
    }
    
  } while (u8g2.nextPage());
}
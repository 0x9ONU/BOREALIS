/***************************************************
//Web: http://www.buydisplay.com
EastRising Technology Co.,LTD
Examples for ER-OLEDM027-3
Display is Hardward SPI 4-Wire SPI Interface 
Tested and worked with:
Works with Arduino 1.6.0 IDE  
NOTE: testOK:DUE    If you use MEGA and UNO board, you need to add level conversion.
****************************************************/
 
/*
  == Hardware connection ==
    OLED   =>    Arduino
  *1. GND    ->    GND
  *2. VCC    ->    3.3
  *3. SCL    ->    SCK
  *4. SDA    ->    MOSI
  *5. RES    ->    8 
  *6. DC     ->    9
  *7. CS     ->    10
*/



#include "SPI.h"
#include "er_oled.h"

//uint8_t oled_buf[OLED_Y_MAXPIXEL * OLED_X_MAXPIXEL/2];

void setup() {
  Serial.begin(9600);
  Serial.print("OLED Example\n");

  /* display an image of bitmap matrix */
  er_oled_begin();

  
}

void loop() {
  
  

 //   er_oled_Fill(0,0,256,128,0x00);
 
  DrawBMP(0,0,256,128,pic0,1); //	reverse color:no         gray              
  delay(3000); 


  DrawBMP(0,0,256,128,pic1,1); //	reverse color:yes 	  gray 
  delay(3000); 

  DrawSingleBMP(0,0,256,128,pic2,0); //	reverse color:	  monochrome
  delay(3000); 

   
  DrawSingleBMP(0,0,256,128,pic3,1); // 	reverse color:  monochrome 
  delay(3000); 



  er_oled_Fill(0,0,320,160,0x00);
  er_oled_string(64,20,"www.buydisplay.com",16,0); 
  er_oled_string(64,50,"EastRising",24,1);
  er_oled_string(64,80,"Hollw!World",32,0); 

  uint8_t i;
  for(i=0;i<=48;i++)
  {
  command(0xa1); //start line
  data(i);
  delay(100);
  }
   for(i=48;i>0;i--)
  {
  command(0xa1); //start line
  data(i);
  delay(100);
  }
    delay(1000);
   command(0xa1); //start line
  data(0); 
  
}



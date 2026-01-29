/***************************************************
//Web: http://www.buydisplay.com
EastRising Technology Co.,LTD
****************************************************/
#include <SPI.h>
#include "er_oled.h"

void command(uint8_t cmd)
{
  digitalWrite(OLED_DC, LOW);
  digitalWrite(OLED_CS, LOW);
  SPI.transfer(cmd);
  digitalWrite(OLED_CS, HIGH);
}

void data(uint8_t dat)
{
  digitalWrite(OLED_DC, HIGH);
  digitalWrite(OLED_CS, LOW );
  SPI.transfer(dat);
  digitalWrite(OLED_CS, HIGH);
}

void er_oled_begin()
{

    pinMode(OLED_RST, OUTPUT);
    pinMode(OLED_DC, OUTPUT);
    pinMode(OLED_CS, OUTPUT);
    SPI.begin();
    
    SPI.setClockDivider(SPI_CLOCK_DIV2);
    
    digitalWrite(OLED_CS, LOW);
    digitalWrite(OLED_RST, HIGH);
    delay(10);
    digitalWrite(OLED_RST, LOW);
    delay(10);
    digitalWrite(OLED_RST, HIGH);
    delay(100);
 
   	command(0xfd);  /*Command Lock*/ 
	data(0x12);

	command(0xAE);

	command(0xC1);  
	data(0xA0);	

	command(0xA0);  
	data(0x32);	
	data(0x00);	
	command(0xa2);////A2.Display Offset
	data(0x20); ////Offset:: 0~127		
	/*
        command(0xA0);  
	data(0x24);	
	data(0x00);	
	command(0xa2);////A2.Display Offset
	data(0x80); ////Offset:: 0~127	*/	

	
	command(0xca);////CA.Set Mux Ratio
	data(0x7f); ////Mux:: 0~255

	command(0xad); ////AD.Set IREF
	data(0x90); ////Select:: Internal

	command(0xb3); ////B3.Set Display Clock Divide Ratio/Oscillator Frequency
	data(0x61); ////DivClk:: 0~255

	command(0xb9); ////B9 Default GAMMA

	command(0xAF); //Set Display On

    delay(10);
}


void Column_Address(uint8_t a,uint8_t b)
{
 
    command(0x15);      // Set Column Address
    data(a+8);
    data(b+8);

    
}

void Row_Address(uint8_t a,uint8_t b)
{
    command(0x75);      // Row  Address
    data(a);
    data(b);
    command(0x5C);    //WRITE RAM   
}


void er_oled_Fill(uint16_t x1,uint8_t y1,uint16_t x2,uint8_t y2,uint8_t color)
{
    uint8_t j,i;
    x1/=4;
    x2/=4;
    Column_Address(x1,x2-1);
    Row_Address(y1,y2-1);
    for(i=y1;i<y2;i++)
    {
        for(j=x1;j<x2;j++)
        {
           data(color);
           data(color);
        }
    }
}




void er_oled_char(uint16_t x, uint8_t y, const char  *acsii, uint8_t sizey, uint8_t mode)
{   uint8_t sizex,c,temp,t=2,DATA1=0,DATA2=0,m;
    uint16_t i,k,size2;
    x/=4;
    sizex=sizey/4/2; //除4是因为一个列地址线控制4列 除2是字宽:字高 1:2
    size2=(sizey/16+((sizey%16)?1:0))*sizey; //计算一个字符所占字节数
    c=*acsii-' ';
    Column_Address(x,x+sizex-1);//设置列地址
    Row_Address(y,y+sizey-1);//设置行地址
    for(i=0;i<size2;i++)
	{
		if(sizey==16)
		{      temp=(pgm_read_byte(&ascii_1608[c][i]));//8x16 ASCII码
			
		}
		else if(sizey==24)
		{       temp=(pgm_read_byte(&ascii_2412[c][i]));//12x24 ASCII码
			
		}
		else if(sizey==32)
		{       temp=(pgm_read_byte(&ascii_3216[c][i]));//16x32 ASCII码
			
		}
		if(sizey%16)
		{
			m=sizey/16+1;
			if(i%m) t=1;
			else t=2;
		}
		for(k=0;k<t;k++)
		{
			if(temp&(0x01<<(k*4)))
			{
				DATA1=0x0F;
			}
			if(temp&(0x01<<(k*4+1)))
			{
				DATA1|=0xF0;
			}
			if(temp&(0x01<<(k*4+2)))
			{
				DATA2=0x0F;
			}
			if(temp&(0x01<<(k*4+3)))
			{
				DATA2|=0xF0;
			}
			if(mode)
			{
				data(~DATA2);
				data(~DATA1);		
			}
			else
			{	
				data(DATA2);
				data(DATA1);
			}
			DATA1=0;
			DATA2=0;
		}
    }

}

void er_oled_string(uint16_t x, uint8_t y, const char *pString,uint8_t sizey,  uint8_t Mode)
{     
    while(*pString!='\0')
    {
        er_oled_char(x,y,pString,sizey,Mode);
        pString++;
        x+=sizey/2;
    }
}





void DrawBMP(uint16_t x,uint8_t y,uint16_t length,uint8_t width,const uint8_t * pBuf,uint8_t mode)
{
 
    uint16_t i,num;uint8_t DATA0=0;
 	length=(length/4+((length%4)?1:0))*4;
	num=length*width/2/2; //计算一个灰度图片所占字节数 一次写入2个字节 所以再除以2
	x/=4;
	length/=4;
	Column_Address(x,x+length-1);
	Row_Address(y,y+width-1);
	for(i=0;i<num;i++)
	{
		if(mode)
		{DATA0=(pgm_read_byte(pBuf+i*2+1));
		 data(~DATA0);
                 DATA0=(pgm_read_byte(pBuf+i*2));
		 data(~DATA0);
		}
		else
		{
		 DATA0=(pgm_read_byte(pBuf+i*2+1));
		 data(DATA0);
                 DATA0=(pgm_read_byte(pBuf+i*2));
		 data(DATA0);			
		}
	}



}





void DrawSingleBMP(uint16_t x,uint8_t y,uint16_t length,uint8_t width,const uint8_t * pBuf,uint8_t mode)
{
    uint8_t t=2,DATA0=0,DATA1=0,DATA2=0;
    uint16_t i,k,num=0;
    length=(length/8+((length%8)?1:0))*8;
    num=length/8*width;  //计算图片所占字节数
    x/=4;
    length/=4;
    Column_Address(x,x+length-1);
    Row_Address(y,y+width-1);
for(i=0;i<num;i++)
	{    DATA0=(pgm_read_byte(pBuf));
		for(k=0;k<t;k++)
		{
			if(DATA0&(0x80>>(k*4)))
			{
				DATA1=0x0F;
			}
			if(DATA0&(0x80>>(k*4+1)))
			{
				DATA1|=0xF0;
			}
			if(DATA0&(0x80>>(k*4+2)))
			{
				DATA2=0x0F;
			}
			if(DATA0&(0x80>>(k*4+3)))
			{
				DATA2|=0xF0;
			}
			if(mode)
			{
				data(~DATA2);
				data(~DATA1);		
			}
			else
			{	
                                data(DATA2);
				data(DATA1);				
			}
			DATA1=0;
			DATA2=0;
		}
               *pBuf++; 
	}

}



#include "oled.h"



void DELAY_MS(int ms) {
  ms = ms * F_CPU / 1000000;
  while (ms > 0) {
    _delay_us(500);
    ms -= 1;
  }
}




#define I2C_DELAY() // no delay



void oled_init(void){
  SET_MODE_OUTPUT(OLED_SDA); // Set port as output
  SET_MODE_OUTPUT(OLED_SCL); // Set port as output

  oled_send_command(0xAE); // display off
//   oled_send_command(0x00); // Set Memory Addressing Mode
//   oled_send_command(0x10); // 00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
//   oled_send_command(0x40); // Set Page Start Address for Page Addressing Mode,0-7
//   oled_send_command(0x81); // Set COM Output Scan Direction
//   oled_send_command(0xCF); // ---set low column address
//   oled_send_command(0xA1); // ---set high column address
//   oled_send_command(0xC8); // --set start line address
//   oled_send_command(0xA6); // --set contrast control register
//   oled_send_command(0xA8);
//   oled_send_command(0x3F); // --set segment re-map 0 to 127
//   oled_send_command(0xD3); // --set normal display
//   oled_send_command(0x00); // --set multiplex ratio(1 to 64)
//   oled_send_command(0xD5); //
//   oled_send_command(0x80); // 0xa4,Output follows RAM content;0xa5,Output ignores RAM content
//   oled_send_command(0xD9); // -set display offset
//   oled_send_command(0xF1); // -not offset
//   oled_send_command(0xDA); // --set display clock divide ratio/oscillator frequency
//   oled_send_command(0x12); // --set divide ratio
//   oled_send_command(0xDB); // --set pre-charge period
//   oled_send_command(0x40); //
//   oled_send_command(0x20); // --set com pins hardware configuration
//   oled_send_command(0x02);
//   oled_send_command(0x8D); // --set vcomh
//   oled_send_command(0x14); // 0x20,0.77xVcc
//   oled_send_command(0xA4); // --set DC-DC enable
//   oled_send_command(0xA6); //

// new init
  oled_send_command(0xd5);   // SET_DISP_FREQ_MODE
  oled_send_command(0x50);   // DEFAULT_DISP_FREQ = 0x50

  oled_send_command(0xd3);   // SET_DISPLY_LINE_OFFSET
  oled_send_command(0x00);   // SET_DISPLY_LINE_OFFSET = 0

  oled_send_command(0x40 | 0);   // SET_DISPLY_START_LINE = 0

  oled_send_command(0xAF); // --turn on oled panel
}

void oled_xfer_start(void){
  DIGITAL_WRITE_HIGH(OLED_SCL);  // Set to HIGH
  I2C_DELAY();
  DIGITAL_WRITE_HIGH(OLED_SDA);  // Set to HIGH
  I2C_DELAY();
  DIGITAL_WRITE_LOW(OLED_SDA);   // Set to LOW
  I2C_DELAY();
  DIGITAL_WRITE_LOW(OLED_SCL);   // Set to LOW
}

void oled_xfer_stop(void){
  DIGITAL_WRITE_LOW(OLED_SCL);   // Set to LOW
  I2C_DELAY();
  DIGITAL_WRITE_LOW(OLED_SDA);   // Set to LOW
  I2C_DELAY();
  DIGITAL_WRITE_HIGH(OLED_SCL);  // Set to HIGH
  I2C_DELAY();
  DIGITAL_WRITE_HIGH(OLED_SDA);  // Set to HIGH
}


void oled_send_command(uint8_t command){
  oled_xfer_start();
  oled_send_byte(OLED_SA);  // Slave address, SA0=0
  oled_send_byte(0x00);  // write command
  oled_send_byte(command);
  oled_xfer_stop();
}

void oled_send_data_start(void){
  oled_xfer_start();
  oled_send_byte(OLED_SA);
  oled_send_byte(0x40);  //write data
}

void oled_send_data_stop(void){
  oled_xfer_stop();
}

void oled_setpos(uint8_t x, uint8_t y)
{
  if (y>7) return;
  if (x>127) return;
  x = x + 2;
  oled_xfer_start();
  oled_send_byte(OLED_SA);  //Slave address,SA0=0
  oled_send_byte(0x00);  //write command

  oled_send_byte(((x>>4) & 0x0f)|0x10); // |0x10
  oled_send_byte((x & 0x0f)|0x00); // |0x00
  oled_send_byte(0xb0 | y);

  oled_xfer_stop();
}


#pragma GCC push_options
#pragma GCC optimize ("O2")

void oled_send_byte(uint8_t byte){
  uint8_t i;
  for(i=0; i<8; i++)
  {
    if((byte << i) & 0x80)
      DIGITAL_WRITE_HIGH(OLED_SDA);
    else
      DIGITAL_WRITE_LOW(OLED_SDA);

    DIGITAL_WRITE_HIGH(OLED_SCL);
    I2C_DELAY();
    DIGITAL_WRITE_LOW(OLED_SCL);
    I2C_DELAY();
  }
  // look for ACK
  DIGITAL_WRITE_HIGH(OLED_SDA);
  SET_MODE_INPUT(OLED_SDA);

  DIGITAL_WRITE_HIGH(OLED_SCL);
  I2C_DELAY();
  DIGITAL_WRITE_LOW(OLED_SCL);
  I2C_DELAY();
  SET_MODE_OUTPUT(OLED_SDA);
  DIGITAL_WRITE_LOW(OLED_SDA);
}


void oled_fillscreen(uint8_t fill_Data){
  uint8_t m,n;
  for(m=0;m<8;m++)
  {
    oled_setpos(0, m);
    oled_send_data_start();
    for(n=0;n<128;n++)
    {
      oled_send_byte(fill_Data);
    }
    oled_send_data_stop();
  }
}


void oled_char_f6x8(uint8_t x, uint8_t y, const char ch[]){
  uint8_t c,i,j=0;
  while(ch[j] != '\0')
  {
    c = ch[j] - 32;
    if (c >0) c = c - 12;
    if (c >15) c = c - 6;
    if (c>40) c = c - 9;
   if(x>127)
   {
     x=0;
     y++;
   }
    oled_setpos(x,y);
    oled_send_data_start();
    for(i=0; i<6; i++)
    {
      oled_send_byte(pgm_read_byte(&xoled_font6x8[(c*6)+i]));
    }
    x+=6;
    oled_send_data_stop();
    j++;
  }
}
#pragma GCC pop_options

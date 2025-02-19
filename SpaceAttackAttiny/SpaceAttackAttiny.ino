/* 2015 / 2016
 * SpaceAttack game by Andy Jackson - Twitter @andyhighnumber
 * Inspired by http://webboggles.com/ and includes some code from the #AttinyArcade games on that site
 * The code that does not fall under the licenses of sources listed below can be used non commercially with attribution.
 *
 * Press both left and right buttons to fire (works on #AttinyArcade which only has two buttons), or...
 * This code allows for three button inputs - the usual two on pins 5 and 7 (for left and right) and an additional switch on the reset pin
 * One side of the fire button should be wired to the reset pin (pin 1 - the only unused pin on the chip!) and the other side to  a voltage divider,
 * consisting of a 6.8k resistor to ground and a 1k resistor to vcc (battery +ve) - this allows the code to read a voltage on the input that
 * drops enough to be noticible when the button is pressed, but not enough to reset the chip!
 *
 * Also, from standby....
 *  Press and hold left button to turn sound on and off
 *  Press and hold left button with the right button held to reset high score
 *
 * If you have problems uploading this sketch, this is probably due to sketch size - you need to update ld.exe in arduino\hardware\tools\avr\avr\bin
 * https://github.com/TCWORLD/ATTinyCore/tree/master/PCREL%20Patch%20for%20GCC
 *
 * This sketch is using the screen control and font functions written by Neven Boyanov for the http://tinusaur.wordpress.com/ project
 * Source code and font files available at: https://bitbucket.org/tinusaur/ssd1306xled
 * **Note that this highly size-optimised version requires modified library functions (which are in this source code file)
 * and a modified font header
 *
 * Sleep code is based on this blog post by Matthew Little:
 * http://www.re-innovation.co.uk/web12/index.php/en/blog-75/306-sleep-modes-on-attiny85
*/
#include <EEPROM.h>
#include <avr/sleep.h>
#include <avr/interrupt.h> // needed for the additional interrupt



#include "oled.h"


// Routines to set and clear bits (used in the sleep code)
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))



#define RIGHT_BTN_PIN  4
#define LEFT_BTN_PIN  3

// Function prototypes
void resetAliens(void);
void drawPlatform(void);
void sendBlock(int);
void playSpaceAttack(void);
void beep(int,int);
void levelUp(int);
void drawFire(int x, int y);
void doDrawLS(long, byte);
void doDrawRS(long, byte);
void clearAlienArea(int row, int lastActive);
void doNumber (int x, int y, int value);


long firecounter = 0; // Timer for managing fire
long aliencounter = 0; // Timer for alien movement
int mothercounter = 0; // Timer for movement of the mothership
int level = 1; // Game level - incremented every time you clear the screen
int mothershipX = 0; // position of mothership
int mothership = 0; // is mothership active?
int mothershipWidth = 8; // mothership width in pixels
int fireXidx, fireYidx; // mapping of player fire locaiton onto array of aliens
int leftLimit; // furtherst left in x-axis pixels that the aliens are currently
int positionNow = 0; // current position of the alien array (as steps from the left)
boolean alienDirection = 1; // current direction of travel for alien swarm - 1 is right 0 is left
int alienRow = 0; // which alien row are we considering
int alienFire[5][3]; // max 5 lots of alien fire  - indices are active, xpos, ypos
int playerFire[3]; // one lot of player fire - indices are active, xpos, ypos
boolean row[4][10]; // on-off array of aliens
int firstAlien = 0; // index of first live alien in the array
int lastAlien = 8; // index of last live alien in the array
int newFirst = firstAlien; // as above when it changes
int newLast = lastAlien; // ...
int aliensDead = 0; // how many aliens have been killed in total on this level?
int lastActiveRow = 2; // what's the lowest row in which we will find live aliens?
int deadOn1 = 0; // how many aliens are dead on row one (middle row)
int deadOn2 = 0; // how many aliens are dead on row two (bottom row)

boolean fire = 0;
int topScoreB = 0;
int player; //0 to 128-platformWidth  - this is the position of the player
int platformWidth = 16;
boolean stopAnimate = 0; // this is set to 1 when a collision is detected
boolean mute = 0;
boolean newHigh = 0;
int score = 0; // score - this affects the difficulty of the game
int top = 0;

// // Interrupt handlers
// ISR(PCINT0_vect){
//   if ((digitalRead(LEFT_BTN_PIN)==0) | (digitalRead(RIGHT_BTN_PIN)==0)) fire = 1;
// }

// void playerIncSpaceAttack(){ // PB2 pin button interrupt
//   if (digitalRead(RIGHT_BTN_PIN)==0) fire = 1;
// }

// Arduino stuff - setup
void setup() {
  PORTB = (1<<RIGHT_BTN_PIN) | (1<<LEFT_BTN_PIN);
  DDRB = 0b00000010;    // set PB1 as output (for the speaker)
  // PCMSK = 0b00011000; // pin change mask: listen to portb bit 4
  // GIMSK |= 0b00100000;  // enable PCINT interrupt
  sei();          // enable all interrupts
}

// Arduino stuff - loop
void loop() {
  oled_init();
  oled_fillscreen(0x00);

  // The lower case character set is seriously compromised because I've had to truncate the ASCII table
  // to release space for executable code - hence lower case y and w are remapped to h and / respectively.
  // There is no z in the table (or h!) as these aren't used anywhere in the text here and most of the
  // symbols are also missing for the same reason (see my hacked version of font6x8.h - font6x8AJ.h for more detail)
  oled_char_f6x8(0, 1, "S P A C E");
  oled_char_f6x8(4, 2, "A T T A C K");
  oled_char_f6x8(0, 4, "andh jackson"); // see comments above !


  oled_setpos(85,1);
  oled_send_data_start();
  sendBlock(1);
  sendBlock(0);
  sendBlock(1);
  oled_send_data_stop();
  oled_setpos(85,2);
  oled_send_data_start();
  sendBlock(0);
  sendBlock(2);
  sendBlock(0);
  oled_send_data_stop();
  oled_setpos(85,3);
  oled_send_data_start();
  sendBlock(0);
  sendBlock(0);
  sendBlock(1);
  sendBlock(0);
  sendBlock(1);
  oled_send_data_stop();
  player = 96;
  drawPlatform();

  while(digitalRead(RIGHT_BTN_PIN) == HIGH);

  long startT = millis();
  long nowT =0;
  boolean sChange = 0;
  while(digitalRead(RIGHT_BTN_PIN) == LOW) {
    nowT = millis();
    if (nowT - startT > 2000) {
      sChange = 1;
      if (digitalRead(LEFT_BTN_PIN) == LOW) {
        EEPROM.write(0,0);
        EEPROM.write(1,0);
        oled_char_f6x8(8, 0, "-HIGH SCORE RESET-");
      } else if (mute == 0) { mute = 1; oled_char_f6x8(32, 0, "-- MUTE --"); } else { mute = 0; oled_char_f6x8(23, 0, "-- SOUND ON --");  }
      break;
    }
    if (sChange == 1) break;
  }
  while(digitalRead(RIGHT_BTN_PIN) == LOW);


  if (sChange == 0) {
    for (byte mm = 112;mm>=1;mm--) {
      if ( (mm>=32) && (mm<56) ) drawFire(104,mm);
      if (mm == 32) {
        oled_setpos(100,3);
        oled_send_data_start();
        sendBlock(0);
        oled_send_data_stop();
        oled_setpos(100,4);
        oled_send_data_start();
        sendBlock(0);
        oled_send_data_stop();
        beep(30,100);
      }
      oled_setpos(mm,0);
      oled_send_data_start();
      sendBlock(3);
      sendBlock(0);
      oled_send_data_stop();
      drawPlatform();
      DELAY_MS(5);
    }
    oled_setpos(0,0);
    oled_send_data_start();
    sendBlock(0);
    sendBlock(0);
    oled_send_data_stop();
    oled_char_f6x8(0, 6, "inspired bh");  // see comments above !
    oled_char_f6x8(0, 7, "/ebboggles.com");  // see comments above !
    DELAY_MS(1500);
    // delay(1500);
    oled_init();
    oled_fillscreen(0x00);
    stopAnimate = 0;
    score = 0;

    playSpaceAttack();
    top=topScoreB;

    oled_fillscreen(0x00);
    oled_char_f6x8(11, 1, "----------------");
    oled_char_f6x8(11, 2, "G A M E  O V E R");
    oled_char_f6x8(11, 3, "----------------");
    oled_char_f6x8(37, 5, "SCORE:");
    doNumber(75, 5, score);
    if (!newHigh) {
      oled_char_f6x8(21, 7, "HIGH SCORE:");
      doNumber(88, 7, top);
    }
    for (int i = 0; i<1000; i = i+ 50){
      beep(50,i);
    }
    DELAY_MS(1500);
    DELAY_MS(1500);
    // delay(2000);
    if (newHigh) {
      oled_fillscreen(0x00);
      oled_char_f6x8(10, 1, "----------------");
      oled_char_f6x8(10, 3, " NEW HIGH SCORE ");
      oled_char_f6x8(10, 7, "----------------");
      doNumber(50,5,top);
      for (int i = 700; i>200; i = i - 50){
      beep(30,i);
      }
      newHigh = 0;
      DELAY_MS(1500);
    }
  }
  system_sleep();
}

void doNumber (int x, int y, int value) {
    char temp[10] = {0,0,0,0,0,0,0,0,0,0};
    itoa(value,temp,10);
    oled_char_f6x8(x, y, temp);
}


void system_sleep() {
  // oled_fillscreen(0x00);
  oled_send_command(0xAE);
  cbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();
  sleep_mode();                        // System actually sleeps here
  sleep_disable();                     // System continues execution here when watchdog timed out
  sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
  oled_send_command(0xAF);
}

void beep(int bCount,int bDelay){
  if (mute) return;
  for (int i = 0; i<=bCount; i++){digitalWrite(1,HIGH);for(int i2=0; i2<bDelay; i2++){__asm__("nop\n\t");}digitalWrite(1,LOW);for(int i2=0; i2<bDelay; i2++){__asm__("nop\n\t");}}
}



/* ------------------------
 *  SpaceAttack Code
 */
void playSpaceAttack() {

  firecounter = 0; // Timer for managing fire
  level = 1; // Game level - incremented every time you clear the screen
  topScoreB = 0; // highscore
  score = 0; // obvious

  // Initialisations
  for (byte i = 0; i<5;i++) {
    alienFire[i][0] = 0;
  }
  resetAliens();

  levelUp(1); // This also does various essential initialisations

  // attachInterrupt(0,playerIncSpaceAttack,CHANGE);

  while (stopAnimate == 0) {
    while(1) {
    aliencounter++;
    firecounter++;
    mothercounter++;

    // deal with inputs

    // ADC button        | Resistance (R2) | Voltage | ADC threshold (8 MSB precision) | | V1 Mapping
    // ------------------|-----------------|---------|---------------------------------|-|-------------
    // ADC_VD_CH0_BTN_0  | 51 K            | 1.222 v | 0x70    | 0b01110000            | NAV_UP_BTN
    // ADC_VD_CH0_BTN_1  | 68 K            | 1.395 v | 0x80    | 0b10000000            | NAV_DOWN_BTN
    // ADC_VD_CH0_BTN_2  | 100 K           | 1.613 v | 0x94    | 0b10010100            | NAV_LEFT_BTN
    // ADC_VD_CH0_BTN_3  | 300 K           | 2.074 v | 0xbf    | 0b10111111            | NAV_RIGHT_BTN
    // ADC_VD_CH0_BTN_4  | 1 M             | 2.305 v | 0xd4    | 0b11010100            | OK

    int adcBtn = analogRead(0);
    if(adcBtn < (0x70 + 8 << 2)) {
      fire = 1;
    } else if(adcBtn < (0x80 + 8 << 2)) {
      // noop
    } else if(adcBtn < (0x94 + 8 << 2)) {
      if(player > 1) {
        player--;
      }
    } else if (adcBtn < (0xbf + 8 << 2)) {
      if (player < 127-platformWidth){
        player++;
      }
    }

    // if(analogRead(3) < (0x1f + 8 << 2)) {
    //   fire = 1;
    // }

    if ((digitalRead(RIGHT_BTN_PIN)==LOW) | digitalRead(LEFT_BTN_PIN)==LOW) fire = 1;

    if ( (mothership == 0) && (random(0,1000) > 998) && (alienRow>0) ) {
      mothership = 1;
      mothershipX= 127-16;
    }

    // draw aliens
    for (int inc = 0; inc <= lastActiveRow; inc ++) {
      oled_setpos((positionNow*8),inc+alienRow);
      oled_send_data_start();
      for (int bl = firstAlien; bl <=lastAlien; bl++){
        if(row[inc][bl]==1){
          if (inc == lastActiveRow) {
            if(random(0,1000) > (999-level)) { // this alien is going to fire!
              byte afIndex = 0;
              while (alienFire[afIndex][0] == 1) {
                afIndex++; // search for empty alien fire option
                if (afIndex == 5) break;
              }
              if (afIndex < 5) { // we've found a slot
                alienFire[afIndex][0] = 1; // activate fire on this slot
                alienFire[afIndex][1] = ( (positionNow*8) + ((bl-firstAlien) * 8) + 4); // x position
                alienFire[afIndex][2] = (inc+alienRow+1)*8; // Where the fire starts
              }
            } // end of this alien firing
          } // only if we are on the lowest row of live aliens
          if ( (inc == 0) || (inc == 2) ) {
            sendBlock(1);
          } else {
            sendBlock(2);
          }
        }else {
          sendBlock(0);
        }
       }
      oled_send_data_stop();
    }


    // Display the score
    doNumber(0,6,score);

    // Burn clock cycles to keep game at constant (ish) speed when there are low numbers of live aliens
    int burnLimit = (8-(lastAlien-firstAlien));
    for (int burn = 0; burn < burnLimit; burn+=2) {
        drawPlatform();
        }

    // Display the mothership
    if (mothercounter >= 3) {
      mothercounter = 0;
      // draw mothership
      if (mothership) {
        oled_setpos(mothershipX,0);
        oled_send_data_start();
        sendBlock(3);
        sendBlock(0);
        oled_send_data_stop();
        mothershipX --;
        if (mothershipX == 0) {
          mothership = 0;
          oled_setpos(mothershipX,0);
          oled_send_data_start();
          sendBlock(0);
          sendBlock(0);
          oled_send_data_stop();
        }
      }
    }

    // Move the aliens
    if (aliencounter >= (92-((level-1)*5)) ) {
        aliencounter = 0;
      if(alienDirection) { // Moving right
        // move down a row
        if (positionNow >= 6+(8-(lastAlien-firstAlien))) {
          alienDirection = 0;
          clearAlienArea(alienRow,lastActiveRow);
          alienRow++;
        } else {
          positionNow++;
        }
      } else { // Moving left
        // move down a row
        if (positionNow <= 0) {
          alienDirection = 1;
          clearAlienArea(alienRow,lastActiveRow);
          alienRow++;
        } else {
          positionNow --;
        }
      }
      clearAlienArea(alienRow,lastActiveRow);
    }

    // Fire !
    if ((fire == 1) && (playerFire[0] == 0)) { // fire has been pressed and we're not currently firing - initiate fire!!
      playerFire[0] = 1;
      playerFire[1] = player+platformWidth/2; // xposition of new fire!
      playerFire[2] = 56;
    }

    // Handle all firing-related stuff (in both directions!)
    if (firecounter >= 2) {
      firecounter = 0;
      // --- Deal with player Firing ---
      if (playerFire[0] == 1) {
          drawFire(playerFire[1], playerFire[2]);
          if (playerFire[2] == 0) {
            oled_setpos(playerFire[1],0);
            uint8_t temp = B00000000;
            oled_send_data_start();
            oled_send_byte(temp);
            oled_send_data_stop();
            playerFire[0] = 0;
            fire = 0;
          } else {
            playerFire[2] = playerFire[2] - 1;
          }
      }

      // aliens are at positionNow * 8 + 8* their index

      leftLimit = positionNow*8;
      if ((fire == 1)) {
        fireXidx = firstAlien + floor((playerFire[1]-positionNow*8)/8);
        fireYidx=  floor(floor(playerFire[2]/8) - alienRow);

        if((mothership == 1) && (playerFire[1] >= mothershipX) && (playerFire[1] <= mothershipX +8) && playerFire[2] <=8) {
          long scm = random(1,100);
          if (scm<30) {
            score +=50;
          } else if (scm<60) {
            score += 100;
          } else if (scm<90) {
            score += 150;
          } else {
            score += 300;
          }

          beep(30,400);
          beep(30,300);
          beep(30,200);
          beep(30,100);
          mothership = 0;
          oled_setpos(mothershipX,0);
          oled_send_data_start();
          sendBlock(0);
          sendBlock(0);
          oled_send_data_stop();
        }

        // Alien has been hit
        if ((playerFire[1] >= leftLimit) && (fireYidx>=0) && (fireYidx<=lastActiveRow) && (fireXidx>=0) && (fireXidx<9)) {
          if (row[fireYidx][fireXidx] == 1) {

            int lastActiveToClear = lastActiveRow; // if we kill the last alien on a row - we still need to clear that row (end of this fn)

            if (fireYidx == 2) deadOn2++;
            if (fireYidx == 1) deadOn1++;

            if (deadOn2 == 5) {lastActiveRow = 1;}
            if ((deadOn1 == 4) && (deadOn2 == 5)) {lastActiveRow = 0;}

            score = score + (int)((3-fireYidx) * 10);

            aliensDead++;
            oled_setpos(playerFire[1],alienRow+fireYidx+1);
            uint8_t temp = B00000000;
            oled_send_data_start();
            oled_send_byte(temp);
            oled_send_data_stop();
            beep(30,100);

            fire = 0;
            playerFire[0] = 0;
            playerFire[1] = 0;
            playerFire[2] = 7;
            row[fireYidx][fireXidx] = 0;

            if (fireXidx == firstAlien) { // this is the first Alien - sweep back and reset to new first alien
              for (int xi = lastAlien; xi>=firstAlien;xi--) {
                if ( (row[0][xi] == 1) || (row[1][xi] == 1) || (row[2][xi] == 1) ) newFirst = xi;
              }
              positionNow += newFirst - firstAlien;
              firstAlien = newFirst;
            }

            if (fireXidx == lastAlien) { // this is the last Alien - sweep thru and reset to new last alien
              for (int xi = firstAlien; xi<=lastAlien;xi++) {
                if ( (row[0][xi] == 1) || (row[1][xi] == 1) || (row[2][xi] == 1) ) newLast = xi;
              }
              lastAlien = newLast;
            }
            clearAlienArea(alienRow,lastActiveToClear);

          }
        }
      }


      // --- Deal with Alien Firing ---
      for (byte afIndex = 0; afIndex<5; afIndex++) {
        if(alienFire[afIndex][0] == 1) {
          drawFire(alienFire[afIndex][1],alienFire[afIndex][2]);
          alienFire[afIndex][2] = alienFire[afIndex][2] + 1;

          if (alienFire[afIndex][2] >= 56) {
            oled_setpos(alienFire[afIndex][1],7);
            uint8_t temp = B00000000;
            oled_send_data_start();
            oled_send_byte(temp);
            oled_send_data_stop();
            alienFire[afIndex][0] = 0; // the fire's got to the end
            if( ((alienFire[afIndex][1]) > player) && ( (alienFire[afIndex][1]) < player+platformWidth) ) { // you've been hit!!
              stopAnimate = 1;
              goto die;
              }
          }
        }
      } // end of aliens firing
    }

    if (aliensDead == 14) {
      for (int ai = 0; ai <= 5; ai++) alienFire[ai][0] = 0;

      level++;
      if (level>15) level = 15;
      levelUp(level);

      resetAliens();
    }

    if( ((alienRow == 5) && (lastActiveRow == 2))||  ( (alienRow == 6) && (lastActiveRow == 1)) || ((alienRow == 7) && (lastActiveRow == 0)) || stopAnimate) {
      stopAnimate = 1;
      break;
    }
    // draw the player
    drawPlatform();
    }
 }
die:
  topScoreB = EEPROM.read(0);
  topScoreB = topScoreB << 8;
  topScoreB = topScoreB |  EEPROM.read(1);

  if (score > topScoreB) {
    topScoreB = score;
    EEPROM.write(1,score & 0xFF);
    EEPROM.write(0,(score>>8) & 0xFF);
    newHigh = 1;
    }
}

void drawPlatform(){
 oled_setpos(player,7);
 oled_send_data_start();
 oled_send_byte(B00000000);
 oled_send_byte(B00000000);
 for (byte pw = 0; pw <5; pw++){oled_send_byte(B11000000);}
 for (byte pw = 0; pw <4; pw++){oled_send_byte(B11110000);}
 for (byte pw = 0; pw <5; pw++){oled_send_byte(B11000000);}
 oled_send_byte(B00000000);
 oled_send_byte(B00000000);
 oled_send_data_stop();
}

void sendBlock(int fill){
  if (fill == 1) {
   oled_send_byte(B10011000);
   oled_send_byte(B01011100);
   oled_send_byte(B10110110);
   oled_send_byte(B01011111);
   oled_send_byte(B01011111);
   oled_send_byte(B10110110);
   oled_send_byte(B01011100);
   oled_send_byte(B10011000);
  } else if (fill == 2) {
   oled_send_byte(B00110000);
   oled_send_byte(B00111110);
   oled_send_byte(B10110011);
   oled_send_byte(B01011101);
   oled_send_byte(B01011101);
   oled_send_byte(B10110011);
   oled_send_byte(B00111110);
   oled_send_byte(B00110000);
  } else if (fill == 3) {
   oled_send_byte(B00011000);
   oled_send_byte(B00111000);
   oled_send_byte(B00110100);
   oled_send_byte(B00110100);
   oled_send_byte(B00110100);
   oled_send_byte(B00110100);
   oled_send_byte(B00111000);
   oled_send_byte(B00011000);
  } else if (fill == 0) {
   oled_send_byte(B00000000);
   oled_send_byte(B00000000);
   oled_send_byte(B00000000);
   oled_send_byte(B00000000);
   oled_send_byte(B00000000);
   oled_send_byte(B00000000);
   oled_send_byte(B00000000);
   oled_send_byte(B00000000);
  }
}

void levelUp(int number) {

  fire = 0; // make sure no fire
  playerFire[0] = 0; // player fire inactive
  aliencounter=0;
  firecounter=0;
  mothercounter=0;
  firstAlien = 0;
  lastAlien = 8;
  newFirst = 0;
  newLast = 8;
  lastActiveRow = 2;
  deadOn1 = 0;
  deadOn2 = 0;
  aliensDead = 0;
  mothership = 0;
  alienRow = 0;
  positionNow = 0;
  alienDirection = 1;
  player = 64;

  oled_fillscreen(0x00);
  oled_char_f6x8(16, 3, "--------------");
  oled_char_f6x8(16, 4, " L E V E L ");
  oled_char_f6x8(16, 5, "--------------");
  doNumber(85,4,number);
  for (int i = 800; i>200; i = i - 200){
  beep(30,i);
  }
  DELAY_MS(800);
  oled_fillscreen(0x00);
}

void drawFire(int x, int y) {
  if (y%8!=0){
    oled_setpos(x,y/8);
    oled_send_data_start();
    doDrawLS(0,y%8);
    oled_send_data_stop();

    oled_setpos(x,y/8+1);
    oled_send_data_start();
    doDrawRS(0,8-y%8);
    oled_send_data_stop();
  } else {
    oled_setpos(x,y/8);
    oled_send_data_start();
    doDrawLS(0,0);
    oled_send_data_stop();
  }
}

// Drawing routine for the player and fire - with right-shifts
void doDrawRS(long P1, byte P2) {
  oled_send_byte((B01111110 | P1)>>P2);
}

// Drawing routine for the player and fire - with left-shifts
void doDrawLS(long P1, byte P2) {
  oled_send_byte((B01111110 | P1)<<P2);
}

void clearAlienArea(int row, int lastActive) {
  // clear alien area
  for (int inc = row; inc <=row+lastActive; inc ++) {
    oled_setpos(0,inc);
    oled_send_data_start();
    for (int bl = 0; bl <16; bl++){
      sendBlock(0);
      }
      oled_send_data_stop();
    }
}

void resetAliens(void) {
  for (byte i =0; i<10;i++){ // reset aliens
    row[0][i]=0; row[1][i]=0; row[2][i]=0;
  }
  for (byte i =0; i<9;i+=2){ // reset aliens
    row[0][i]=1; row[2][i]=1;
  }
  for (byte i =1; i<9;i+=2){ // reset aliens
    row[1][i]=1;
  }
}

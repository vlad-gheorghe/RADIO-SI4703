/// \file LCDKeypadRadio.ino 
/// \brief Radio implementation using the Serial communication.
/// https://github.com/mathertel/Radio
/// \author Matthias Hertel, http://www.mathertel.de
/// \copyright Copyright (c) 2014 by Matthias Hertel.\n
/// This work is licensed under a BSD style license.\n
/// See http://www.mathertel.de/License.aspx
///
/// \details
/// This is a full function radio implementation that uses a LCD display to show the current station information.\n
/// It can be used with various chips after adjusting the radio object definition.\n
/// Open the Serial console with 57600 baud to see current radio information and change various settings.
///
/// Wiring
/// ------
/// The necessary wiring of the various chips are described in the Testxxx example sketches.
/// The boards have to be connected by using the following connections:
/// 
/// Arduino port | SI4703 signal | 
/// :----------: | :-----------: | 
///          GND | GND           | 
///         3.3V | VCC           | 
///           5V | -             | 
///           A5 | SCLK          | 
///           A4 | SDIO          | 
///           D2 | RST           |
/// foR TFT
///MOSI = pin 11 and
// SCLK = pin 13.
/// More documentation and source code is available at http://www.mathertel.de/Arduino
///
/// History:
/// --------
/// * 05.08.2014 created.
/// * 04.10.2014 working.
/// * 22.03.2015 Copying to LCDKeypadRadio.


#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>
#include <Wire.h>

#include <radio.h>

#include <SI4703.h>

#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerif24pt7b.h>
#include <RDSParser.h>

 #define TFT_CS        10
  #define TFT_RST        9 // Or set to -1 and connect to Arduino RESET pin
  #define TFT_DC         8


// Define some stations available at your locations here:
// 89.40 MHz as 8940

RADIO_FREQ preset[] = {
  8770,
  8810, // hr1
  8820,
  8850, // Bayern2
  8890, // ???
  10650, // * hr3
  8980,
  9180,
  9220, 9350,
  9440, // * hr1
  9510, // - Antenne Frankfurt
  9530,
  9560, // Bayern 1
  9680, 9880,
  10020, // planet
  10090, // ffh
  10110, // SWR3
  10030, 10260, 10380, 10400,
  10500 // * FFH
};

int    i_sidx=5;        ///< Start at Station with index=5

/// The radio object has to be defined by using the class corresponding to the used chip.
/// by uncommenting the right radio object definition.

// RADIO radio;       ///< Create an instance of a non functional radio.
// RDA5807M radio;    ///< Create an instance of a RDA5807 chip radio
SI4703   radio;    ///< Create an instance of a SI4703 chip radio.
//SI4705   radio;    ///< Create an instance of a SI4705 chip radio.
// TEA5767  radio;    ///< Create an instance of a TEA5767 chip radio.


int menu = 7;
int fu = 3;
int fd = 4;
int vu = 5;
int vd = 6;
int pv=0;
/// get a RDS parser
RDSParser rds;


/// State definition for this radio implementation.
enum RADIO_STATE {
  STATE_PARSECOMMAND, ///< waiting for a new command character.
  
  STATE_PARSEINT,     ///< waiting for digits for the parameter.
  STATE_EXEC          ///< executing the command.
};

RADIO_STATE state; ///< The state variable is used for parsing input characters.

// - - - - - - - - - - - - - - - - - - - - - - - - - -



 Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

/// This function will be when a new frequency is received.
/// Update the Frequency on the LCD display.
void DisplayFrequency(RADIO_FREQ f)
{
  char s[12];
  radio.formatFrequency(s, sizeof(s));
 float a=radio.getFrequency(); // afisez numeric frecventa
 tft.setTextSize(1);
 int pa=0;
 if(a!=pa){
  pa=a;
   tft.fillRect(0,14,105,35,ST77XX_BLACK); 
  tft.setFont(&FreeSerif24pt7b);
  tft.setCursor(0, 45);tft.setTextColor(ST77XX_CYAN,ST77XX_BLACK); tft.print(a/100,1);
  tft.setFont();
 }

  
} // DisplayFrequency()


/// This function will be called by the RDS module when a new ServiceName is available.
/// Update the LCD to display the ServiceName in row 1 chars 0 to 7.
void DisplayServiceName(char *name)
{
  size_t len = strlen(name);
  
  tft.setCursor(0, 95); tft.setTextColor(ST77XX_GREEN,ST77XX_BLACK);tft.setTextSize(2);
  tft.print(name);
  while (len < 8) {
    tft.print(' ');
    len++;  
  } // while
} // DisplayServiceName()


/// This function will be called by the RDS module when a rds time message was received.
/// Update the LCD to display the time in right upper corner.
/*void DisplayTime(uint8_t hour, uint8_t minute) {
 // lcd.setCursor(11, 0); 
  if (hour < 10) lcd.print('0');
  lcd.print(hour);
  lcd.print(':');
  if (minute < 10) lcd.print('0');
  lcd.println(minute);
} // DisplayTime()
*/
void volum(){
   int v = radio.getVolume();
  
   tft.setTextColor(ST77XX_YELLOW);tft.setTextSize(1);
  
   if(v!=pv){
    pv=v;
    tft.fillRect(65,58,22,20,ST77XX_BLACK); 
    tft.setCursor(65, 65); tft.setFont(&FreeSerif9pt7b);
    tft.setTextColor(ST77XX_YELLOW,ST77XX_BLACK);
    tft.print(v);
    tft.setFont();
    tft.fillRect(5,86,110,5,ST77XX_BLACK);
   
     tft.fillRect(5,87,7*v,3,ST77XX_YELLOW);
   
   }//}

}


// - - - - - - - - - - - - - - - - - - - - - - - - - -


void RDS_process(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4) {
  rds.processData(block1, block2, block3, block4);
}


/// This function determines the current pressed key but
/// * doesn't return short key down situations that are key bouncings.
/// * returns a specific key down only once.
/*KEYSTATE getLCDKeypadKey() {
  static unsigned long lastChange = 0;
  static KEYSTATE lastKey = KEYSTATE_NONE;

  unsigned long now = millis();
  KEYSTATE newKey;

  // Get current key state
 // int v = analogRead(A0); // the buttons are read from the analog0 pin
  
  if (digitalRead(vu)==LOW) {
    newKey = KEYSTATE_RIGHT;
  } if (digitalRead(fu)==LOW) {
    newKey = KEYSTATE_UP;
  } if (digitalRead(fd)==LOW) {
    newKey = KEYSTATE_DOWN;
  }  if (digitalRead(vd)==LOW) {
    newKey = KEYSTATE_LEFT;
  }  if (digitalRead(menu)==LOW) {
    newKey = KEYSTATE_SELECT;
  } else {
    newKey = KEYSTATE_NONE;
  }

  if (newKey != lastKey) {
    // a new situation - just remember but do not return anything pressed.
    lastChange = now;
    lastKey = newKey;
    return (KEYSTATE_NONE);

  } else if (lastChange == 0) {
    // nothing new.
    return (KEYSTATE_NONE);

  } if (now > lastChange + 50) {
    // now its not a bouncing button any more.
    lastChange = 0; // don't report twice
    return (newKey);

  } else {
    return (KEYSTATE_NONE);

  } // if
} // getLCDKeypadKey()
*/
/// Setup a FM only radio configuration with I/O for commands and debugging on the Serial port.
void setup() {
  // open the Serial port
  Serial.begin(57600);
  Serial.println("LCDKeypadRadio...");
pinMode( menu, INPUT);
pinMode( fu, INPUT);
pinMode( fd, INPUT);
pinMode( vu, INPUT);
pinMode( vd, INPUT);
digitalWrite (menu, HIGH);
digitalWrite (fu, HIGH);
digitalWrite (fd, HIGH);
digitalWrite (vu, HIGH);
digitalWrite (vd, HIGH);
  
 tft.initR(INITR_144GREENTAB); tft.setRotation(3);  
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 50);tft.setTextSize(1);
  tft.print("TFT-KeypadRadio");
  delay(1000);
 tft.fillScreen(ST77XX_BLACK);
tft.setCursor(10, 1);tft.setTextSize(1);tft.setTextColor(ST77XX_YELLOW);
tft.print("FM-Radio SI4703");
tft.setCursor(110, 38); tft.setTextColor(ST77XX_CYAN);tft.setTextSize(1);
tft.print("MHz");
tft.setCursor(0, 65); tft.setTextColor(ST77XX_YELLOW);tft.setTextSize(1);tft.setFont(&FreeSerif9pt7b);
tft.print("Volum");
tft.setCursor(90, 70);tft.print("/15");
tft.setFont();
  tft.drawRect(2,84,115,9,ST77XX_YELLOW);
  // Initialize the Radio 
  radio.init();

  // Enable information to the Serial port
  radio.debugEnable();

  radio.setBandFrequency(RADIO_BAND_FM, preset[i_sidx]); // 5. preset.

  // delay(100);

  radio.setMono(false);
  radio.setMute(false);
  // radio.debugRegisters();
  radio.setVolume(8);

  Serial.write('>');
  
  state = STATE_PARSECOMMAND;
  
  // setup the information chain for RDS data.
  radio.attachReceiveRDS(RDS_process);
  rds.attachServicenNameCallback(DisplayServiceName);
  //rds.attachTimeCallback(DisplayTime);
} // Setup


/// Constantly check for serial input commands and trigger command execution.
void loop() {
  int newPos;
  unsigned long now = millis();
  static unsigned long nextFreqTime = 0;
  static unsigned long nextRadioInfoTime = 0;
  
  // some internal static values for parsing the input
  static char command;
  static int16_t value;
  static RADIO_FREQ lastf = 0;
  RADIO_FREQ f = 0;
  volum();
  char c;

//  KEYSTATE k = getLCDKeypadKey();

  if (digitalRead(fu)==LOW ){
    delay(100);
    radio.seekUp(true);

  } else if (digitalRead(vu)==LOW ) {
    // increase volume
    delay(100);
    Serial.println("increase volume");
    int v = radio.getVolume();
    if (v < 15) radio.setVolume(++v);

  } else if (digitalRead(vd)==LOW ) {
    // decrease volume
    delay(100);
    int v = radio.getVolume();
    if (v > 0) radio.setVolume(--v);

  } else if (digitalRead(fd)==LOW ) {
   delay(100);
    radio.seekDown(true);

  } else if (digitalRead(menu)==LOW ) {
    // 10110
    radio.setFrequency(10110);

  } else {
    // 
  }

  // check for RDS data
  radio.checkRDS();

  // update the display from time to time
  if (now > nextFreqTime) {
    f = radio.getFrequency();
    if (f != lastf) {
      // print current tuned frequency
      DisplayFrequency(f);
      lastf = f;
    } // if
    nextFreqTime = now + 400;
  } // if  

} // loop

// End.


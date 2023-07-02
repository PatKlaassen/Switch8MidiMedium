// ESP32 Switch8 medium version with 160x128 tft and I2C I/O expander
/*
Select Board: Tools > Board > ESP32 Arduino > ESP32 Dev Module

The first version will only use midi program change calls to select presets on one attached midi device
A future version will be filly programmable where eacht preset can trigger multiple midi program and or control changes
A future version will also provide a portal for editing preset actions using wifi

In this version the wifi button only changes the color scheme on the tft full color display


Routines
void initLEDs()
void setColorScheme()
void printHeaderFooter()
void printData(String strPreset, String strBank)
void printTextTFT(String strLine1, String strLine2, String strLine3, String strLine4)
void print2TextTFT(String strLine1, String strLine2, int intColor=TFT_BLACK)
void printIntroTFT(String strLine1)
void setup() 
void programChange(int intPreset)
void handleBankUp()
void handleBankDown()
void setMute()
void setProgMode()
void handleLock()
void keypadEvent(KeypadEvent key)
void loop()

*/

#include "Final_Frontier_28.h"
#include "NotoSansBold36.h"
#include "NotoSansBold15.h"


#include <MIDI.h>
#include <Wire.h>
#include <TFT_eSPI.h> 
#include <SPI.h>
#include <Keypad.h>

// Set I2C IO addresses
#define IO_ADDR_LEDs 0x24

uint32_t delayMS;

TFT_eSPI tft = TFT_eSPI();  // Invoke library

#define ROW_NUM     2 // 2 rows
#define COLUMN_NUM  6 // 6 columns

char keys[ROW_NUM][COLUMN_NUM] = {
  {'a', 'b', 'c','d', 'i', 'k'},
  {'e', 'f', 'g','h', 'j', 'l'}
};
// ID  button name         Pin usage
// a - preset/channel 1    12-14
// b - preset/channel 2    12-27
// c - preset/channel 3    12-26
// d - preset/channel 4    12-25
// e - preset/channel 5    13-14
// f - preset/channel 6    13-27
// g - preset/channel 7    13-26
// h - preset/channel 8    13-25
// i - bank switching down 12-33
// j - bank switching up   13-33
// k - Wifi toggle         12-32
// l - Bank lock / Prog    13-32

byte pin_rows[ROW_NUM] = {12, 13}; // GIOP2, GIOP15 connect to the row pins
byte pin_column[COLUMN_NUM] = {14, 27, 26, 25, 33, 32};  // GIOP14, GIOP27, GIOP26, GIOP25, GIOP33, GIOP32 connect to the column pins

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

boolean debug = true;
int intCurrentBank = 0;
int intCurrentPreset = -1;
int intPresetLEDs=0;
int intPresetBlinkingLEDs=0;
int intMidiChannel = 1;
int intMuteMode=0; // Var for storing mute mode, 0 for not active, 1 for active and 2 for tilt ;-) 
int intLockMode=0; // var for storing bank button lock, 0 for not active 1 for active. In lock mode the bank buttons are used for presets!
int arrLockMode[] = {0,1,2,3,5,6,7,8,4,9}; // When in bank lock mode the bank buttons become preset buttons 
int intProgMode=0;
int intBankPressed = 0;
unsigned long lngBurnInTimeOut = 0; // if this reaches 10 minutes the screen protector will be enabled  -- TODO NOT IMPLEMENTED YET
int intSSX=0; // Used for screensaver startpoint X
int intSSY=0; // Used for screensaver startpoint y
// const int intDisplayPower = 15; // connect 3v3 display pin to GPIO15. Power down when no activity for 10 min
// Pin 15 is used for midi Rx, this code is now obsolete and the leda display pin is connected tpt 3.3v
int intState=0; // 0=Waiting for input, 1=Active preset, 2=Mute, 3=After bank button
int intColorPreset = 0; // 0=black/white 1=black/blue, 2=black/red 3=blue/white, 4=red/white

const long interval = 250; // interval at which the leds will blink
unsigned long previousMillis = 0; // timer is used for blinking led's when saving preset

struct Serial1MIDISettings : public midi::DefaultSettings
{
  static const long BaudRate = 31250;
  static const int8_t TxPin  = 17;
};

MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial2, DIN_MIDI, Serial1MIDISettings);

void initLEDs()
{
  int i=0;
  // initLEDs tests all LED's so the user can see if all is working (and it looks nice)
  static unsigned char data = 0x01;  // data to display on LEDs
  static unsigned char direc = 1;    // direction of knight rider display
  int x = 0;
  for (x=0; x<2;x++)
  {
    for(i=0; i<16; i++)
    {
      // send the data to the LEDs
      Wire.beginTransmission(IO_ADDR_LEDs);
      Wire.write(~data);
      Wire.endTransmission();
      delay(20);  // speed of display
      // shift the on LED in the specified direction
      if (direc) {
        data <<= 1;
      }
      else {
        data >>= 1;
      }
      // see if a direction change is needed
      if (data == 0x80) {
        direc = 0;
      }
      if (data == 0x01) {
        direc = 1;
      }
    }
  }
  // LEDs of
  Wire.beginTransmission(IO_ADDR_LEDs);
  Wire.write(~0);
  Wire.endTransmission();
}


void setColorScheme(){
  // int intColorPreset = 0; // 0=black/white 1=black/blue, 2=black/red 3=blue/white, 4=red/white
  switch (intColorPreset) {
    case 0:
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      break;
    case 1:
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_BLUE, TFT_BLACK);
      break;
    case 2:
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_RED, TFT_BLACK);
      break;
    case 3:
      tft.fillScreen(TFT_BLUE);
      tft.setTextColor(TFT_WHITE, TFT_BLUE);
      break;
    case 4:
      tft.fillScreen(TFT_RED);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      break;
  }  
}


void printHeaderFooter(){
  // Prints the header and footer containiing the button labels
  if (intLockMode==0){
    tft.setCursor(2, 0);
    tft.loadFont(NotoSansBold15); // Final_Frontier_28
    tft.print("5      6      7      8       B+");
    tft.loadFont(NotoSansBold15);
    tft.setCursor(2, 113);
    tft.print("1      2      3      4       B-");
  } else { // In lock mode the bank buttons are preset buttons
    tft.setCursor(2, 0);
    tft.loadFont(NotoSansBold15); // Final_Frontier_28
    tft.print("6      7      8      9       10");
    tft.loadFont(NotoSansBold15);
    tft.setCursor(2, 113);
    tft.print("1      2      3      4       5");

  }
}


void printData(String strPreset, String strBank){
// Print single screen with top and bottom layout
  if (debug) Serial.println("Print Preset " + strPreset);
  if (debug) Serial.println("Print Bank " + strBank);
  setColorScheme();
  
  printHeaderFooter();
  tft.loadFont(NotoSansBold36);
  tft.setTextSize(2);
  
  if (strPreset=="10"){
    tft.setCursor(20,35);
  } else {
    tft.setCursor(30,35);
  }
  tft.print("P" + strPreset + "-B" + strBank);
  tft.setTextSize(1);
  tft.loadFont(NotoSansBold15); // Final_Frontier_28
  if (intLockMode==1){
    tft.setCursor(20, 72);
    tft.print("Bank lock Active");
  }
  tft.setCursor(50, 92);
  tft.print("Switch8");
}

void printTextTFT(String strLine1, String strLine2, String strLine3, String strLine4){
// Print screen with 4 lines of text
  setColorScheme();
  printHeaderFooter();
  tft.loadFont(Final_Frontier_28);
  tft.setTextSize(1);
  tft.setCursor(5, 16);
  tft.println(strLine1);
  tft.setCursor(5, 39);  
  tft.println(strLine2);
  tft.setCursor(5, 64);
  tft.println(strLine3);
  tft.setCursor(5, 87);
  tft.println(strLine4);
}


void print2TextTFT(String strLine1, String strLine2, int intColor=TFT_BLACK){
// Print screen with 2 lines of text
  if (debug) Serial.println("print2TextTFT ");
  setColorScheme();
  printHeaderFooter();
  tft.loadFont(Final_Frontier_28);
  tft.setTextSize(1);
  int intLen=strLine1.length();
  int intLeft=(28-intLen)/2;
  if (debug) Serial.println("intLen " + String(intLen));
  if (debug) Serial.println("intLeft " + String(intLeft));
  tft.setCursor(intLeft*3, 27);
  tft.println(strLine1);
  intLen=strLine2.length();
  intLeft=(28-intLen)/2;
  if (debug) Serial.println("intLen " + String(intLen));
  if (debug) Serial.println("intLeft " + String(intLeft));
  tft.setCursor(intLeft*3, 75);  
  tft.println(strLine2);
}

void printIntroTFT(String strLine1){
// Print introduction screen with single line of text
  setColorScheme();
  tft.setCursor(30, 27);
  tft.loadFont(Final_Frontier_28);
  tft.setTextSize(1);
  tft.println("Switch8");
  tft.loadFont(NotoSansBold15);
  // Set start of 2nd line
  int intLen=strLine1.length();
  if (debug) Serial.println("intLen " + String(intLen));
  int intLeft=(35-intLen)/2;
  if (debug) Serial.println("intLeft " + String(intLeft));
  tft.setCursor(intLeft*3, 75);  
  tft.println(strLine1);
}



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Set power to display
//  pinMode(intDisplayPower, OUTPUT);
//  digitalWrite(intDisplayPower, HIGH);
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(3); // 3 = 180 rotated
  // Make screen black
  tft.fillScreen(TFT_BLACK);
  delay(250);
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  tft.loadFont(Final_Frontier_28);
  printIntroTFT("");
  delay(250);
  printIntroTFT("Init Switch8");
  delay(500);
  printIntroTFT("Init Midi Bus");
  delay(500);
  DIN_MIDI.begin(); // MIDI init
  DIN_MIDI.turnThruOff();
  printIntroTFT("Init Leds");
  delay(500);
  Wire.begin();
  initLEDs();

  printIntroTFT("Init switches");
  keypad.addEventListener(keypadEvent); //add an event listener for this keypad, meaning all the buttons on the Switch8
  keypad.setHoldTime(800);
  keypad.setDebounceTime(80);

  delay(500);
  printIntroTFT("Ready");

}


void programChange(int intPreset){
  // Send midi preset number message

  if (intMuteMode==1){
    setMute();
  }
  if (intLockMode==1){ // In case of lock mode use the arrLockMode replacement array to enable preset x8 and x9
    intPreset=arrLockMode[intPreset];
  }
  intBankPressed = 0;
  intCurrentPreset = intPreset;
  int intMidiValue = intPreset + (intCurrentBank*10);
  DIN_MIDI.sendProgramChange(intMidiValue,intMidiChannel);
  // TODO Update display
  printData(String(intPreset+1), String(intCurrentBank));
  // Update Preset LEDs
  intPresetLEDs = 1<<intPreset;
  Wire.beginTransmission(IO_ADDR_LEDs);
  Wire.write(~intPresetLEDs);
  Wire.endTransmission();
  
}

void handleBankUp(){
  if (intBankPressed == 1)
  {
    intCurrentBank++;
    if (intCurrentBank==10){
      intCurrentBank = 0;
    }
    printData("X", String(intCurrentBank));
  } else {
    intBankPressed = 1;
  }

  
}

void handleBankDown(){
  intCurrentBank--;
  if (intCurrentBank==-1){
    intCurrentBank = 9;
  }
  printData("X", String(intCurrentBank));
  intBankPressed = 1;
}

void setMute(){
  // Create midi preset number

  switch (intMuteMode)
  {
    case 0:
      DIN_MIDI.sendControlChange(1,127,intMidiChannel);
      print2TextTFT("Mute",  "Bank: " + String(intCurrentBank));
      intMuteMode=1;
      break;
    case 1:
      DIN_MIDI.sendControlChange(1,0,intMidiChannel);
      if (intCurrentPreset!=-1){
        printData(String(intCurrentBank), String(intCurrentPreset));
      } else {
        printData(String(intCurrentBank), "X");
      }
      intMuteMode=0;
      break;
  }
}

void setProgMode(){
  // Create midi preset number

  switch (intProgMode)
  {
    case 0:
      print2TextTFT("Program",  "Mode",TFT_RED);
      intProgMode=1;
      // Execute code for listening to the buttons
      break;
    case 1:
      printData(String(intCurrentBank), String(intCurrentPreset));
      intProgMode=0;
      break;
  }
}

void handleLock(){
  // Take care of locking the bank up and bank down button
  if (debug) Serial.println("Bank lock");

  if (intLockMode==1){
    intLockMode=0;
  } else {
    intLockMode=1;
  }
  printData(String(intCurrentBank), String(intCurrentPreset));
}

/******************************************************/
//take care of some special events
void keypadEvent(KeypadEvent key){
  if (debug) Serial.println("key event " + String(key));

  switch (keypad.getState())
  {
    case PRESSED:
      if (debug) Serial.println("key pressed event");
      switch (key)
      {
        case 'a': /* 'a' to 'h' for 8 pedals- adapt it to the number you needs */
          programChange(0);
          break;
        case 'b':
          programChange(1); 
          break;
        case 'c':
          programChange(2);
          break;
        case 'd':
          programChange(3);
          break;
        case 'e':
          programChange(4);
          break;
        case 'f':
          programChange(5);
          break;
        case 'g':
          programChange(6);
          break;
        case 'h':
          programChange(7);
          break;
        case 'i':
          if (intLockMode==1){
            programChange(8);
          }
          break;
        case 'j':
          if (intLockMode==1){
            programChange(9);
          }
          break;

      }
      break;
    case RELEASED:
      if (debug) Serial.println("key released event");
      // saveState check savestate
      switch (key)
      {
        case 'i': // depending on mode either amp Reverb switch or bank down
          if (intMuteMode==0 && intLockMode==0){
            handleBankDown();  
          }
          break;
        case 'j': // depending on mode either amp Channel switch or bank up
          if (intLockMode==0){
            handleBankUp();
          }
          break;
        case 'k': // Short press wifi/coloscheme button
          // Change color scheme TODO
          initLEDs();
          intColorPreset++;
          if (intColorPreset>4){
            intColorPreset=0;
          }
          printData(String(intCurrentPreset+1), String(intCurrentBank));
          break;
        case 'l': // Short press lock bank buttons
          handleLock();
          break;
      }
      break;
    case HOLD:
      if (debug) Serial.println("key hold event");
      switch (key)
      {
        case 'i': // depending on mode either amp Reverb switch or bank down
          if (intMuteMode==0){
            setMute();   
          }
          break;
        case 'k': // Wifi toggle
          if (intMuteMode==0){
            // Handle Wifi toggle   
            setProgMode();
          }
          break;
        case 'l': // Prog mode
          if (intMuteMode==0){
            // Set prog    
            setProgMode();
          }
          break;
      }
      break;
  }
}



void loop() {
  // put your main code here, to run repeatedly:
  char key = keypad.getKey();

  unsigned long currentMillis = millis();
//  if (key) {
//    Serial.println(key);
//  } 
  // Process wait states
  if (intMuteMode==1 and currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    if (intPresetBlinkingLEDs==0)
    {
      intPresetBlinkingLEDs = intPresetLEDs;
    }
    else
    {
      intPresetBlinkingLEDs = 0;
    }
    // Write value to presets IO
    Wire.beginTransmission(IO_ADDR_LEDs);
    Wire.write(~intPresetBlinkingLEDs);
    Wire.endTransmission();
  }



}

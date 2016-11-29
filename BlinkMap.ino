#include <SPI.h>
#include <string.h>
#include <Arduino.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined (_VARIANT_ARDUINO_ZERO_)
#include <SoftwareSerial.h>
#endif

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"

#define READ_BUFSIZE    (20)
#define LEFT            9   //analog  BLUE
#define RIGHT           11  //digital PINK

#define FACTORYRESET_ENABLE         1
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

//Globals
char disconnection = '@';
char stayOn = '$';
char readNext = '#';
char turnLeft = 'L';
char turnRight = 'R';
char uturn = 'U';
uint8_t OFF = 0;
uint8_t ON  = 255;
bool leftOn;
bool rightOn;

void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

char packetbuffer[READ_BUFSIZE + 1];
char readPacket(Adafruit_BLE *ble, uint16_t timeout);
void printHex(const uint8_t * data, const uint32_t numBytes);

void setup(void)
{
  while (!Serial);
  delay(500);

  Serial.begin(115200);
  Serial.println(F("Initializing the Bluefruit LE module: "));

  if (!ble.begin(VERBOSE_MODE)) {
    error(F("Couldn't begin verbose mode"));
  }
  Serial.println("OK!");

//  if ( FACTORYRESET_ENABLE )
//  {
//    /* Perform a factory reset to make sure everything is in a known state */
//    Serial.println(F("Performing a factory reset: "));
//    if ( ! ble.factoryReset() ) {
//      error(F("Couldn't factory reset"));
//    }
//  }

  ble.echo(true);

  Serial.println("Requesting BLE info:");
  ble.info();

  ble.verbose(false);

  while (!ble.isConnected()) {
    delay(500);
  }

  // LED Activity command is only supported from 0.6.6
  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    // Change Mode LED Activity
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
  }

  // Set Bluefruit to DATA mode
  Serial.println( F("Switching to DATA mode!") );
  ble.setMode(BLUEFRUIT_MODE_DATA);

  //Setup lights
  pinMode(LEFT, OUTPUT);
  analogWrite(LEFT, OFF);

  pinMode(RIGHT, OUTPUT);
  digitalWrite(RIGHT, LOW);

}

void loop(void) {
  /* Wait for new data to arrive */
  uint8_t len = readPacket(&ble, BLE_READPACKET_TIMEOUT);
  String received;
  char command;
  bool keepLooping;
  leftOn = false;
  rightOn = false;


  if (len == 0) {
    //    Serial.println("len = 0");
    return;
  }
  Serial.print("HEX: ");
  printHex(packetbuffer, len);

  for (uint8_t i = 1; i < len; i++) {
//    writeLine("loop() packetbuffer[i]", packetbuffer[i]);
    received += packetbuffer[i];
  }
  command = received[0];
  Serial.print("[Recvd] ");
  Serial.println(received);
  Serial.print("[Command] ");
  Serial.println(command);

  if (command == readNext || command == disconnection) {
    keepLooping = false;
  } else {
    keepLooping = true;
  }

  writeLine("keeplooping", keepLooping);
  Serial.print("calling doSwitch('");
  Serial.print(command);
  Serial.print("', '");
  Serial.print(keepLooping);
  Serial.println("')");

  doSwitch(&ble, command, keepLooping);
  if(command == readNext){
    Serial.println("******NEW MOVE******");
    return;
  }
  Serial.println("AFTER DO_SWITCH");
  
  if(command == disconnection){
    Serial.println("Disconnecting...");
    ble.disconnect();
  }

}

char readPacket(Adafruit_BLE *ble, uint16_t timeout)
{
  uint16_t origtimeout = timeout, replyidx = 0;

  memset(packetbuffer, 0, READ_BUFSIZE);

  while (timeout--) {
    if (replyidx >= 20) break;

    while (ble->available()) {
//      Serial.println("Reading buffer...");
      char c =  ble->read();
      if (c == '1') { //if it's the end of the packet, stop
        replyidx = 0;
      }
//      Serial.print("CHAR c = '");
//      Serial.print(c);
//      Serial.print("'. HEX c = '");
//      Serial.print(c, HEX);
//      Serial.println("'.");
      packetbuffer[replyidx] = c;
      replyidx++;
      timeout = origtimeout;
    }

    if (timeout == 0) break;
    delay(1);
  }

  //  writeLine("after while. packetbuffer[0]", packetbuffer[0]);
  //  Serial.print("setting packetbuffer["); Serial.print(replyidx); writeLine("]", 0);
  packetbuffer[replyidx] = 0;  // null term

  if (!replyidx) { // no data or timeout
    //    Serial.println("if (!replyidx). returning 0...");
    return 0;
  }
  if (packetbuffer[0] != '1') { // doesn't start with '1' packet beginning
    Serial.println("received a packet, but doesn't start with 1. Returning 0...");
    return 0;
  }

  Serial.print("returning replyidx = ");
  Serial.println(replyidx);
  return replyidx;
}


void blinkMapDisconnected(Adafruit_BLE * ble) {
  if (!ble->isConnected()) {
    writeLine("ble", "disconnected");
  }
}

void doSwitch(Adafruit_BLE *ble, char turn, boolean looping) {
    writeLine("doSwitch(). turn", turn);
    writeLine("doSwitch(). looping", looping);
  switch (turn) {
    case 'L':   //left
      Serial.println("in doSwitch(L)");
      doLeft(ble, looping);
      Serial.println("doLeft(): breaking...");
      break;
    case 'R':   //right
      Serial.println("in doSwitch(R)");
      doRight(ble, looping);
      Serial.println("doRight(): breaking...");
      break;
    case 'U':   //uturn
      Serial.println("in doSwitch(U)");
      doUturn(ble, looping);
      Serial.println("doUturn(): breaking...");
      break;
  }
  delay(1);
}

void doLeft(Adafruit_BLE *ble, bool looping) {
  Serial.println("doLeft() = RIGHT, LOW");
  digitalWrite(RIGHT, LOW);
  rightOn = false;
  leftOn = true;

  while (!ble-> available() && looping == true) {
    Serial.println("doLeft() = LEFT, ON");
    analogWrite(LEFT, ON);
    delay(500);
    Serial.println("doLeft() = LEFT, OFF");
    analogWrite(LEFT, OFF);
    delay(500);
  }
}

void doRight(Adafruit_BLE *ble, boolean looping) {
  Serial.println("doRight() = LEFT, OFF");
  analogWrite(LEFT, OFF);
  leftOn = false;
  rightOn = true;

  while (!ble-> available() && looping == true) {
    Serial.println("doRight() = RIGHT, HIGH");
    digitalWrite(RIGHT, HIGH);
    delay(500);
    Serial.println("doRight() = RIGHT, LOW");
    digitalWrite(RIGHT, LOW);
    delay(500);
  }
}
void doUturn(Adafruit_BLE *ble, boolean looping) {
  while (!ble-> available() && looping == true) {
    Serial.println("doUturn() = LEFT, ON");
    //Turn them both on
    analogWrite(LEFT, ON);
    leftOn = true;
    Serial.println("doUturn() = RIGHT, HIGH");
    digitalWrite(RIGHT, HIGH);
    rightOn = true;
  }
}

void writeLine(char *prompt, char text) {
  Serial.print(prompt);
  Serial.print(" = '");
  Serial.print(text);
  Serial.println("'");
}

void writeLine(char *prompt, String text) {
  Serial.print(prompt);
  Serial.print(" = '");
  Serial.print(text);
  Serial.println("'");
}

void writeLine(char *prompt, uint16_t text) {
  Serial.print(prompt);
  Serial.print(" = '");
  Serial.print(text);
  Serial.println("'");
}

void writeLine(char *prompt, int text) {
  Serial.print(prompt);
  Serial.print(" = '");
  Serial.print(text);
  Serial.println("'");
}

void writeLine(char *prompt, bool text) {
  Serial.print(prompt);
  Serial.print(" = '");
  Serial.print(text);
  Serial.println("'");
}

void printHex(const uint8_t * data, const uint32_t numBytes)
{
  uint32_t szPos;
  for (szPos = 0; szPos < numBytes; szPos++)
  {
    Serial.print(F("0x"));
    // Append leading 0 for small values
    if (data[szPos] <= 0xF)
    {
      Serial.print(F("0"));
      Serial.print(data[szPos] & 0xf, HEX);
    }
    else
    {
      Serial.print(data[szPos] & 0xff, HEX);
    }
    // Add a trailing space if appropriate
    if ((numBytes > 1) && (szPos != numBytes - 1))
    {
      Serial.print(F(" "));
    }
  }
  Serial.println();
}

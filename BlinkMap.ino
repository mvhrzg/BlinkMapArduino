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
char connection = "@";
char stayOn = "$";
char turnOff = "#";
char turnLeft = 'l';
char turnRight = 'r';
char uturn = 'u';
uint8_t OFF = 0;
uint8_t ON  = 255;
bool leftOn;
bool rightOn;

void error(const __FlashStringHelper*err) {
  Serial.println(err);
  //  while (1);
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

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ) {
      error(F("Couldn't factory reset"));
    }
  }

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

  //Send this line to Android so we know the moment we're connected
  ble.println(connection);  //if this doesn't work, try ble.write (although it prob won't work bc ble can only write when available

  //Setup lights
  pinMode(LEFT, OUTPUT);
  analogWrite(LEFT, OFF);

  pinMode(RIGHT, OUTPUT);
  digitalWrite(RIGHT, LOW);

}

void loop(void) {
  /* Wait for new data to arrive */
  uint8_t len = readPacket(&ble, BLE_READPACKET_TIMEOUT);
  String data;
  char command;
  bool breakWhile = false;
  leftOn = false;
  rightOn = false;

  if (len == 0) {
    return;
  }

  printHex(packetbuffer, len);

  for (uint8_t i = 1; i < len; i++) {
    data += packetbuffer[i];
  }
  data.trim();
  command = data[0];
  Serial.print("[Recvd] ");
  Serial.println(command);
  //  ble.println("Hello from Arduino");

  if (command == turnOff) {
    breakWhile = true;
  }

  //While blinkMap wants lights on
    while(!breakWhile){
      doSwitch(command);
    }

  //Broke out of while because we got told to turn one or both lights off
  //If the left light is on, turn it off
  if (leftOn) {
    analogWrite(LEFT, OFF);
    leftOn = false;
  }

  //Separate if to handle uturns
  if (rightOn) {
    digitalWrite(RIGHT, LOW);
    rightOn = false;
  }


}

void doSwitch(char turn) {
  switch (turn) {
    case 'l':   //left
      doLeft();
      break;
    case 'r':   //right
      doRight();
      break;
    case 'u':   //uturn
      doUturn();

      break;
  }
}

void doLeft() {
  digitalWrite(RIGHT, LOW);
  rightOn = false;
  leftOn = true;

  analogWrite(LEFT, ON);
  delay(500);
  analogWrite(LEFT, OFF);
  delay(500);
}

boolean doRight() {
  analogWrite(LEFT, OFF);
  leftOn = false;
  rightOn = true;

  digitalWrite(RIGHT, HIGH);
  delay(500);
  digitalWrite(RIGHT, LOW);
  delay(500);
}
void doUturn() {
  //Turn them both on
  analogWrite(LEFT, ON);
  leftOn = true;
  digitalWrite(RIGHT, HIGH);
  rightOn = true;
}

char readPacket(Adafruit_BLE *ble, uint16_t timeout)
{
  uint16_t origtimeout = timeout, replyidx = 0;

  memset(packetbuffer, 0, READ_BUFSIZE);

  while (timeout--) {
    if (replyidx >= 20) break;

    while (ble->available()) {
      char c =  ble->read();
      if (c != "1") {
        packetbuffer[replyidx] = c;
        replyidx++;
      }
      timeout = origtimeout;
    }

    if (timeout == 0) break;
    delay(1);
  }

  packetbuffer[replyidx] = 0;  // null term

  if (!replyidx)  // no data or timeout
    return 0;
  if (packetbuffer[0] != '1') { // doesn't start with '1' packet beginning
    return 0;
  }

  return replyidx;
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

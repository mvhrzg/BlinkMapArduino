#include <Arduino.h>
#include <SPI.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined (_VARIANT_ARDUINO_ZERO_)
#include <SoftwareSerial.h>
#endif

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "Adafruit_BLEGatt.h"

#include "BluefruitConfig.h"

#define FACTORYRESET_ENABLE         1
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
Adafruit_BLEGatt gatt(ble);

void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

void setup(void) {
  while (!Serial);
  delay(500);

  pinMode(13, OUTPUT);

  Serial.begin(115200);
  Serial.println("Initializing the Bluefruit LE module: ");
  //  ble.factoryReset();

  if (!ble.begin(VERBOSE_MODE)) {
    error(F("Couldn't begin verbose mode"));
  }
  Serial.println("OK!");
  ble.echo(true);
  ble.verbose(true);

  Serial.println("Requesting BLE info:");
  ble.info();

  while (!ble.isConnected()) {
    delay(500);
  }

  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    // Change Mode LED Activity
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
    //Send AT Commands
    //    ble.sendCommandCheckOK("AT+GAPDEVNAME=BlinkMap");
    //    ble.sendCommandCheckOK("ATZ");
//    ble.sendCommandCheckOK("AT+GATTADDSERVICE=UUID128=00001800-0000-1000-8000-00805f9b34fb");
    ble.sendCommandCheckOK("AT+GATTADDSERVICE=UUID128=6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
    //  ble.sendCommandCheckOK("AT+GATTADDSERVICE=UUID128=6E400003-B5A3-F393-E0A9-E50E24DCCA9E");
    //  ble.sendCommandCheckOK("AT+GAPSETADVDATA=02-01-06");
  }




}

void loop(void) {

  //Check for input
  char n, inputs[BUFSIZE + 1];

  if (Serial.available()) {
    n = Serial.readBytes(inputs, BUFSIZE);
    inputs[n] = 0;
    //Send to Bluefruit
    Serial.print("Sending: ");
    Serial.println(inputs);

    //Send input data to host via Bluefruit
    ble.print(inputs);
  }

  // Echo received data
  while ( ble.available() )
  {
    Serial.println("ble available");
    int c = ble.read();

    Serial.print((char)c);

    // Hex output too, helps w/debugging!
    Serial.print(" [0x");
    if (c <= 0xF) Serial.print(F("0"));
    Serial.print(c, HEX);
    Serial.print("] ");
  }

  delay(1000);



  //  if (ble.isConnected()) {
  //    digitalWrite(13, HIGH);
  //    Serial.println("Connected reading: \t");
  //    Serial.println(Serial.read());
  //    //    Serial.println(ble.read());
  //  }
  //  else {
  //    Serial.println("Disconnected");
  //    digitalWrite(13, LOW);
  //  }
  //  delay(2000);

}

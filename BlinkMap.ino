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
#define LEFT            9
#define RIGHT           11

#define FACTORYRESET_ENABLE         1
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
//Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);

uint16_t blinkMapServiceUUID = "0xA000";
uint16_t readCharUUID        = "0xA001";
uint16_t writeCharUUID       = "0xA002";

int32_t clientServiceId;
int32_t gattServiceId;
int32_t defaultGattServiceId;
int32_t gattNotifiableCharId;
int32_t gattWritableResponseCharId;
int32_t gattWritableNoResponseCharId;
int32_t gattReadableCharId;
int32_t gattListReply;

void error(const __FlashStringHelper*err) {
  Serial.println(err);
  //  while (1);
}

char packetbuffer[READ_BUFSIZE + 1];
char readPacket(Adafruit_BLE *ble, uint16_t timeout);
void printHex(const uint8_t * data, const uint32_t numBytes);

void setup(void)
{
  digitalWrite(LEFT, LOW);
  pinMode(LEFT, OUTPUT);

  digitalWrite(RIGHT, LOW);
  pinMode(RIGHT, OUTPUT);

  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  
  while (!Serial);
  delay(500);

  Serial.begin(115200);
  Serial.println(F("Initializing the Bluefruit LE module: "));

  ble.available();

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
  //  ble.setInterCharWriteDelay(5); // 5 ms

  ble.verbose(false);

  while (!ble.isConnected()) {
    delay(500);
  }

  //  ble.sendCommandCheckOK("AT+GATTCLEAR");
  //
  //    Serial.println(F("Adding UART Service UUID"));
  //    success = ble.sendCommandWithIntReply(F("AT+GATTADDSERVICE=UUID128=6E-40-00-01-B5-A3-F3-93-E0-A9-E5-0E-24-DC-CA-9E"), &defaultGattServiceId);
  //    if (! success) {
  //      error(F("Could not add CLIENT_UUID"));
  //    }
  //    else {
  //      Serial.print(F("Success at "));
  //      Serial.println(defaultGattServiceId);
  //      Serial.print(F("Adding RX characteristic: "));
  //      success = ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=6E-40-00-03-B5-A3-F3-93-E0-A9-E5-0E-24-DC-CA-9E,PROPERTIES"), &gattNotifiableCharId);
  //      if (! success) {
  //        Serial.println(F("Couldn't set up RX UUID"));
  //      }
  //      else {
  //        Serial.print(F("Success at "));
  //        Serial.print(gattNotifiableCharId);
  //      }
  //    }
  //
  //    Serial.print(F("Adding CLIENT_UUID: "));
  //    success = ble.sendCommandWithIntReply( F("AT+GATTADDSERVICE=UUID128=00-00-2a-29-00-00-10-00-80-00-00-80-5f-9b-34-fb"), &clientServiceId);
  //    if (! success) {
  //      error(F("Could not add CLIENT_UUID"));
  //    }
  //    else {
  //      Serial.print(F("Success at "));
  //      Serial.println(clientServiceId);
  //      Serial.print(F("Adding RX characteristic: "));
  //      success = ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=6E-40-00-03-B5-A3-F3-93-E0-A9-E5-0E-24-DC-CA-9E,PROPERTIES"), &gattNotifiableCharId);
  //      if (! success) {
  //        Serial.println(F("Couldn't set up RX UUID"));
  //      }
  //      else {
  //        Serial.print(F("Success at"));
  //        Serial.println(gattNotifiableCharId);
  //      }
  //    }
  //
  //  /* Add the Custom GATT Service definition */
  //  /* Service ID should be 1 */
  //  Serial.print(F("Adding the Custom GATT Service definition: "));
  //  success = ble.sendCommandWithIntReply( F("AT+GATTADDSERVICE=UUID128=00-77-13-12-11-00-00-00-00-00-AB-BA-0F-A1-AF-E1"), &gattServiceId);
  //  if (! success) {
  //    error(F("Could not add Custom GATT service"));
  //  }
  //  else {
  //    Serial.print(F("Success at "));
  //    Serial.println(gattServiceId);
  //  }
  //
  //  /* Add the Readable/Notifiable characteristic - this characteristic will be set every time the color of the LED is changed */
  //  /* Characteristic ID should be 1 */
  //  /* 0x00FF00 == RGB HEX of GREEN */
  //  Serial.print(F("Adding the Notifiable characteristic: "));
  //  success = ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID128=6E-40-00-03-B5-A3-F3-93-E0-A9-E5-0E-24-DC-CA-9E,PROPERTIES=0x02,MIN_LEN=1, MAX_LEN=20, VALUE=0x00FF00"), &gattNotifiableCharId);
  //  if (! success) {
  //    error(F("Could not add Custom Notifiable characteristic"));
  //  } else {
  //    Serial.print(F("Success: "));
  //    Serial.println(gattNotifiableCharId);
  //  }
  //  //00-68-42-01-14-88-59-77-42-42-AB-BA-0F-A1-AF-E1
  //
  //  /* Add the Writable characteristic - an external device writes to this characteristic to change the LED color */
  //  /* Characteristic ID should be 2 */
  //  Serial.print(F("Adding the Writable with Response characteristic: "));
  //  success = ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID128=6E-40-00-02-B5-A3-F3-93-E0-A9-E5-0E-24-DC-CA-9E,PROPERTIES=0x08,MIN_LEN=1, MAX_LEN=20, VALUE='Arduino Hey'"), &gattWritableNoResponseCharId);
  //  if (! success) {
  //    error(F("Could not add Custom Writable characteristic"));
  //  } else {
  //    Serial.print(F("Success at"));
  //    Serial.println(gattWritableNoResponseCharId);
  //  }
  //  //00-69-42-03-00-77-12-10-13-42-AB-BA-0F-A1-AF-E1
  //
  //  /* Add the Readable characteristic - external devices can query the current LED color using this characteristic */
  //  /* Characteristic ID should be 3 */
  //  Serial.print(F("Adding the Readable characteristic: "));
  //  success = ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID128=00-70-42-04-00-77-12-10-13-42-AB-BA-0F-A1-AF-E1,PROPERTIES=0x02,MIN_LEN=1, MAX_LEN=20, VALUE=0x00FF00"), &gattReadableCharId);
  //  if (! success) {
  //    error(F("Could not add Custom Readable characteristic"));
  //  } else {
  //    Serial.print(F("Success at "));
  //    Serial.println(gattReadableCharId);
  //  }
  //
  //  /* Add the Custom GATT Service to the advertising data */
  //  //0x1312 from AT+GATTLIST - 16 bit svc id
  ////  Serial.print(F("Request GATTLIST: "));
  ////  success = ble.sendCommandWithIntReply("AT+GATTLIST", &gattListReply);
  ////  if (!success) {
  ////    Serial.println(F("Couldn't get GATTLIST"));
  ////  } else {
  ////    Serial.print("Got GATTLIST:" );
  ////    Serial.println(gattListReply);
  ////  }
  //
  //  Serial.print(F("Adding Custom GATT Service UUID to the advertising payload: "));
  //  ble.sendCommandCheckOK(F("AT+GAPSETADVDATA=05-02-11-18-0a-18"));
  //  //ble.sendCommandCheckOK( F("AT+GAPSETADVDATA=02-01-06-03-02-12-13") );
  //
  //  /* Reset the device for the new service setting changes to take effect */
  //  Serial.print(F("Performing a SW reset (service changes require a reset): "));
  //  ble.sendCommandCheckOK(F("AT+ATZ"));

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

  Serial.println();
}

void loop(void) {
  digitalWrite(LEFT, LOW);
  digitalWrite(RIGHT, LOW);
  /* Wait for new data to arrive */
  uint8_t len = readPacket(&ble, BLE_READPACKET_TIMEOUT);
  String command;
  
  if (len == 0) {
    return;
  }

  for (uint8_t i = 1; i < len; i++) {
//    Serial.print("len == ");
//    Serial.print(len);
//    Serial.print(". packetbuffer[");
//    Serial.print(i);
//    Serial.print("] = ");
//    Serial.println(packetbuffer[i]);
    command += packetbuffer[i];
  }
  Serial.print("[Recvd] .");
  Serial.print(command);
  Serial.println(".");
  if(command.compareTo("left")){
    Serial.println("command.equals(left)");
//    digitalWrite(LEFT, HIGH);
    digitalWrite(13, HIGH);
    delay(1000);
  }else if(command.compareTo("right")){
    Serial.println("command.equals(right)");
//    digitalWrite(RIGHT, HIGH);
    digitalWrite(13, LOW);
    delay(1000);
  }else{
    Serial.println("!command.equals()");
//    digitalWrite(LEFT, LOW);
//    digitalWrite(RIGHT, LOW);
  }
}

char readPacket(Adafruit_BLE *ble, uint16_t timeout)
{
  uint16_t origtimeout = timeout, replyidx = 0;

  memset(packetbuffer, 0, READ_BUFSIZE);

  while (timeout--) {
    if (replyidx >= 20) break;

    while (ble->available()) {
      char c =  ble->read();
      Serial.print("all c:");
      Serial.println(c);
      if (c != "1") {
        Serial.print("c != 1: ");
        Serial.println(c);
        packetbuffer[replyidx] = c;
        replyidx++;
      }
      timeout = origtimeout;
//      Serial.print("timeout = ");
//      Serial.println(timeout);
      if (c == "-")
        Serial.println("breaking...");
      break;
    }

    if (timeout == 0) break;
    delay(1);
  }

  packetbuffer[replyidx] = 0;  // null term

  if (!replyidx)  // no data or timeout
    return 0;
  if (packetbuffer[0] != '1') { // doesn't start with '1' packet beginning
//    Serial.print("packetbuffer[0] from readPacket = ");
//    Serial.println(packetbuffer[0]);
//    Serial.println("returning len...");
    return 0;
  }

  // checksum passed!
//  Serial.print("Checksum passed. replyidx = ");
//  Serial.println(replyidx);
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

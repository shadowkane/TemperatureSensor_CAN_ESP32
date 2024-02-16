/*
Author: Habib JOMAA
Date: 04/02/2024
App: TemperatureSensor CAN ESP32
Description: Temperature sensor device collects data and send it via CAN protocol, project based on ESP32 and uses TTGO T-Beam as dev board

How it works:
  Temperatue sensor is LM335 witch calibration, GPIO_35
  Read temperature, filter data, send it to CAN bus
  cycle is 1s
  CAN description:
    CAN bus used for test is a 1 wire can bus without transiver
    Tx GPIO_21
    Rx GPIO_34
    speed 50khz
    Payload length is 4 bytes
  -- Read code comments for more informations --


Note: THIS PROJECT IS STILL IN PROGRESS, CHANGES WILL BE MADE
*/

#include <stdlib.h>
#include <stdint.h>
#include <Arduino.h>
#include <CAN.h>

#define LM335_PIN 34

#define CAN_TX  25
#define CAN_RX  35

// CAN ID: [bit8-bit15] for device type (max device types is 16)
//         [bit0-bit7] for device id for specific device type (max device per type is 16)
//          0 is for ECU or master devices
//          1 is for temperature sensor
// 0x10-0x1f is tempearature sensor range
#define SENSOR_PROFILE_DEFAULT_CAN_ID 0x10
#define SENSOR_PROFILE_TEMPERATURE_SCALE  100 // we keep 2 values from fractional part
// reserver first byte for future use (maybe sensor description, profile, sub id, status etc)
// max tempearature value is approximately 50°C (hardware limitation, uses LM335 with 3.3v that can only reach 53°C max temperature)
// we scale temperature measurement by 100 which mean max value is 5000 (in hex: 0x1388), we need 2 bytes
// we reserve 1 more byte for future use (in case we need to expand value)
// 1 byte (reserved) + 2 bytes (data) + 1 byte (reserver) => payload length is 4 bytes
#define PAYLOAD_LENGTH 4

void vPrepareCanFrame(float data, uint8_t *pu8Buffer);

uint8_t upCanPayload2Send[PAYLOAD_LENGTH];
int iTempDegital;
float fTempRaw;

void setup() {
  
  // Serial port config
  Serial.begin(115200);

  // CAN config
  CAN.setPins(CAN_RX, CAN_TX);
  if(CAN.begin(50E3))
  {
    Serial.println("CAN Initiated, speed is 500Kbps");
  }
  else
  {
    Serial.println("Failed to initiate CAN");
  }

  Serial.println("Start Temperature sensor CAN ESP32");
  delay(1000);
}

int iFilterSize = 100;
int i;

void loop() {
  // step in mV for 12 bit resolution => step = 3300/(2^12 -1)
  // 2^12-1= 4096-1= 4095
  // for the sake of normalization we will call step a resolution
  // resolution = 3300/4095
  // Digital Value = Analogic Value/resolution => Vin = D * resolution
  iTempDegital = 0;
  for(i=0; i<iFilterSize; i++){
    iTempDegital += analogRead(LM335_PIN);
    delay(10);
  }
  iTempDegital /=iFilterSize;
  fTempRaw = iTempDegital * 3300 / 4095.0;
  Serial.printf("iTempDegital=%d | TempRaw mV= %.3f | Temp K = %.3f | Temp C = %.3f\n", iTempDegital, fTempRaw, fTempRaw/10, (fTempRaw/10)-273.15);
  fTempRaw = (fTempRaw/10)-273.15;
  // prepare payload
  vPrepareCanFrame(fTempRaw, upCanPayload2Send);

  // prepare CAN frame
  CAN.beginPacket(SENSOR_PROFILE_DEFAULT_CAN_ID);
  for(i=0; i<PAYLOAD_LENGTH; i++){
    CAN.write(upCanPayload2Send[i]);
  }
  CAN.endPacket(); // send message

  //delay(1000);
}

void vPrepareCanFrame(float data, uint8_t *pu8Buffer){
  pu8Buffer[0]  = 0x00;
  uint16_t data_decm = data*100.0;
  memcpy(pu8Buffer+1, (uint8_t*)(&data_decm), 2);
  pu8Buffer[3] = 0x00; 
}
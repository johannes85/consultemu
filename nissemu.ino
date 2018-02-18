#include "constants.h"

typedef void(*CommandFunction) ();

uint8_t initialized = 0;
uint8_t incomingByte;
uint8_t commandHandeled = 0;
CommandFunction commandFunction;

uint16_t currentSpeed = 0;
uint16_t currentRpm = 0;
uint8_t currentCoolantTemp = 0;
uint8_t currentIgnitionTiming = 0;

byte buffer[250];
byte bufferSize;
byte responseBuffer[250];
byte responseBufferSize;

void setup()
{
  Serial.begin(9600);
  pinMode(A0, INPUT);
}

void handleCommandReadRegister() {

  currentRpm = map(analogRead(A0), 0, 1023, 0, 7200) / 12.5;
  
  responseBufferSize = 0;
  
  for (uint8_t i = 0; i < bufferSize; i++) {
    switch (buffer[i]) {
      case ECU_REGISTER_COOLANT_TEMP:
        currentCoolantTemp = map(analogRead(A0), 0, 1023, 0, 200);
        responseBuffer[responseBufferSize] = currentCoolantTemp + 50;
        responseBufferSize += 1;
        break;

      case ECU_REGISTER_TACH_LSB:
        responseBuffer[responseBufferSize] = currentRpm & 0x00FF;
        responseBufferSize += 1;
        break;

      case ECU_REGISTER_TACH_MSB:
        responseBuffer[responseBufferSize] = (currentRpm & 0xFF00) >> 8;
        responseBufferSize += 1;
        break;

      case ECU_REGISTER_VEHICLE_SPEED:
        currentSpeed = map(analogRead(A0), 0, 1023, 0, 400) / 2;
        responseBuffer[responseBufferSize] = currentSpeed;
        responseBufferSize += 1;

      case ECU_REGISTER_BATTERY_VOLTAGE:
        responseBuffer[responseBufferSize] = 0x00;
        responseBufferSize += 1;

      case ECU_REGISTER_IGNITION_TIMING:
        currentIgnitionTiming = 110 - map(analogRead(A0), 0, 1023, 10, 30);
        responseBuffer[responseBufferSize] = currentIgnitionTiming;
        responseBufferSize += 1;

      case ECU_REGISTER_LEFT_O2:
      case ECU_REGISTER_RIGHT_O2:
      case ECU_REGISTER_AAC_VALVE:
      case ECU_REGISTER_AF_ALPHA:
      case ECU_REGISTER_DIGITAL_BIT_REGISTER:
      case ECU_REGISTER_DIGITAL_CONTROL_REGISTER_E:
      case ECU_REGISTER_DIGITAL_CONTROL_REGISTER_F:
      case ECU_REGISTER_INJECTION_TIME_LSB:
      case ECU_REGISTER_INJECTION_TIME_MSB:
      case ECU_REGISTER_LEFT_MAF_MSB:
      case ECU_REGISTER_LEFT_MAF_LSB:
      case ECU_REGISTER_RIGHT_MAF_MSB:
      case ECU_REGISTER_RIGHT_MAF_LSB:
        responseBuffer[responseBufferSize] = 0x00;
        responseBufferSize += 1;
        break;

      case ECU_COMMAND_READ_REGISTER:
        break;
    }
  }
}

void loop()
{

  if (Serial.available() > 0) {
    if (initialized == 1) {
      incomingByte = Serial.read();
      Serial.write(~incomingByte);
      bufferSize = Serial.readBytesUntil(PROTOCOL_TEMINATE, buffer, 250);
      commandHandeled = 0;
      switch (incomingByte) {
        case ECU_COMMAND_CLEAR_CODES:
          break;

        case ECU_COMMAND_ECU_INFO:
          break;

        case ECU_COMMAND_READ_REGISTER:
          commandHandeled = 1;
          commandFunction = handleCommandReadRegister;
          break;

        case ECU_COMMAND_SELF_DIAG:
          break;

        case ECU_COMMAND_STOP_STREAM:
          break;

        case ECU_COMMAND_TERM:
        default:

          break;
      }

      if (commandHandeled == 1) {
        while (1) {
          Serial.write(PROTOCOL_FRAME_START);

          commandFunction();
          Serial.write(responseBufferSize);
          for (uint8_t i = 0; i < responseBufferSize; i++) {
            Serial.write(responseBuffer[i]);
          }

          if (Serial.available() > 0) {
            if (Serial.read() == ECU_COMMAND_TERM) {
              break;
            }
          }
        }
      }
    } else {
      Serial.readBytes(buffer, 3);
      if (
        buffer[0] == 0xFF &&
        buffer[1] == 0xFF &&
        buffer[2] == 0xEF
      ) {
        Serial.write(0x10);
        initialized = 1;
      }
    }

  }

}

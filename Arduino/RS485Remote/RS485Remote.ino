/* RS485 Remote Control 
 * (C) by Michael Stuermer <ms@mallorn.de>
 * 
 * Hardware: Arduino Nano
 * 
 * Upload settings:
 *   Board: Arduino Nano
 *   CPU: ATmega328P
 */   
#include <SoftwareSerial.h>

#define SLAVE_ID 0

#define PIN_SERIAL1_RX 3
#define PIN_SERIAL1_TX 4

#define PIN_SW0  A0
#define PIN_SW4  A1
#define PIN_SW3  A2
#define PIN_SW2  A3
#define PIN_SW1  A4
#define PIN_LED0 6
#define PIN_LED4 5
#define PIN_LED3 11
#define PIN_LED2 9
#define PIN_LED1 10

SoftwareSerial Serial1(PIN_SERIAL1_RX, PIN_SERIAL1_TX);

int brightness(bool enabled) {
  return enabled ? 0 : 245;
}

void setSelectedLed(int num) {
  analogWrite(PIN_LED0, brightness(num == 0));
  analogWrite(PIN_LED1, brightness(num == 1));
  analogWrite(PIN_LED2, brightness(num == 2));
  analogWrite(PIN_LED3, brightness(num == 3));
  analogWrite(PIN_LED4, brightness(num == 4));
}

void setErrorLed() {
  int t = (millis() / 200) % 5;
  analogWrite(PIN_LED0, brightness(t == 0));
  analogWrite(PIN_LED1, brightness(t == 1));
  analogWrite(PIN_LED2, brightness(t == 2));
  analogWrite(PIN_LED3, brightness(t == 3));
  analogWrite(PIN_LED4, brightness(t == 4));
}

void setup() {
  Serial.begin(57600);
  while (!Serial) {
    // wait for availability
  }

  Serial1.begin(9600);
  
  pinMode(PIN_SW0, INPUT_PULLUP);
  pinMode(PIN_SW1, INPUT_PULLUP);
  pinMode(PIN_SW2, INPUT_PULLUP);
  pinMode(PIN_SW3, INPUT_PULLUP);
  pinMode(PIN_SW4, INPUT_PULLUP);
  pinMode(PIN_LED0, OUTPUT);
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PIN_LED3, OUTPUT);
  pinMode(PIN_LED4, OUTPUT);
  setSelectedLed(0);
}

int getPressed() {
  if (digitalRead(PIN_SW0) == LOW) {
    return 0;
  } else if (digitalRead(PIN_SW1) == LOW) {
    return 1;
  } else if (digitalRead(PIN_SW2) == LOW) {
    return 2;
  } else if (digitalRead(PIN_SW3) == LOW) {
    return 3;
  } else if (digitalRead(PIN_SW4) == LOW) {
    return 4;
  } else {
    return -1;
  }
}

int pendingTransmitAnt = -1;
void transmitAnt(int newAnt) {
  pendingTransmitAnt = newAnt;
  // will be picked up in next slave response
}

int receivedAnt = -1;
void processCommand(const char* cmd) {
  if (cmd[0] == 'A' && cmd[1] == 'N' && cmd[2] >= '0'&& cmd[2] <= '4' && cmd[3] == ';') {
    int newAnt = cmd[2] - '0';
    transmitAnt(newAnt);
    Serial.print("AN");
    Serial.print(newAnt);
    Serial.print(";");
  } else if (cmd[0] == 'A' && cmd[1] == 'N' && cmd[2] == ';') {
    Serial.print("AN");
    Serial.print(receivedAnt >= 0 ? receivedAnt : 0);
    Serial.print(";");
  }
}

char line[128] = {0};
int lineLen = 0;
uint64_t lastMasterMessage = 0;
void loop() {
  while (Serial1.available() > 0) {
    byte masterData = Serial1.read();
    // we read any data, but respond only to our slave id
    int newReceivedAnt = masterData & 0x0f;
    if (newReceivedAnt != receivedAnt) {
      receivedAnt = newReceivedAnt;
      setSelectedLed(receivedAnt);
    }
    if (pendingTransmitAnt >= 0 && (masterData >> 4) == SLAVE_ID) {
      Serial1.write((uint8_t)((SLAVE_ID << 4) | (uint8_t) pendingTransmitAnt));
      pendingTransmitAnt = -1;
    }
    lastMasterMessage = millis();
  }

  if (millis() - lastMasterMessage > 1000) {
    setErrorLed();
  } else {
    int pressed = getPressed();
    if (pressed >= 0 && pressed != receivedAnt) {
      transmitAnt(pressed);
    }
  
    while (Serial.available() > 0) {
      char c = Serial.read();
      if (c == '\r' || c == '\n') {
        lineLen = 0;
      } else if (lineLen < sizeof(line) - 1) {
        line[lineLen] = c;
        lineLen++;
        if (c == ';') {
          line[lineLen] = 0;
          processCommand((const char*) &line);
        }
      }
    }
  }
}

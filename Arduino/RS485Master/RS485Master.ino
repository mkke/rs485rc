/* RS485 Master 
 * (C) by Michael Stuermer <ms@mallorn.de>
 * 
 * Hardware: Arduino Nano
 * 
 * Upload settings:
 *   Board: Arduino Nano
 *   CPU: ATmega328P
 */   
#include <SoftwareSerial.h>

#define PIN_SERIAL1_RX 3
#define PIN_SERIAL1_TX 4

#define PIN_RELAIS1 6
#define PIN_RELAIS2 7
#define PIN_RELAIS3 8
#define PIN_RELAIS4 9

SoftwareSerial Serial1(PIN_SERIAL1_RX, PIN_SERIAL1_TX);

int ant = 0;
void setAnt(int newAnt) {
  ant = newAnt;

  digitalWrite(PIN_RELAIS1, LOW);
  digitalWrite(PIN_RELAIS2, LOW);
  digitalWrite(PIN_RELAIS3, LOW);
  digitalWrite(PIN_RELAIS4, LOW);
  delay(10);
  switch (ant) {
    case 1:
      digitalWrite(PIN_RELAIS1, HIGH);
      break;
    case 2:
      digitalWrite(PIN_RELAIS2, HIGH);
      break;
    case 3:
      digitalWrite(PIN_RELAIS3, HIGH);
      break;
    case 4:
      digitalWrite(PIN_RELAIS4, HIGH);
      break;
  }
}

void setup() {
  digitalWrite(PIN_RELAIS1, LOW);
  digitalWrite(PIN_RELAIS2, LOW);
  digitalWrite(PIN_RELAIS3, LOW);
  digitalWrite(PIN_RELAIS4, LOW);
  pinMode(PIN_RELAIS1, OUTPUT);
  pinMode(PIN_RELAIS2, OUTPUT);
  pinMode(PIN_RELAIS3, OUTPUT);
  pinMode(PIN_RELAIS4, OUTPUT);
  
  Serial.begin(57600);
  while (!Serial) {
    // wait for availability
  }

  Serial1.begin(9600);
  setAnt(0);
}

void loop() {
  // bit 0..3 antenna no
  // bit 4..7 slave no

  for (int slave = 0; slave <= 7; slave++) {
    Serial.print(slave);
    Serial1.write((uint8_t)((slave << 4) | ant));

    uint64_t timeout = millis() + 20;
    while (Serial1.available() == 0 && millis() < timeout) {
      delay(5);
    }
    if (Serial1.available() > 0) {
      byte newAnt = Serial1.read() & 0x0f;
      Serial.print("received new ant ");
      Serial.println(newAnt);
      if (newAnt != ant) {
        setAnt(newAnt);
      }
    }
  }
}

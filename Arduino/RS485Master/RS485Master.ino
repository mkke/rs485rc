/* RS485 Master 
 * (C) by Michael Stuermer <ms@mallorn.de>
 * 
 * Hardware: Arduino Nano
 * 
 * Upload settings:
 *   Board: Arduino Nano
 *   CPU: ATmega328P
 *   
 * message to remote:
 *   bit 0..1 channel 0 antenna no
 *   bit 2..3 channel 1 antenna no
 *   bit 4..7 slave no
 *   
 * message from remote:
 *   bit 0..1 channel no
 *   bit 2..3 antenna no
 *   bit 4..7 slave no
 */   

#include <SoftwareSerial.h>

#define PIN_SERIAL1_RX 3
#define PIN_SERIAL1_TX 4

#define PIN_RELAIS1 6
#define PIN_RELAIS2 7
#define PIN_RELAIS3 8
#define PIN_RELAIS4 9

#define CHANNELS 2
#define MAX_PINS_PER_CHANNEL 4
#define NONE -1
#define END -2
static int relaisPins[CHANNELS][MAX_PINS_PER_CHANNEL] = {
  {NONE, PIN_RELAIS1, PIN_RELAIS2, END}, 
  {NONE, PIN_RELAIS3, PIN_RELAIS4, END}};

SoftwareSerial Serial1(PIN_SERIAL1_RX, PIN_SERIAL1_TX);

int ant[CHANNELS] = {0};
void setAnt(int channel, int newAnt) {
  // first disable old relais
  for (int pinIdx = 0; pinIdx < MAX_PINS_PER_CHANNEL; pinIdx++) {
    int pin = relaisPins[channel][pinIdx];
    if (pin == END) {
      break;
    } else if (pin != NONE) {
      digitalWrite(pin, LOW);
    }
  }
  delay(10);

  // enable new relais
  for (int pinIdx = 0; pinIdx < MAX_PINS_PER_CHANNEL; pinIdx++) {
    int pin = relaisPins[channel][pinIdx];
    if (pin == END) {
      break;
    } else if (pin != NONE && pinIdx == newAnt) {
      digitalWrite(pin, HIGH);
    }
  }

  ant[channel] = newAnt;
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
  for (int chan = 0; chan < CHANNELS; chan++) {
    setAnt(chan, 0);
  }
}

void loop() {
  for (int slave = 0; slave <= 7; slave++) {
    Serial.print(slave);
    Serial1.write((uint8_t)((slave << 4) | ant[0] | (ant[1] << 2)));

    uint64_t timeout = millis() + 20;
    while (Serial1.available() == 0 && millis() < timeout) {
      delay(5);
    }
    if (Serial1.available() > 0) {
      byte newChAnt = Serial1.read() & 0x0f;
      byte newChannel = newChAnt & 0x03;
      byte newAnt = (newChAnt & 0x0c) >> 2;
      Serial.print("received new channel ");
      Serial.print(newChannel);
      Serial.print(" ant ");
      Serial.println(newAnt);
      if (newAnt != ant[newChannel]) {
        setAnt(newChannel, newAnt);
      }
    }
  }
}

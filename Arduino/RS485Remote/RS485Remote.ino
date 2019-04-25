/* RS485 Remote Control 
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

#define CHANNELS 2
#define MAX_PINS_PER_CHANNEL 4
#define END -2
static int ledPins[CHANNELS][MAX_PINS_PER_CHANNEL] = {
  {PIN_LED0, PIN_LED1, PIN_LED2, END}, 
  {PIN_LED3, PIN_LED4, END}};

#define SWITCHES 5
static int switchChannelAnt[SWITCHES][2] = {
// ch, ant 
  {0, 0},
  {0, 1},
  {0, 2},
  {1, 0},
  {1, 1}
};

SoftwareSerial Serial1(PIN_SERIAL1_RX, PIN_SERIAL1_TX);

int brightness(bool enabled) {
  return enabled ? 0 : 245;
}

int receivedAnt[CHANNELS];
void setSelectedLed() {
  for (int ch = 0; ch < CHANNELS; ch++) {
    for (int pinIdx = 0; pinIdx < MAX_PINS_PER_CHANNEL; pinIdx++) {
      int pin = ledPins[ch][pinIdx];
      if (pin == END) {
        break;
      } else {
        analogWrite(pin, brightness(receivedAnt[ch] == pinIdx));
      }
    }
  }
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

  for (int i = 0; i < CHANNELS; i++) {
    receivedAnt[i] = -1;
  }
  setSelectedLed();
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
int pendingTransmitChannel;
void transmitAnt(int channel, int newAnt) {
  pendingTransmitAnt = newAnt;
  pendingTransmitChannel = channel;
  // will be picked up in next slave response
}

void processCommand(const char* cmd) {
  // AN0..3 -> channel 0
  // AN4..8 -> channel 1
  if (cmd[0] == 'A' && cmd[1] == 'N' && cmd[2] >= '0'&& cmd[2] <= '8' && cmd[3] == ';') {
    int newAnt = cmd[2] - '0';
    if (newAnt < 4) {
      transmitAnt(0, newAnt);
    } else {
      transmitAnt(1, newAnt - 4);
    }
    Serial.print("AN");
    Serial.print(newAnt);
    Serial.print(";");
  } else if (cmd[0] == 'A' && cmd[1] == 'N' && cmd[2] == ';') {
    Serial.print("AN");
    Serial.print(receivedAnt[0] >= 0 ? receivedAnt[0] : 0);
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
    int newReceivedAnt[CHANNELS] = {masterData & 0x03, masterData & 0x0c};
    for (int i = 0; i < CHANNELS; i++) {
      if (newReceivedAnt[i] != receivedAnt[i]) {
        receivedAnt[i] = newReceivedAnt[i];
        setSelectedLed();
      }
    }
    if (pendingTransmitAnt >= 0 && (masterData >> 4) == SLAVE_ID) {
      Serial1.write((uint8_t)((SLAVE_ID << 4) |  (uint8_t) (pendingTransmitChannel << 2) | (uint8_t) pendingTransmitAnt));
      pendingTransmitAnt = -1;
    }
    lastMasterMessage = millis();
  }

  if (millis() - lastMasterMessage > 1000) {
    setErrorLed();
  } else {
    int pressed = getPressed();
    if (pressed >= 0) {
      int txChannel = switchChannelAnt[pressed][0];
      int txAnt     = switchChannelAnt[pressed][1];
      if (txAnt != receivedAnt[txChannel]) {
        transmitAnt(txChannel, txAnt);
      }
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

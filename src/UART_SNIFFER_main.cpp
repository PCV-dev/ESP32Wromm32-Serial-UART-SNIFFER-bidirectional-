#include <Arduino.h>

#include <Arduino.h>

#define PC_BAUD      115200
#define TARGET_BAUD  19200

#define FRAME_GAP_US 3000
#define BUF_SIZE     256


// Richtung 1: Dashboard -> ESC
#define RX_DASH_TX   3 //  GPIO3 (RX0) Lauschen in Richtung ESC
#define TX_UNUSED_1  1 //  GPIO1 (TX0) Unused, da wir sniffen und nicht senden wollen

// Richtung 2: ESC -> Dashboard
#define RX_ESC_TX    16 // GPIO16 (RX2) Lauschen in Richtung Dashboard
#define TX_UNUSED_2  17 // GPIO17 (TX2) Unused, da wir sniffen und nicht senden wollen



HardwareSerial UartDash(1);
HardwareSerial UartEsc(2);

struct SniffBuffer {
  const char *name;
  HardwareSerial *uart;
  uint8_t buf[BUF_SIZE];
  uint16_t len;
  uint32_t frameStart;
  uint32_t lastByte;
};

SniffBuffer dashToEsc = {
  "DASH->ESC",
  &UartDash,
  {0},
  0,
  0,
  0
};

SniffBuffer escToDash = {
  "ESC->DASH",
  &UartEsc,
  {0},
  0,
  0,
  0
};

void printHexByte(uint8_t b) {
  if (b < 0x10) Serial.print("0");
  Serial.print(b, HEX);
}

void flushFrame(SniffBuffer &s) {
  if (s.len == 0) return;

  Serial.print(s.frameStart);
  Serial.print(" ");
  Serial.print(s.lastByte);
  Serial.print(" ");
  Serial.print(s.name);
  Serial.print(" LEN=");
  Serial.print(s.len);
  Serial.print(" DATA=");

  for (uint16_t i = 0; i < s.len; i++) {
    printHexByte(s.buf[i]);
    if (i + 1 < s.len) Serial.print(" ");
  }

  Serial.println();

  s.len = 0;
}

void readSniff(SniffBuffer &s) {
  while (s.uart->available()) {
    uint8_t b = s.uart->read();
    uint32_t now = micros();

    if (s.len == 0) {
      s.frameStart = now;
    }

    s.lastByte = now;

    if (s.len < BUF_SIZE) {
      s.buf[s.len++] = b;
    } else {
      flushFrame(s);
      s.frameStart = now;
      s.lastByte = now;
      s.buf[s.len++] = b;
    }
  }

  if (s.len > 0 && (uint32_t)(micros() - s.lastByte) > FRAME_GAP_US) {
    flushFrame(s);
  }
}

void setup() {
  Serial.begin(PC_BAUD);
  delay(500);

  UartDash.setRxBufferSize(1024);
  UartEsc.setRxBufferSize(1024);

  UartDash.begin(TARGET_BAUD, SERIAL_8N1, RX_DASH_TX, TX_UNUSED_1);
  UartEsc.begin(TARGET_BAUD, SERIAL_8N1, RX_ESC_TX, TX_UNUSED_2);

  Serial.println();
  Serial.println("ESP32 dual UART sniffer started");
  Serial.print("UART1 RX GPIO ");
  Serial.print(RX_DASH_TX);
  Serial.println(" = DASH->ESC");

  Serial.print("UART2 RX GPIO ");
  Serial.print(RX_ESC_TX);
  Serial.println(" = ESC->DASH");
}

void loop() {
  readSniff(dashToEsc);
  readSniff(escToDash);
}
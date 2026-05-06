#include <Arduino.h>


#define PC_BAUD      115200
#define TARGET_BAUD  19200

#define FRAME_GAP_US 5000
#define BUF_SIZE     256

// UART0 lassen wir in Ruhe ... für PC interface... immer! Es sei denn wir brauchen das interface nicht dann können wir es nach dem Start des ESP nutzen (wie auch immer)


// Richtung 1: Dashboard -> ESC
#define RDX1_RX 32 //  GPIO09 (UART1) müssen umgebogen werden weil Pin gleich mit SPI,  Lauschen in Richtung ESC

#define RDX1_DX 33 //  GPIO10 (UART1) (Unused da wir sniffen) müssen umgebogen werden weil Pin gleich mit SPI. In dem speziellen Def. Überflüssig dient der Vollständigkeit.


// Richtung 2: ESC -> Dashboard
#define RDX2_RX 16 // GPIO16 (UART2) Lauschen in Richtung Dashboard

#define RDX2_DX 17 // GPIO17 (UART2) Unused, da wir sniffen und nicht senden wollen. In dem speziellen Fall Def überflussig, dient der Vollständigkeit. 



HardwareSerial UartDash_ESC (1);
HardwareSerial UartESC_Dash (2);

struct SniffBuffer {
  const char *name;
  HardwareSerial *uart;
  uint8_t buf[BUF_SIZE];
  uint16_t len;
  uint32_t frameStart;
  uint32_t lastByte;
};

SniffBuffer UartDash_ESC_Buff = {
  "DASH->ESC",
  &UartDash_ESC,
  {0},
  0,
  0,
  0
};

SniffBuffer UartESC_Dash_Buff = {
  "ESC->DASH",
  &UartESC_Dash,
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

  UartDash_ESC.setRxBufferSize(1024);
  UartESC_Dash.setRxBufferSize(1024);

  UartDash_ESC.begin(TARGET_BAUD, SERIAL_8N1, RDX1_RX, -1 ); // -1 setzt TX auf inaktiv
  UartESC_Dash.begin(TARGET_BAUD, SERIAL_8N1, RDX2_RX, -1 ); // -1 setzt TX auf inaktiv

  Serial.println();
  Serial.println("ESP32 dual UART sniffer started");
  Serial.print("UART1 RX GPIO ");
  Serial.print(RDX1_RX);
  Serial.println(" = DASH->ESC");

  Serial.print("UART2 RX GPIO ");
  Serial.print(RDX2_RX);
  Serial.println(" = ESC->DASH");
}

void loop() {
  readSniff(UartDash_ESC_Buff);
  readSniff(UartESC_Dash_Buff);
}
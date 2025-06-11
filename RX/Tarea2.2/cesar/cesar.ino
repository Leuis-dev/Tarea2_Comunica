#include <VirtualWire.h>

#define WIDTH 32
#define HEIGHT 32
#define TOTAL_PIXELS (WIDTH * HEIGHT)
#define BITS_PER_PKT 8
#define TOTAL_PKTS (TOTAL_PIXELS / BITS_PER_PKT)

const int PIN_DATA = 2;
const int LED_RX = 8;
const uint8_t MY_RECEIVER_ID = 0x03;

const uint8_t POLY_CRC8 = 0x07;
const uint8_t INIT_CRC8 = 0x00;
#define CLAVE_CESAR 7

uint8_t dataBytes[TOTAL_PKTS];
bool receivedFlags[TOTAL_PKTS];
uint16_t uniqueCount = 0;
bool imagenCompleta = false;

uint8_t calcCRC8(const uint8_t *buf, uint8_t len) {
  uint8_t crc = INIT_CRC8;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= buf[i];
    for (uint8_t b = 0; b < 8; b++)
      crc = (crc & 0x80) ? ((crc << 1) ^ POLY_CRC8) & 0xFF : (crc << 1) & 0xFF;
  }
  return crc;
}

uint8_t descifrar(uint8_t data) {
  return (data - CLAVE_CESAR + 256) % 256;
}

void setup() {
  Serial.begin(9600);
  vw_set_rx_pin(PIN_DATA);
  vw_setup(500);
  vw_rx_start();
  pinMode(LED_RX, OUTPUT);
  digitalWrite(LED_RX, LOW);
  for (int i = 0; i < TOTAL_PKTS; i++) { receivedFlags[i] = false; dataBytes[i] = 0x00; }
  Serial.println("[RX] Modo: Cifrado César");
}

void loop() {
  if (imagenCompleta) while (true) delay(1000);

  uint8_t buf[6];
  uint8_t buflen = sizeof(buf);
  if (vw_get_message(buf, &buflen)) {
    if (buflen == 6 && buf[0] == 0xAA && buf[1] == 0x01 && buf[2] == MY_RECEIVER_ID) {
      if (calcCRC8(buf, 5) == buf[5]) {
        uint8_t seq = buf[3];
        uint8_t data = descifrar(buf[4]);
        if (!receivedFlags[seq]) {
          receivedFlags[seq] = true;
          dataBytes[seq] = data;
          uniqueCount++;

          Serial.print("[RX] Llegó seq = ");
          Serial.print(seq);
          Serial.print(" data = 0x");
          if (data < 0x10) Serial.print('0');
          Serial.println(data, HEX);

          Serial.print("[RX] Índices únicos recibidos: ");
          Serial.println(uniqueCount);

          digitalWrite(LED_RX, HIGH); delay(50); digitalWrite(LED_RX, LOW);

          if (uniqueCount == TOTAL_PKTS) {
            imagenCompleta = true;
            Serial.println("[RX] Imagen completa. Reconstruyendo...");
            reconstruirImagen();
          }
        }
      }
    }
  }
}

void reconstruirImagen() {
  Serial.println("----- Imagen 32x32 reconstruida -----");
  for (int i = 0; i < TOTAL_PKTS; i++) {
    for (int b = 7; b >= 0; b--) {
      Serial.print((dataBytes[i] & (1 << b)) ? '1' : '0');
    }
    if ((i + 1) % 4 == 0) Serial.println();
  }
  Serial.println("----- FIN -----");
}

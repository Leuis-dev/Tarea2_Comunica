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

// Claves comprobadas: e = 3, d = 86, n = 257
#define D_MULT 86
#define N_MULT 257

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

uint8_t descifrar_multiplicacion(uint8_t cifrado) {
  return (cifrado * D_MULT) % N_MULT;
}

void setup() {
  Serial.begin(9600);
  vw_set_rx_pin(PIN_DATA);
  vw_setup(500);
  vw_rx_start();
  pinMode(LED_RX, OUTPUT);
  digitalWrite(LED_RX, LOW);

  for (int i = 0; i < TOTAL_PKTS; i++) {
    receivedFlags[i] = false;
    dataBytes[i] = 0x00;
  }

  Serial.println("[RX] Modo: Cifrado multiplicativo activo");
}

void loop() {
  if (imagenCompleta) while (true) delay(1000);

  uint8_t buf[6];
  uint8_t buflen = sizeof(buf);
  if (vw_get_message(buf, &buflen)) {
    if (buflen == 6 && buf[0] == 0xAA && buf[1] == 0x01 && buf[2] == MY_RECEIVER_ID) {
      if (calcCRC8(buf, 5) == buf[5]) {
        uint8_t seq = buf[3];
        uint8_t cifrado = buf[4];
        uint8_t data = descifrar_multiplicacion(cifrado);

        Serial.print("[RX] seq = ");
        Serial.print(seq);
        Serial.print(" | Cifrado = ");
        Serial.print(cifrado);
        Serial.print(" | Descifrado = ");
        Serial.println(data);

        if (!receivedFlags[seq]) {
          receivedFlags[seq] = true;
          dataBytes[seq] = data;
          uniqueCount++;

          Serial.print("[TX-ACK] ");
          Serial.println(seq);

          digitalWrite(LED_RX, HIGH); delay(50); digitalWrite(LED_RX, LOW);

          if (uniqueCount == TOTAL_PKTS) {
            imagenCompleta = true;
            Serial.println("[RX] Imagen completa. Reconstruyendo...");
            reconstruirImagen();
          }
        }
      } else {
        Serial.println("[RX] CRC incorrecto. Paquete descartado.");
      }
    }
  }
}

void reconstruirImagen() {
  Serial.println("----- Imagen 32x32 reconstruida -----");
  for (int fila = HEIGHT - 1; fila >= 0; fila--) {  // Imprime desde la última fila hacia la primera
    for (int colByte = 0; colByte < WIDTH / 8; colByte++) {
      int i = fila * (WIDTH / 8) + colByte;
      for (int b = 7; b >= 0; b--) {
        bool bit = (dataBytes[i] & (1 << b));
        Serial.print(bit ? '0' : '1');  // Invertir color: 1 → 0, 0 → 1
      }
    }
    Serial.println();
  }
  Serial.println("----- FIN -----");
}
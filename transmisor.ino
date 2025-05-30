#include <VirtualWire.h>
#include <avr/pgmspace.h>
#include "imagen32x32.h"

// Protocolo
#define START_BYTE      0xAA
#define SENDER_ID       0x01
#define RECEIVER_ID     0x02

// Parámetros de imagen
const uint8_t WIDTH           = 32;                    // 32×32 píxeles
const uint16_t PIXELS         = uint16_t(WIDTH) * WIDTH;
const uint8_t  BITS_PER_P     = 24;                    // 24 píxeles/paquete
const uint8_t  BYTES_PER_P    = BITS_PER_P / 8;        // 3 bytes de imagen
const uint16_t PACKETS_TOTAL  = (PIXELS + BITS_PER_P - 1) / BITS_PER_P;

// Buffer plano: cada byte = 8 píxeles
uint8_t flatImg[(PIXELS + 7) / 8];

void flattenImage() {
  uint16_t bitIndex = 0;
  for (uint8_t row = 0; row < WIDTH; ++row) {
    for (uint8_t b = 0; b < WIDTH/8; ++b) {
      uint8_t val = pgm_read_byte(&(image_data[row][b]));
      for (int8_t bit = 7; bit >= 0; --bit) {
        uint8_t pix = (val >> bit) & 0x01;
        uint16_t bytePos = bitIndex / 8;
        uint8_t  bitPos  = 7 - (bitIndex % 8);
        flatImg[bytePos] |= (pix << bitPos);
        bitIndex++;
      }
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("Transmisor 32×32 (24b/paq) listo");

  vw_set_ptt_inverted(true);
  vw_setup(2000);
  vw_set_tx_pin(2);

  memset(flatImg, 0, sizeof(flatImg));
  flattenImage();
}

void loop() {
  static uint16_t seq = 0;
  uint8_t packet[1+1+1+1+BYTES_PER_P+1];
  uint8_t idx = 0;

  packet[idx++] = START_BYTE;
  packet[idx++] = SENDER_ID;
  packet[idx++] = RECEIVER_ID;
  packet[idx++] = seq;  // secuencia

  uint16_t base = seq * BYTES_PER_P;
  for (uint8_t j = 0; j < BYTES_PER_P; ++j) {
    packet[idx++] = flatImg[base + j];
  }

  uint16_t sum = 0;
  for (uint8_t i = 0; i < idx; ++i) sum += packet[i];
  packet[idx++] = sum & 0xFF;

  vw_send(packet, idx);
  vw_wait_tx();

  Serial.print("Enviado paquete ");
  Serial.println(seq);

  seq++;
  if (seq >= PACKETS_TOTAL) {
    seq = 0;
    Serial.println("**Frame completo**");
    delay(500);
  } else {
    delay(100);
  }
}

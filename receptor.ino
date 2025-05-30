#include <VirtualWire.h>

// Protocolo
#define START_BYTE      0xAA
#define SENDER_ID       0x01
#define RECEIVER_ID     0x02

const uint8_t WIDTH           = 32;
const uint16_t PIXELS         = uint16_t(WIDTH) * WIDTH;
const uint8_t  BITS_PER_P     = 24;
const uint8_t  BYTES_PER_P    = BITS_PER_P / 8;        // 3
const uint16_t PACKETS_TOTAL  = (PIXELS + BITS_PER_P - 1) / BITS_PER_P;

bool img[WIDTH][WIDTH];
uint16_t receivedPackets = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("Receptor 32×32 (24b/paq) listo");

  vw_set_ptt_inverted(true);
  vw_setup(2000);
  vw_set_rx_pin(2);
  vw_rx_start();
}

void loop() {
  uint8_t buf[VW_MAX_MESSAGE_LEN];
  uint8_t len = VW_MAX_MESSAGE_LEN;

  if (vw_get_message(buf, &len)) {
    if (len != (1+1+1+1+BYTES_PER_P+1)) return;
    if (buf[0] != START_BYTE ||
        buf[1] != SENDER_ID  ||
        buf[2] != RECEIVER_ID) return;

    uint8_t seq = buf[3];
    if (seq >= PACKETS_TOTAL) return;

    uint16_t sum = 0;
    for (uint8_t i = 0; i < len-1; ++i) sum += buf[i];
    if ((sum & 0xFF) != buf[len-1]) return;

    for (uint8_t bit = 0; bit < BITS_PER_P; ++bit) {
      uint16_t gbit = uint16_t(seq) * BITS_PER_P + bit;
      if (gbit >= PIXELS) break;
      uint16_t row = gbit / WIDTH;
      uint16_t col = gbit % WIDTH;
      uint8_t  bidx = 4 + (bit / 8);
      uint8_t  bpos = 7 - (bit % 8);
      img[row][col] = (buf[bidx] >> bpos) & 0x01;
    }

    receivedPackets++;
  }

  if (receivedPackets >= PACKETS_TOTAL) {
    Serial.println(F("\n===== IMAGEN RECIBIDA ====="));
    for (uint8_t r = 0; r < WIDTH; ++r) {
      for (uint8_t c = 0; c < WIDTH; ++c) {
        Serial.print(img[r][c] ? '#' : ' ');
      }
      Serial.println();
    }
    Serial.println(F("==========================="));
    receivedPackets = 0;
  }
}

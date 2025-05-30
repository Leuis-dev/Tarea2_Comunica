#include <VirtualWire.h>

// Protocolo
#define START_BYTE    0xAA
#define SENDER_ID     0x01
#define RECEIVER_ID   0x02

const int RX_PIN    = 2;
const int LED_PIN   = 8;
const uint8_t WIDTH = 32;
const uint8_t BYTES_PER_ROW = WIDTH / 8;

bool img[WIDTH][WIDTH];
uint8_t receivedLines = 0;

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("Receptor 32×32 listo");
  vw_set_ptt_inverted(true);
  vw_setup(2000);
  vw_set_rx_pin(RX_PIN);
  vw_rx_start();
}

void loop() {
  uint8_t buf[VW_MAX_MESSAGE_LEN];
  uint8_t len = VW_MAX_MESSAGE_LEN;

  if (vw_get_message(buf, &len)) {
    // Validar tamaño mínimo (1+1+1+1+4+1)
    if (len != (1+1+1+1+BYTES_PER_ROW+1)) return;
    // Cabeceras
    if (buf[0]!=START_BYTE || buf[1]!=SENDER_ID || buf[2]!=RECEIVER_ID) return;
    uint8_t lineIdx = buf[3];
    if (lineIdx >= WIDTH) return;
    // Checksum
    uint16_t sum = 0;
    for (uint8_t i = 0; i < len-1; i++) sum += buf[i];
    if ((sum & 0xFF) != buf[len-1]) return;

    // Parpadeo LED
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);

    // Decodificar datos en img[lineIdx]
    for (uint8_t b = 0; b < BYTES_PER_ROW; b++) {
      for (uint8_t bit = 0; bit < 8; bit++) {
        img[lineIdx][b*8 + bit] = buf[4 + b] & (1 << bit);
      }
    }
    receivedLines++;
  }

  // Cuando lleguen 32 líneas, imprimimos
  if (receivedLines == WIDTH) {
    Serial.println("\n===== IMAGEN RECIBIDA =====");
    for (uint8_t row = 0; row < WIDTH; row++) {
      for (uint8_t col = 0; col < WIDTH; col++) {
        Serial.print(img[row][col] ? '#' : ' ');
      }
      Serial.println();
    }
    Serial.println("===========================");
    receivedLines = 0;
  }
}

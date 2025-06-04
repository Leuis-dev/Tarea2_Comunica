// tx_433_image_crc8.ino
// ---------------------
// TX lee 5 bytes del Serial, calcula CRC-8, transmite 6 bytes por RF a 500 bps
// y luego responde con "[TX-ACK]seq" por el mismo Serial USB.
// Ahora queda en loop continuo esperando nuevos encabezados.
//
// Conexiones TX (Arduino):
//  - Módulo TX 433 MHz:
//      VCC → 5 V
//      GND → GND
//      DATA → D2
//      ANT → antena (~10 cm de cable)
//  - LED (opcional) → pin D13 (con resistencia a GND)

#include <VirtualWire.h>

#define HEADER_SIZE   5   // Python envía 5 bytes: [0xAA,0x01,0x03,seq,data]
#define PACKET_SIZE   6   // TX retransmitirá 6 bytes: [5 bytes + CRC]

const int PIN_DATA = 2;   // D2 para DATA del módulo 433 MHz
const int LED_TX   = 13;  // LED parpadea cada vez que transmite

// Parámetros CRC-8-CCITT (polinomio 0x07, valor inicial 0x00)
const uint8_t POLY_CRC8 = 0x07;
const uint8_t INIT_CRC8 = 0x00;

// Función para calcular CRC-8 sobre buf[0..len-1]
uint8_t calcCRC8(const uint8_t *buf, uint8_t len) {
  uint8_t crc = INIT_CRC8;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= buf[i];
    for (uint8_t b = 0; b < 8; b++) {
      if (crc & 0x80) {
        crc = ((crc << 1) ^ POLY_CRC8) & 0xFF;
      } else {
        crc = (crc << 1) & 0xFF;
      }
    }
  }
  return crc;
}

void setup() {
  Serial.begin(9600);           // 9600 bps para recibir encabezados de Python
  vw_set_ptt_inverted(true);
  vw_set_tx_pin(PIN_DATA);
  vw_setup(500);                // VirtualWire a 500 bps (RX debe usar igual)
  pinMode(LED_TX, OUTPUT);
  digitalWrite(LED_TX, LOW);
  Serial.println("[TX] Listo a 500 bps. Esperando encabezados en loop...");
}

void loop() {
  // 1) Esperar hasta que haya al menos 5 bytes disponibles en el buffer Serie
  if (Serial.available() >= HEADER_SIZE) {
    uint8_t buf5[HEADER_SIZE];
    // DEBUG: mostrar cuántos bytes hay en buffer Serie
    Serial.print("[TX DEBUG] Serial.available()= ");
    Serial.println(Serial.available());

    // 2) Leer exactamente 5 bytes: [0xAA, 0x01, 0x03, seq, data]
    Serial.readBytes(buf5, HEADER_SIZE);

    uint8_t seq = buf5[3];
    Serial.print("[TX] Recibí seq = ");
    Serial.println(seq);

    // 3) Calcular CRC-8 sobre esos 5 bytes
    uint8_t crc = calcCRC8(buf5, HEADER_SIZE);

    // 4) Armar paquete de 6 bytes: [5 bytes originales + CRC]
    uint8_t packet[PACKET_SIZE];
    memcpy(packet, buf5, HEADER_SIZE);
    packet[5] = crc;

    // 5) Transmitir los 6 bytes por RF
    digitalWrite(LED_TX, HIGH);
    vw_send(packet, PACKET_SIZE);
    vw_wait_tx();
    digitalWrite(LED_TX, LOW);

    // 6) Enviar ACK por Serial indicando el 'seq' procesado
    Serial.print("[TX-ACK]");
    Serial.println(seq);

    // 7) Esperar 50 ms antes de procesar el siguiente encabezado
    delay(50);
  }
}

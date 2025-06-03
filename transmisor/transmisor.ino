// tx_433_image_debug.ino
// ----------------------
// Este es tu TX (transmisor). Lee paquetes de 6 bytes desde el puerto Serial (USB)
// y los retransmite por el módulo 433 MHz usando VirtualWire. Además imprime por
// el Monitor Serie el campo `seq` (byte 3) de cada paquete, para que veas
// en qué orden está recibiendo los índices de Python.
//
// Conexiones para TX:
//  - Módulo TX 433 MHz:
//      VCC → 5V
//      GND → GND
//      DATA → D2
//      ANT → antena (~10 cm de cable)
//  - LED (opcional) integrado o externo → D13 (con resistencia a GND)

#include <VirtualWire.h>

#define PACKET_SIZE 6    // Cada paquete de Python mide 6 bytes: [AA, 01, 02, seq, data, checksum]

const int PIN_DATA = 2;    // DATA del transmisor 433 MHz conectado a D2 de Arduino
const int LED_TX   = 13;   // LED parpadea cada vez que transmite

void setup() {
  Serial.begin(9600);         // 9600 bps para debug en TX
  vw_set_ptt_inverted(true);  // Si tu módulo TX no transmite, prueba comentando esta línea
  vw_set_tx_pin(PIN_DATA);    // Configura VirtualWire para transmitir por D2
  vw_setup(2000);             // VirtualWire a 2000 bps (RX debe usar el mismo)
  pinMode(LED_TX, OUTPUT);
  digitalWrite(LED_TX, LOW);
  Serial.println("[TX] Listo. Esperando paquetes de Python...");
}

void loop() {
  if (Serial.available() >= PACKET_SIZE) {
    uint8_t packet[PACKET_SIZE];
    Serial.readBytes(packet, PACKET_SIZE);

    // Imprimimos el índice de secuencia (packet[3]) para verificar orden
    uint8_t seq = packet[3];
    Serial.print("[TX] Recibí seq = ");
    Serial.println(seq);

    // Transmitir los 6 bytes por RF
    digitalWrite(LED_TX, HIGH);
    vw_send((uint8_t*)packet, PACKET_SIZE);
    vw_wait_tx();
    digitalWrite(LED_TX, LOW);

    delay(5);  // breve retardo para no saturar el RF
  }
}

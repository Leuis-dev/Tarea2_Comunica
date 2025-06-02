// rx_433_debug_data.ino
// ---------------------
// Versión de RX que, además de seq, imprime en Monitor Serie el byte `data`
// (buf[4]) que llega en cada paquete válido. Así podrás ver si realmente
// buf[4] = 0x00 o tiene valores distintos.
//
// Conexiones para RX:
//  - Módulo RX 433 MHz:
//      VCC → 5V
//      GND → GND
//      DATA → D2
//      ANT → antena (~10 cm de cable)
//  - LED de notificación → Ánodo a D8, cátodo a GND (con resistencia ~220 Ω)

#include <VirtualWire.h>

#define WIDTH         32
#define HEIGHT        32
#define TOTAL_PIXELS  (WIDTH * HEIGHT)      // 1024 bits
#define BITS_PER_PKT  8                     // 1 byte = 8 píxeles
#define TOTAL_PKTS    (TOTAL_PIXELS / BITS_PER_PKT)  // 128 paquetes

const int PIN_DATA = 2;   // DATA del receptor 433 MHz conectado a D2
const int LED_RX   = 8;   // LED que parpadea al recibir cada paquete válido

uint8_t  dataBytes[TOTAL_PKTS];  // Buffer para almacenar el byte de datos de cada seq
uint16_t recibidos = 0;          // Contador de paquetes válidos recibidos

void setup() {
  Serial.begin(9600);         // 9600 bps para ver debug y la imagen
  Serial.println(F("[RX] Iniciando (modo debug de data)..."));
  vw_set_rx_pin(PIN_DATA);    // VirtualWire escucha en D2
  vw_setup(2000);             // VirtualWire a 2000 bps (debe coincidir con TX)
  vw_rx_start();              // Inicia la recepción por RF

  pinMode(LED_RX, OUTPUT);
  digitalWrite(LED_RX, LOW);
  Serial.println(F("[RX] Listo. Esperando 128 paquetes..."));
}

void loop() {
  uint8_t buf[6];
  uint8_t buflen = sizeof(buf);  // 6

  // Si VirtualWire dice que llegó un paquete de hasta 6 bytes...
  if (vw_get_message(buf, &buflen)) {
    if (buflen == 6) {
      // 1) Validar cabecera e IDs
      if (buf[0] == 0xAA && buf[1] == 0x01 && buf[2] == 0x02) {
        // 2) Calcular checksum = suma buf[0..4] & 0xFF
        uint8_t sum = 0;
        for (uint8_t i = 0; i < 5; i++) {
          sum += buf[i];
        }
        sum &= 0xFF;

        // 3) Comparar con buf[5]
        if (sum == buf[5]) {
          uint8_t seq  = buf[3];  // índice de paquete (0..127)
          uint8_t data = buf[4];  // byte de datos (8 bits de píxeles)

          // —— Aquí imprimimos seq y data para debug —— 
          Serial.print("[RX] Llegó seq = ");
          Serial.print(seq);
          Serial.print("   data = 0x");
          if (data < 0x10) Serial.print('0');
          Serial.println(data, HEX);
          // ————————————————————————————————————————

          // 4) Si está en rango válido, guardamos el byte
          if (seq < TOTAL_PKTS) {
            dataBytes[seq] = data;
            recibidos++;

            // 5) Parpadeo breve del LED_RX en pin 8
            digitalWrite(LED_RX, HIGH);
            delay(50);
            digitalWrite(LED_RX, LOW);

            // 6) Si llegamos a los 128 paquetes, reconstruimos la imagen
            if (recibidos == TOTAL_PKTS) {
              reconstruirYMostrarImagen();
              recibidos = 0;  // Reiniciar contador
            }
          }
        }
      }
    }
    buflen = sizeof(buf);  // Restaurar buflen antes de la próxima recepción
  }
}

void reconstruirYMostrarImagen() {
  static bool flatBits[TOTAL_PIXELS];

  // 1) Desempaquetar cada dataBytes[i] en 8 bits (MSB primero)
  for (uint16_t i = 0; i < TOTAL_PKTS; i++) {
    uint8_t bytePix = dataBytes[i];
    uint16_t base   = i * BITS_PER_PKT;  // i * 8

    for (uint8_t b = 0; b < BITS_PER_PKT; b++) {
      bool bit = (bytePix & (1 << (7 - b))) != 0;
      flatBits[base + b] = bit;
    }
  }

  // 2) Imprimir la matriz 32×32 en el Monitor Serie
  Serial.println(F("----- Imagen 32×32 recibida -----"));
  for (uint8_t fila = 0; fila < HEIGHT; fila++) {
    String linea = "";
    for (uint8_t col = 0; col < WIDTH; col++) {
      linea += (flatBits[fila * WIDTH + col] ? '1' : '0');
    }
    Serial.println(linea);
  }
  Serial.println(F("----- FIN de imagen -----"));
}

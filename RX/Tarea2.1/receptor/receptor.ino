// rx_433_image_crc8_freeze.ino
// -----------------------------
// Receptor 433 MHz que filtra por ID==0x03 y valida CRC-8.
// Cuando completa la imagen 32×32, “se congela” en un bucle infinito,
// ignorando cualquier paquete adicional.
//
// Conexiones RX (Arduino):
//  - Módulo RX 433 MHz:
//      VCC → 5 V
//      GND → GND
//      DATA → D2
//  - LED RX (opcional) → pin D8 (con resistencia a GND)

#include <VirtualWire.h>

#define WIDTH          32
#define HEIGHT         32
#define TOTAL_PIXELS   (WIDTH * HEIGHT)      // 1 024 bits
#define BITS_PER_PKT   8                     // 1 byte = 8 píxeles
#define TOTAL_PKTS     (TOTAL_PIXELS / BITS_PER_PKT)  // 128 paquetes

const int PIN_DATA           = 2;    // DATA del módulo 433 MHz en D2
const int LED_RX             = 8;    // LED parpadea al recibir paquete válido
const uint8_t MY_RECEIVER_ID = 0x03; // Sólo acepta paquetes cuyo byte[2] == 0x03

// Parámetros CRC-8-CCITT (polinomio = 0x07, valor inicial = 0x00)
const uint8_t POLY_CRC8      = 0x07;
const uint8_t INIT_CRC8      = 0x00;

uint8_t  dataBytes[TOTAL_PKTS];      // Almacena el byte de datos de cada seq
bool     receivedFlags[TOTAL_PKTS];  // True si ya recibimos el índice i (0..127)
uint16_t uniqueCount = 0;            // Cuántos índices únicos hemos guardado
bool     imagenCompleta = false;     // True cuando lleguen los 128 paquetes válidos

// --------------------------------------------------------------------------------
// Calcula CRC-8 sobre buf[0..len-1], usando CRC-8-CCITT (polinomio 0x07, init 0x00)
// --------------------------------------------------------------------------------
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
  Serial.begin(9600);
  Serial.println(F("[RX] Iniciando (modo CRC8 + filtro ID) a 500 bps…"));

  vw_set_rx_pin(PIN_DATA);
  vw_setup(500);   // Debe coincidir con el TX: VirtualWire a 500 bps
  vw_rx_start();

  pinMode(LED_RX, OUTPUT);
  digitalWrite(LED_RX, LOW);

  for (uint16_t i = 0; i < TOTAL_PKTS; i++) {
    receivedFlags[i] = false;
    dataBytes[i]     = 0x00;
  }
  uniqueCount    = 0;
  imagenCompleta = false;

  Serial.print(F("[RX] Filtrando ID = 0x"));
  if (MY_RECEIVER_ID < 0x10) Serial.print('0');
  Serial.println(MY_RECEIVER_ID, HEX);
}

void loop() {
  uint8_t buf[6];
  uint8_t buflen = sizeof(buf);  // = 6

  // Si ya completamos la imagen, “congelamos” el receptor aquí:
  if (imagenCompleta) {
    // Entramos en un bucle infinito donde no procesamos nada más.
    // El LED queda apagado y no respondemos a más paquetes.
    while (true) {
      delay(1000);
    }
  }

  // 1) Si VirtualWire indica que llegó un paquete de hasta 6 bytes…
  if (vw_get_message(buf, &buflen)) {
    if (buflen == 6) {
      // 1.1) Filtrar por cabecera (0xAA, 0x01) y por ID de este receptor
      if (buf[0] == 0xAA && buf[1] == 0x01 && buf[2] == MY_RECEIVER_ID) {
        // 1.2) Calcular CRC-8 sobre buf[0..4] y comparar con buf[5]
        uint8_t crc_calc = calcCRC8(buf, 5);
        if (crc_calc == buf[5]) {
          uint8_t seq  = buf[3];  // Índice de paquete (0..127)
          uint8_t data = buf[4];  // Byte de datos (8 bits de la imagen)

          // Muestra debug en Monitor Serie
          Serial.print(F("[RX] Llegó seq = "));
          Serial.print(seq);
          Serial.print(F("   data = 0x"));
          if (data < 0x10) Serial.print('0');
          Serial.println(data, HEX);

          // 2) Si este índice aún no lo habíamos guardado:
          if (seq < TOTAL_PKTS && !receivedFlags[seq]) {
            receivedFlags[seq] = true;
            dataBytes[seq]     = data;
            uniqueCount++;

            // Parpadea el LED_RX brevemente para indicar que el paquete es válido
            digitalWrite(LED_RX, HIGH);
            delay(50);
            digitalWrite(LED_RX, LOW);

            Serial.print(F("[RX] Índices únicos recibidos: "));
            Serial.println(uniqueCount);

            // 2.1) Si recibimos seq=127 pero no tenemos 128 índices, mostramos faltantes
            if (seq == (TOTAL_PKTS - 1) && uniqueCount < TOTAL_PKTS) {
              Serial.print(F("[ADVERTENCIA] Llegó seq=127 pero solo "));
              Serial.print(uniqueCount);
              Serial.println(F(" únicos. Faltan algunos."));
              mostrarIndicesFaltantes();
            }

            // 2.2) Si completamos todos 128 índices, reconstruimos la imagen
            if (uniqueCount == TOTAL_PKTS) {
              imagenCompleta = true;
              Serial.println(F("[RX] ¡128 índices únicos recibidos! Reconstruyendo la imagen…"));
              reconstruirYMostrarImagen();
              Serial.println(F("[RX] Imagen reconstruida. REINICIA el Arduino para nueva ronda."));
              // Tras esto, loop() verá imagenCompleta == true y “se congelará”
            }
          }
          // 3) Si ese seq ya estaba marcado y todavía no reconstruimos:
          else if (seq < TOTAL_PKTS && receivedFlags[seq] && !imagenCompleta) {
            Serial.print(F("[RX] Paquete duplicado seq = "));
            Serial.println(seq);
          }
        }
        else {
          // CRC no coincide: descartamos
          Serial.print(F("[RX] CRC inválido para seq = "));
          Serial.println(buf[3]);
        }
      }
      else {
        // Cabecera o ID no coinciden
        if (buf[0] != 0xAA || buf[1] != 0x01) {
          Serial.println(F("[RX] Cabecera/Sender errónea → descartar."));
        } else {
          Serial.print(F("[RX] Paquete con ID = 0x"));
          if (buf[2] < 0x10) Serial.print('0');
          Serial.print(buf[2], HEX);
          Serial.println(F(" no corresponde a este receptor → descartar."));
        }
      }
    }
    // Restaurar buflen antes de la próxima lectura
    buflen = sizeof(buf);
  }
}

// -----------------------------------------------
// Imprime la lista de índices faltantes (para debug)
// -----------------------------------------------
void mostrarIndicesFaltantes() {
  Serial.print(F("[INFO] Paquetes faltantes: "));
  bool algunoFalta = false;
  for (uint16_t i = 0; i < TOTAL_PKTS; i++) {
    if (!receivedFlags[i]) {
      Serial.print(i);
      Serial.print(F(" "));
      algunoFalta = true;
    }
  }
  if (!algunoFalta) {
    Serial.print(F("ninguno"));
  }
  Serial.println();
}

// -----------------------------------------------------
// Desempaqueta cada byte en 8 bits y los imprime 32×32
// -----------------------------------------------------
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
  Serial.println(F("----- Imagen 32x32 reconstruida -----"));
  for (uint8_t fila = 0; fila < HEIGHT; fila++) {
    String linea = "";
    for (uint8_t col = 0; col < WIDTH; col++) {
      linea += (flatBits[fila * WIDTH + col] ? '1' : '0');
    }
    Serial.println(linea);
  }
  Serial.println(F("----- FIN -----"));
  // A partir de aquí, imagenCompleta = true, así que loop() “se congela”
}

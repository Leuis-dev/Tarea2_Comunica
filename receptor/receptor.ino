#include <VirtualWire.h>

#define WIDTH         32
#define HEIGHT        32
#define TOTAL_PIXELS  (WIDTH * HEIGHT)      // 1024 bits
#define BITS_PER_PKT  8                     // 1 byte = 8 píxeles
#define TOTAL_PKTS    (TOTAL_PIXELS / BITS_PER_PKT)  // 128 paquetes

const int PIN_DATA = 2;   // DATA del receptor 433 MHz conectado a D2
const int LED_RX   = 8;   // LED que parpadea al recibir cada paquete válido

uint8_t  dataBytes[TOTAL_PKTS];      // Buffer para almacenar el byte de datos de cada seq
bool     receivedFlags[TOTAL_PKTS];  // Banderas: si recibimos el índice i (0..127)
uint16_t uniqueCount = 0;            // Cuántos índices únicos hemos almacenado
bool imagenCompleta = false;         // Bandera para saber si ya se completó la matriz

void setup() {
  Serial.begin(9600);         // 9600 bps para debug y la imagen
  Serial.println(F("[RX] Iniciando (modo debug con flags)…"));

  vw_set_rx_pin(PIN_DATA);    // VirtualWire escucha en D2
  vw_setup(2000);             // VirtualWire a 2000 bps (debe coincidir con TX)
  vw_rx_start();              // Inicia la recepción por RF

  pinMode(LED_RX, OUTPUT);
  digitalWrite(LED_RX, LOW);

  // Inicializar flags y contador
  for (uint16_t i = 0; i < TOTAL_PKTS; i++) {
    receivedFlags[i] = false;
    dataBytes[i] = 0x00;      // opcional: limpiar buffer
  }
  uniqueCount = 0;
  imagenCompleta = false;

  Serial.println(F("[RX] Listo. Esperando hasta 128 índices únicos..."));
}

void loop() {
  uint8_t buf[6];
  uint8_t buflen = sizeof(buf);  // 6 bytes

  // Si la imagen ya está completa, ignoramos paquetes nuevos (sin procesar)
  if (imagenCompleta) {
    // Opcional: parpadeo rápido para indicar que llegan paquetes, pero no se procesan
    if (vw_have_message()) {
      // Limpiar mensaje sin procesar
      vw_get_message(buf, &buflen);
      digitalWrite(LED_RX, HIGH);
      delay(10);
      digitalWrite(LED_RX, LOW);
    }
    return;
  }

  // 1) Si VirtualWire dice que llegó un paquete de hasta 6 bytes...
  if (vw_get_message(buf, &buflen)) {
    if (buflen == 6) {
      // 1.1) Validar cabecera e IDs
      if (buf[0] == 0xAA && buf[1] == 0x01 && buf[2] == 0x02) {
        // 1.2) Calcular checksum = suma buf[0..4] & 0xFF
        uint8_t sum = 0;
        for (uint8_t i = 0; i < 5; i++) {
          sum += buf[i];
        }
        sum &= 0xFF;

        // 1.3) Comparar con buf[5]
        if (sum == buf[5]) {
          uint8_t seq  = buf[3];  // índice de paquete (0..127)
          uint8_t data = buf[4];  // byte de datos (8 bits de píxeles)

          // —— Imprimir seq y data para debug —— 
          Serial.print(F("[RX] Llegó seq = "));
          Serial.print(seq);
          Serial.print(F("   data = 0x"));
          if (data < 0x10) Serial.print('0');
          Serial.println(data, HEX);
          // ————————————————————————————————————————

          // 2) Si este índice aún no se había almacenado:
          if (seq < TOTAL_PKTS && !receivedFlags[seq]) {
            receivedFlags[seq] = true;    // Marcar como recibido
            dataBytes[seq]     = data;    // Guardar el byte
            uniqueCount++;                // Incrementar conteo único

            // Parpadeo breve del LED_RX en pin 8
            digitalWrite(LED_RX, HIGH);
            delay(50);
            digitalWrite(LED_RX, LOW);

            // 2.1) Mostrar cuántos índices únicos llevamos:
            Serial.print(F("[RX] Índices únicos recibidos: "));
            Serial.println(uniqueCount);

            // 2.2) Si acabamos de recibir seq=127 pero aún no completamos 128:
            if (seq == (TOTAL_PKTS - 1) && uniqueCount < TOTAL_PKTS) {
              Serial.print(F("[ADVERTENCIA] Llegó seq=127 pero sólo "));
              Serial.print(uniqueCount);
              Serial.println(F(" índices únicos. Faltan algunos."));
              mostrarIndicesFaltantes();
            }

            // 2.3) Si completamos todos 128 índices únicos:
            if (uniqueCount == TOTAL_PKTS) {
              imagenCompleta = true;
              Serial.println(F("[RX] ¡128 índices únicos recibidos! Reconstruyendo la imagen..."));
              reconstruirYMostrarImagen();
              Serial.println(F("[RX] Imagen reconstruida. Ignorando más paquetes para esta imagen."));
            }

          }
          // 3) Si seq ya estaba marcado y aún no reconstruimos:
          else if (seq < TOTAL_PKTS && receivedFlags[seq] && !imagenCompleta) {
            // Es un paquete duplicado: imprimimos aviso y lo ignoramos
            Serial.print(F("[RX] Paquete duplicado seq = "));
            Serial.println(seq);
          }
          // 4) Si imagenCompleta == true, ya no hacemos nada con paquetes nuevos
        }
      }
    }
    buflen = sizeof(buf);  // Restaurar buflen antes de la próxima recepción
  }
}

// Función para resetear la recepción (puedes llamarla para recibir una nueva imagen)
void resetReception() {
  for (uint16_t i = 0; i < TOTAL_PKTS; i++) {
    receivedFlags[i] = false;
    dataBytes[i] = 0x00;
  }
  uniqueCount = 0;
  imagenCompleta = false;
  Serial.println(F("[RX] Listo para recibir nueva imagen..."));
  vw_rx_start();  // Asegurarse que la recepción esté activa
}

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

// Desempaqueta dataBytes[] en bits y los imprime como matriz 32×32
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

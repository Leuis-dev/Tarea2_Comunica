# Comunica el arduino del arduino

Desarrollo de un Sistema de Comunicación inalámbrica con Arduino

La comunicación inalámbrica es en la actualidad una de las áreas más activas de investigación 
e innovación, jugando día a día  un rol preponderante en el desarrollo de nuevos dispositivos, 
mecanismos de intercambio de información y la generación de novedosas aplicaciones 
telemáticas, que utilizan este medio para trasmitir datos. 
Por otro lado, la utilización de diferentes medios de comunicación para la transmisión de 
datos trae consigo la existencia de una alta probabilidad de error en el proceso de transmisión 
de los mismos. En el caso de la comunicación inalámbrica el efecto de la relación señal-ruido  
y la degradación de la señal en el espacio,  permiten que muchos de los bit de los paquetes 
enviados se vean modificados en el trayecto producto de las condiciones del entorno, 
generando con esto, errores  en la transmisión de los  datos.




Transmisión de Imagen 32×32 vía Arduino 433 MHz
Este repositorio contiene todo lo necesario para enviar una imagen binarizada de 32 × 32 píxeles desde un PC a un Arduino receptor usando módulos RF de 433 MHz. El flujo completo es:

Python (ima.py): procesa la imagen, la convierte a paquetes de 6 bytes y los envía por USB al Arduino TX.

Arduino TX (transmisor.ino): recibe esos paquetes por Serial, imprime el índice de secuencia y los retransmite por RF.

Arduino RX (receptor.ino): recibe cada paquete de RF, valida cabecera/IDs/checksum, imprime el índice y el byte de datos, parpadea un LED y, al completar 128 paquetes, reconstruye e imprime la matriz 32 × 32.

Python (reconstruir.py): a partir de las 32 líneas de “0”/“1” impresas en el RX, genera y muestra la imagen reconstruida (32 × 32).(FALTA HACER)

Índice
Requisitos de Hardware

Requisitos de Software

Conexiones de Hardware

Estructura de un paquete

Cargar Sketches en Arduino

Sketch TX: tx_433_image_debug.ino

Sketch RX: rx_433_debug_data.ino

Enviar la Imagen desde Python

ima.py

Reconstruir la Imagen desde la Matriz

Copiar la matriz del Monitor Serie de RX

reconstruir.py

Flujo de Ejecución Completo

Solución de Problemas Comunes

Licencia

Requisitos de Hardware
2× Arduino UNO (o equivalente)

Uno como TX (emisor), conectado al PC por USB.

Otro como RX (receptor).

2× Módulo 433 MHz

Módulo transmisor (con antena)

Módulo receptor (con antena)

LED + resistencia 220 Ω

Indicador de envío en TX (pin 13) (opcional).

Indicador de recepción en RX (pin 8).

Cables Dupont / Breadboard.

Requisitos de Software
Python 3.x con librerías:

nginx
Copiar
Editar
pip install pillow numpy pyserial matplotlib
IDE de Arduino (1.8.x o superior) con la librería VirtualWire instalada:

Abrir “Sketch → Include Library → Manage Libraries…”.

Buscar “VirtualWire” e instalar la versión compatible.

Conexiones de Hardware
Arduino TX (Emisor)
Módulo TX 433 MHz

VCC → 5 V

GND → GND

DATA → D2 (PIN_DATA = 2 en el sketch)

ANT → Antena (≈ 10 cm de cable)

(Opcional) LED

Ánodo → D13

Cátodo → Resistencia 220 Ω → GND

Arduino RX (Receptor)
Módulo RX 433 MHz

VCC → 5 V

GND → GND

DATA → D2 (PIN_DATA = 2 en el sketch)

ANT → Antena (≈ 10 cm de cable)

LED de notificación

Ánodo → D8

Cátodo → Resistencia 220 Ω → GND

Estructura de un paquete
Cada paquete enviado consta de 6 bytes, organizados así:

Byte	Nombre	Valor o Descripción
0	START	0xAA (constante para identificar inicio de paquete)
1	SENDER_ID	0x01 (identificador del emisor)
2	RECEIVER_ID	0x02 (identificador del receptor)
3	SEQ	Número de secuencia 0…127 (128 paquetes en total, uno por cada 8 píxeles)
4	DATA	1 byte = 8 bits de imagen (cada bit = un píxel B/N, MSB primero).
5	CHECKSUM	Suma de los 5 bytes anteriores ((Byte0+Byte1+Byte2+Byte3+Byte4)&0xFF).

Byte 3 (SEQ):
Valor entero de 0 a 127. Indica la posición del byte de datos dentro de los 1 024 píxeles totales (32×32).

SEQ=0 → cubre píxeles 0…7

SEQ=1 → cubre píxeles 8…15

…

SEQ=127 → cubre píxeles 1 016…1 023

Byte 4 (DATA):
1 byte cuyas 8 bits (b7…b0) representan 8 píxeles.

Bit 7 (MSB) → píxel (SEQ*8+0)

Bit 6 → píxel (SEQ*8+1)

…

Bit 0 (LSB) → píxel (SEQ*8+7)

Ejemplo: si SEQ=3 y DATA=0b10110001 (0xB1):

bit 7 = 1 → píxel 24 blanco

bit 6 = 0 → píxel 25 negro

bit 5 = 1 → píxel 26 blanco

bit 4 = 1 → píxel 27 blanco

bit 3 = 0 → píxel 28 negro

bit 2 = 0 → píxel 29 negro

bit 1 = 0 → píxel 30 negro

bit 0 = 1 → píxel 31 blanco

Byte 5 (CHECKSUM):

kotlin
Copiar
Editar
CHECKSUM = (Byte 0 + Byte 1 + Byte 2 + Byte 3 + Byte 4) & 0xFF
Total paquetes:
Cada imagen de 32×32 = 1 024 píxeles / 8 píxeles por paquete = 128 paquetes (SEQ 0…127).

Cargar Sketches en Arduino
5.1. Sketch TX: tx_433_image_debug.ino
arduino
#include <VirtualWire.h>
#define PACKET_SIZE 6
const int PIN_DATA = 2;    // DATA → D2
const int LED_TX   = 13;   // LED opcional en D13
void setup() {
  Serial.begin(9600);
  vw_set_ptt_inverted(true);
  vw_set_tx_pin(PIN_DATA);
  vw_setup(2000);
  pinMode(LED_TX, OUTPUT);
  digitalWrite(LED_TX, LOW);
  Serial.println("[TX] Listo. Esperando paquetes de Python...");
}

void loop() {
  if (Serial.available() >= PACKET_SIZE) {
    uint8_t packet[PACKET_SIZE];
    Serial.readBytes(packet, PACKET_SIZE);

    uint8_t seq = packet[3];
    Serial.print("[TX] Recibí seq = ");
    Serial.println(seq);

    digitalWrite(LED_TX, HIGH);
    vw_send((uint8_t*)packet, PACKET_SIZE);
    vw_wait_tx();
    digitalWrite(LED_TX, LOW);
    delay(5);
  }
}


Abre este archivo en el IDE de Arduino.

Selecciona el Arduino TX (puerto COM correcto).

Carga el sketch.

Cierra el Monitor Serie de este Arduino (para liberar el COM).


5.2. Sketch RX: rx_433_debug_data.ino
arduino
#include <VirtualWire.h>

#define WIDTH        32
#define HEIGHT       32
#define TOTAL_PIXELS (WIDTH * HEIGHT)      // 1 024
#define BITS_PER_PKT 8
#define TOTAL_PKTS   (TOTAL_PIXELS / BITS_PER_PKT)  // 128

const int PIN_DATA = 2;    // DATA → D2
const int LED_RX   = 8;    // LED en D8

uint8_t  dataBytes[TOTAL_PKTS];
uint16_t recibidos = 0;

void setup() {
  Serial.begin(9600);
  Serial.println(F("[RX] Iniciando (modo debug de data)..."));
  vw_set_rx_pin(PIN_DATA);
  vw_setup(2000);
  vw_rx_start();
  pinMode(LED_RX, OUTPUT);
  digitalWrite(LED_RX, LOW);
  Serial.println(F("[RX] Listo. Esperando 128 paquetes..."));
}

void loop() {
  uint8_t buf[6];
  uint8_t buflen = sizeof(buf);

  if (vw_get_message(buf, &buflen)) {
    if (buflen == 6) {
      if (buf[0] == 0xAA && buf[1] == 0x01 && buf[2] == 0x02) {
        uint8_t sum = 0;
        for (uint8_t i = 0; i < 5; i++) sum += buf[i];
        sum &= 0xFF;
        if (sum == buf[5]) {
          uint8_t seq  = buf[3];
          uint8_t data = buf[4];

          Serial.print("[RX] Llegó seq = ");
          Serial.print(seq);
          Serial.print("   data = 0x");
          if (data < 0x10) Serial.print('0');
          Serial.println(data, HEX);

          if (seq < TOTAL_PKTS) {
            dataBytes[seq] = data;
            recibidos++;
            digitalWrite(LED_RX, HIGH);
            delay(50);
            digitalWrite(LED_RX, LOW);
            if (recibidos == TOTAL_PKTS) {
              reconstruirYMostrarImagen();
              recibidos = 0;
            }
          }
        }
      }
    }
    buflen = sizeof(buf);
  }
}

void reconstruirYMostrarImagen() {
  static bool flatBits[TOTAL_PIXELS];
  for (uint16_t i = 0; i < TOTAL_PKTS; i++) {
    uint8_t bytePix = dataBytes[i];
    uint16_t base   = i * BITS_PER_PKT;
    for (uint8_t b = 0; b < BITS_PER_PKT; b++) {
      bool bit = (bytePix & (1 << (7 - b))) != 0;
      flatBits[base + b] = bit;
    }
  }

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
Abre este archivo en el IDE de Arduino.

Selecciona el Arduino RX (puerto COM correcto).

Carga el sketch.

Abre el Monitor Serie a 9600 bps.


6. Enviar la Imagen desde Python
6.1. ima.py
python
from PIL import Image
import numpy as np
import serial
import time

# Parámetros de la imagen y protocolo
INPUT_PATH   = r'C:/Users/sebas/OneDrive/Escritorio/archgithub/cofre.png'
WIDTH, HEIGHT = 32, 32
PIXELS        = WIDTH * HEIGHT    # 1 024
BITS_PER_PKT  = 8
TOTAL_PKTS    = PIXELS // BITS_PER_PKT  # 128

START_BYTE   = 0xAA
SENDER_ID    = 0x01
RECEIVER_ID  = 0x02

# Paso 1: Cargar y redimensionar
img = Image.open(INPUT_PATH).resize((WIDTH, HEIGHT)).convert('L')
arr = np.array(img)

# Paso 2: Binarizar (>128)
binary_arr = (arr > 128).astype(np.uint8)

print("[Python DEBUG] Primeros 8 bits binarios:", binary_arr.flatten()[:8])

# Paso 3: Empacar 8 bits → 1 byte
packed_bytes = []
for i in range(0, PIXELS, BITS_PER_PKT):
    byte = 0
    for b in range(BITS_PER_PKT):
        byte = (byte << 1) | int(binary_arr.flatten()[i + b])
    packed_bytes.append(byte)

print("[Python DEBUG] Primeros 4 bytes empacados:", [f"{b:02X}" for b in packed_bytes[:4]])

# Paso 4: Construir 128 paquetes de 6 bytes
packets = []
for seq in range(TOTAL_PKTS):
    data = packed_bytes[seq]
    packet = [START_BYTE, SENDER_ID, RECEIVER_ID, seq, data]
    checksum = sum(packet) & 0xFF
    packet.append(checksum)
    packets.append(packet)

print(f"[Python] Total de paquetes a enviar: {len(packets)}")




# Paso 5: Abrir Serial y enviar
PUERTO    = 'COM6'   # Cambiar por tu puerto TX
BAUD_RATE = 9600

ser = serial.Serial(PUERTO, BAUD_RATE, timeout=1)
time.sleep(2.5)
ser.reset_input_buffer()
ser.reset_output_buffer()

for idx, p in enumerate(packets):
    ser.write(bytes(p))
    ser.flush()
    print(f"[Python] Enviado paquete {idx:03d}/127  Bytes = {' '.join(f'{b:02X}' for b in p)}")
    time.sleep(0.10)

print("[Python] Todos los paquetes enviados. Cerrando Serial.")
ser.close()
Edita INPUT_PATH para apuntar a tu imagen.

Ajusta PUERTO al COM del Arduino TX.

Asegúrate de que el Monitor Serie de TX esté cerrado.

En consola, ejecuta:
nginx
python ima.py

7. Reconstruir la Imagen desde la Matriz
7.1. Copiar la Matriz desde Arduino RX
En el Monitor Serie de RX, al final verás:

diff
Copiar
Editar
----- Imagen 32×32 recibida -----
11111111111111111111111111111111
11111111111111000011111111111111
...
11111111111111111111111111111111
----- FIN de imagen -----
Copia esas 32 líneas (solo los caracteres “0”/“1”).

7.2. reconstruir.py
python
Copiar
Editar
import numpy as np
import matplotlib.pyplot as plt
from PIL import Image

# Pega aquí las 32 líneas copiadas del Monitor Serie de RX:
matrix_lines = [
    "11111111111111111111111111111111",
    "11111111111111000011111111111111",
    "11111111111100111100111111111111",
    "11111111110011111111001111111111",
    "11111111001111111111110011111111",
    "11111100111111111111111100111111",
    "11110011111111111111111111001111",
    "11001111111111111111111111110011",
    "11000011111111111111111111000011",
    "11010000111111111111111100000011",
    "11011100001111111111110000000011",
    "11011111000011111111000000000011",
    "11011111110000111100000000000011",
    "11000111011100000000000000000011",
    "11000011101111000000000000000011",
    "11011001110111100000000000000011",
    "11011111100111100000000000000011",
    "11011111100011100000000000000011",
    "11011111100000000000000000000011",
    "11011111100110000000000000000011",
    "11011111111111100000000000000011",
    "11011111111111100000000000000011",
    "11011111111111100000000000000011",
    "11011111111111100000000000000011",
    "11000111111111100000000000000011",
    "11110011111111100000000000001111",
    "11111100011111100000000000111111",
    "11111111001111100000000011111111",
    "11111111110011100000001111111111",
    "11111111111100000000111111111111",
    "11111111111111000011111111111111",
    "11111111111111111111111111111111"
]

# Convertir a array NumPy de 0/1
matrix = np.array([[int(bit) for bit in line] for line in matrix_lines])

# Mostrar como imagen blanco/negro
plt.figure(figsize=(4, 4))
plt.imshow(matrix, cmap='gray_r', vmin=0, vmax=1)
plt.axis('off')
plt.title("Imagen Reconstruida 32×32")
plt.show()

# (Opcional) Guardar a PNG
img_array = (1 - matrix) * 255  # Invertir: 1→0(negro), 0→255(blanco)
img = Image.fromarray(img_array.astype(np.uint8), mode='L')
img.save("reconstruida_32x32.png")
Guarda este archivo como reconstruir.py.

Pega las 32 líneas en matrix_lines.

Ejecuta:

nginx
Copiar
Editar
python reconstruir.py
Verás la imagen reconstruida 32×32. Se guardará también reconstruida_32x32.png.

8. Flujo de Ejecución Completo
Cargar TX

Sketch: tx_433_image_debug.ino

Arduino TX (cierre del Monitor Serie)

Cargar RX

Sketch: rx_433_debug_data.ino

Arduino RX (abrir Monitor Serie a 9600 bps)

Editar ima.py:

INPUT_PATH → ruta de la imagen.

PUERTO → COM del Arduino TX.

Ejecutar

nginx
Copiar
Editar
python ima.py
TX recibe e imprime “seq = …” en orden 0→127.

RX imprime “seq = … data = 0x…” y parpadea LED.

Tras 128 paquetes, RX muestra 32 líneas de “0”/“1”.

Copiar esas 32 líneas → pegar en reconstruir.py → ejecutar:

nginx
Copiar
Editar
python reconstruir.py
Imagen reconstruida 32×32 en pantalla y en reconstruida_32x32.png.

9. Solución de Problemas Comunes
TX imprime seq fuera de orden o no empieza en 0

Cerrar Monitores Serie que usen el mismo COM.

Pulsar RESET en Arduino TX antes de ejecutar ima.py.

Verificar que Python use ser.reset_input_buffer() y ser.reset_output_buffer().

Aumentar time.sleep(2.5) en Python si TX tarda en inicializar VirtualWire.

RX recibe data = 0x00 en casi todos los paquetes

Confirmar que Python binariza tras redimensionar (convert('L') + umbral > 128).

Revisar los mensajes de debug en Python ([Python DEBUG]) para asegurarse de que los bytes empacados no sean cero.

Ajustar umbral si la imagen es muy oscura o muy clara.

RX no parpadea con “PING” ni recibe paquetes

Comprobar alimentación y cableado de módulos RF (VCC/GND/ANT).

Asegurar antenas (≈ 10 cm de alambre) en TX y RX.

Verificar mismo bitrate: vw_setup(2000) en ambos sketches (o bajar a 1000 bps si hay interferencia).

Reducir distancia inicial (< 50 cm) para depuración.

Matriz final alterada o incompleta

Aumentar retardo en Python (time.sleep(0.15)–0.20).

En RX, añadir delay(5) antes de cada Serial.println(linea) en reconstruirYMostrarImagen().

Confirmar lógica de desempaquetado:

arduino
Copiar
Editar
bool bit = (bytePix & (1 << (7 - b))) != 0;
10. Licencia
Este proyecto está bajo Licencia MIT:

sql
Copiar
Editar
MIT License

Copyright (c) 2025

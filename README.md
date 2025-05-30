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


## Proyecto: Envío y Recepción de Imagen 32×32 B/N

Este proyecto incluye cuatro componentes:

1. **transmisor.ino** (Arduino)  – envía la imagen empacada en paquetes de 24 bits.
2. **receptor.ino** (Arduino)   – recibe los paquetes y reconstruye la imagen en consola Serial.
3. **llegada.py** (Python)     – recibe paquetes por serie y muestra la imagen con Matplotlib.
4. **imagen2bits.py** (Python) – convierte una imagen PNG B/N de 32×32 en paquetes listos para envío.

---

### Requisitos

* **Hardware**: 2 placas Arduino Uno (o compatibles) con módulo de radio VirtualWire, conexiones TX/RX.
* **Software Arduino IDE** (v1.8.x o superior) con la librería VirtualWire instalada.
* **Python 3.7+** con las librerías:

  * `pyserial`
  * `numpy`
  * `matplotlib`
  * `Pillow`

Instala Python y luego:

```bash
pip install pyserial numpy matplotlib Pillow
```

---

### 1. Configuración del Transmisor (Arduino)

1. Abre `transmisor.ino` en el Arduino IDE.
2. Asegúrate de que `imagen32x32.h` esté en la misma carpeta del sketch, con el array `image_data[32][4]`.
3. Conecta el módulo VirtualWire al pin digital 2 de la placa.
4. Selecciona la placa y puerto correctos (Herramientas → Placa → Arduino Uno; Puerto → COMx).
5. Sube el sketch al Arduino.

---

### 2. Configuración del Receptor (Arduino)

1. Abre `receptor.ino` en el Arduino IDE.
2. Conecta el módulo VirtualWire al pin digital 2 de la segunda placa.
3. Selecciona la placa y puerto adecuados.
4. Sube el sketch.
5. Abre el Monitor Serie (9600 bps) para ver la reconstrucción de la imagen.

---

### 3. Ejecución de `imagen2bits.py`

1. Coloca tu imagen en el directorio del script y renómbrala a `mi_imagen.png` o ajusta `INPUT_PATH`.
2. Ejecuta:

   ```bash
   python imagen2bits.py
   ```
3. El script genera una lista de paquetes en memoria (puedes enviarlos con un pequeño script en Python o guardarlos en `packets.bin`).

---

### 4. Ejecución de `llegada.py`

1. Conecta tu PC al puerto serie de la placa receptora Arduino.
2. Ajusta `PUERTO` y `BAUD_RATE` si es necesario.
3. Ejecuta:

   ```bash
   python llegada.py
   ```
4. La consola mostrará el progreso de recepción y, al finalizar, se abrirá una ventana con la imagen en escala de grises.

---

### Flujo de Trabajo Completo

1. **Convertir** imagen a paquetes: `imagen2bits.py`.
2. **Transmitir** por RF/Serial: Arduino Transmisor.
3. **Recibir y mostrar**:

   * Opción A: Arduino Receptor → Monitor Serie.
   * Opción B: PC + `llegada.py` → ventana con Matplotlib.

---

### Notas

* Asegúrate de que ambos Arduinos comparten la misma tasa de baudios (`2000` bps en VirtualWire, `9600` bps en Serial).
* Ajusta retardos (`delay()`) si el canal es muy ruidoso.
* Para imágenes diferentes de 32×32 píxeles, actualiza las constantes `WIDTH`, `HEIGHT` y recompila.

---

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

# Guía Rápida: Enviar y Reconstruir Imagen 32×32 vía Arduino 433 MHz

## Requisitos
- **Hardware**  
  - 2× Arduino UNO (o equivalente)  
  - 1× módulo TX 433 MHz + antena  
  - 1× módulo RX 433 MHz + antena  
  - 2× LED + resistencias 220 Ω  
  - Cables Dupont / breadboard

- **Software**  
  - Python 3.x con: `pillow`, `numpy`, `pyserial`, `matplotlib`  
    ```bash
    pip install pillow numpy pyserial matplotlib
    ```  
  - IDE de Arduino con la librería **VirtualWire**

---

## Archivos Principales
- **`tx_433_image_debug.ino`**  → Sketch para Arduino TX  
- **`rx_433_debug_data.ino`** → Sketch para Arduino RX  
- **`ima.py`**                → Script Python que procesa la imagen y envía 128 paquetes  
- **`reconstruir.py`**        → Script Python que convierte 32 líneas de “0”/“1” en imagen  
- **`cofre.png`**             → Ejemplo de imagen a transmitir (32 × 32 px)

---

## Paso a Paso

1. **Preparar conexiones**  
   - **TX (emisor):**  
     - Módulo TX 433 MHz: VCC→5 V, GND→GND, DATA→D2, ANT→antena (~10 cm)  
     - LED indicador (opcional) → D13 → GND (resistencia 220 Ω)  
   - **RX (receptor):**  
     - Módulo RX 433 MHz: VCC→5 V, GND→GND, DATA→D2, ANT→antena (~10 cm)  
     - LED indicador → D8 → GND (resistencia 220 Ω)  

2. **Cargar sketches en Arduino**  
   - En **Arduino TX**, cargar `tx_433_image_debug.ino` y **cerrar Monitor Serie**.  
   - En **Arduino RX**, cargar `rx_433_debug_data.ino` y **abrir Monitor Serie a 9600 bps**.

3. **Enviar imagen desde Python**  
   - Editar `ima.py` para poner la ruta de la imagen (`INPUT_PATH`) y el puerto COM de TX (`PUERTO`).  
   - Asegurarse de que el Monitor Serie de TX esté cerrado.  
   - Ejecutar:
     ```bash
     python ima.py
     ```
   - En RX (Monitor Serie) deberán aparecer 128 líneas de “seq = …   data = 0x…” y, al final, 32 líneas de “0”/“1”.

4. **Reconstruir la imagen en el PC**  
   - Copiar las **32 líneas** de “0”/“1” desde el Monitor Serie de RX.  
   - Editar `reconstruir.py`, pegar esas líneas en `matrix_lines`.  
   - Ejecutar:
     ```bash
     python reconstruir.py
     ```
   - Ver la imagen 32×32 en pantalla y (opcional) encontrar `reconstruida_32x32.png` en la carpeta.

---
| Byte | Nombre         | Valor / Descripción                                                                                         |
|:----:|:---------------|:-------------------------------------------------------------------------------------------------------------|
|  0   | **START**      | `0xAA` (constante para marcar inicio de paquete)                                                             |
|  1   | **SENDER_ID**  | `0x01` (identificador fijo del emisor)                                                                       |
|  2   | **RECEIVER_ID**| `0x02` (identificador fijo del receptor)                                                                      |
|  3   | **SEQ**        | Número de secuencia `0…127` (hay 128 paquetes en total). Indica la posición de estos 8 píxeles dentro de los 1 024 de la imagen 32×32.  |
|  4   | **DATA**       | 1 byte que contiene 8 bits de imagen (cada bit = un píxel B/N). “MSB primero”:  
- Bit 7 → píxel (SEQ×8 + 0)  
- Bit 6 → píxel (SEQ×8 + 1)  
- …  
- Bit 0 → píxel (SEQ×8 + 7) |
|  5   | **CHECKSUM**   | Suma de los bytes 0 a 4, calculada como `(Byte0 + Byte1 + Byte2 + Byte3 + Byte4) & 0xFF`. Sirve para verificar integridad. |

- **Total de paquetes:** 128 (SEQ de 0 a 127), cubriendo 128 × 8 = 1 024 píxeles de la imagen 32×32.

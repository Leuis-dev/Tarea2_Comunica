# ima_crc8_loop.py
# ----------------
# Versión de ima_crc8.py que envía la imagen en loop continuo:
# cada ronda envía 128 encabezados de 5 bytes y espera el "[TX-ACK]seq" antes
# de enviar el siguiente. Al finalizar 128 secuencias, pausa 2 s y vuelve a empezar.

from PIL import Image
import numpy as np
import serial
import time

# ——————————————————————————————————————————
# Parámetros de la imagen y protocolo de 5 bytes
# ——————————————————————————————————————————
INPUT_PATH   = r'C:/Users/sebas/OneDrive/Escritorio/archgithub/cofre.png'
WIDTH        = 32
HEIGHT       = 32
PIXELS       = WIDTH * HEIGHT           # 1 024 bits totales
BITS_PER_PKT = 8                        # 8 bits = 1 byte de datos
TOTAL_PKTS   = PIXELS // BITS_PER_PKT   # 1 024/8 = 128 paquetes

START_BYTE   = 0xAA
SENDER_ID    = 0x01
RECEIVER_ID  = 0x03    # Debe coincidir con MY_RECEIVER_ID en el RX Arduino

# ——————————————————————————————————————————
# Paso 1: Cargar imagen, redimensionar a 32×32 y convertir a escala de grises
# ——————————————————————————————————————————
img = Image.open(INPUT_PATH).resize((WIDTH, HEIGHT)).convert('L')
arr = np.array(img)
binary_arr = (arr > 128).astype(np.uint8)  # 1 si >128, 0 si ≤128

# DEBUG: muestra los primeros 8 bits binarios
print("[Python DEBUG] Primeros 8 bits binarios:", binary_arr.flatten()[:8])

# ——————————————————————————————————————————
# Paso 2: Empacar cada 8 bits en un byte → lista de 128 bytes
# ——————————————————————————————————————————
packed_bytes = []
for i in range(0, PIXELS, BITS_PER_PKT):
    byte = 0
    for b in range(BITS_PER_PKT):
        byte = (byte << 1) | int(binary_arr.flatten()[i + b])
    packed_bytes.append(byte)

# DEBUG: muestra los primeros 4 bytes empacados
print("[Python DEBUG] Primeros 4 bytes empacados:", [f"{b:02X}" for b in packed_bytes[:4]])

# ——————————————————————————————————————————
# Paso 3: Construir 128 encabezados de 5 bytes:
# [START_BYTE, SENDER_ID, RECEIVER_ID, seq, data]
# ——————————————————————————————————————————
headers = []
for seq in range(TOTAL_PKTS):
    data = packed_bytes[seq]
    header = [START_BYTE, SENDER_ID, RECEIVER_ID, seq, data]
    headers.append(header)

print(f"[Python] Total de encabezados a enviar por ronda: {len(headers)}")

# ——————————————————————————————————————————
# Paso 4: Abrir Serial a 9600 bps (TX Arduino) y repetir envío en loop
#         Esperar el "[TX-ACK]seq" antes de cada siguiente encabezado.
# ——————————————————————————————————————————
PUERTO    = 'COM6'   # Cambia al puerto correcto de tu Arduino TX
BAUD_RATE = 9600

try:
    ser = serial.Serial(PUERTO, BAUD_RATE, timeout=0.1)
except serial.SerialException as e:
    print(f"[Python ERROR] No se pudo abrir {PUERTO}: {e}")
    exit(1)

time.sleep(2.5)               # Esperar a que Arduino TX inicie
ser.reset_input_buffer()
ser.reset_output_buffer()

print("[Python] Iniciando envío en loop. Cortar con Ctrl+C para detener.")

while True:
    print("[Python] Nueva ronda: enviando 128 encabezados…")
    for idx, h in enumerate(headers):
        packet5 = bytes(h)
        # Verificación: aseguramos que se envían exactamente 5 bytes
        assert len(packet5) == 5, f"Error: se intentan enviar {len(packet5)} bytes en lugar de 5."

        # 1) Enviar 5 bytes
        ser.write(packet5)
        ser.flush()
        print(f"[Python] Enviado header {idx:03d} → {' '.join(f'{x:02X}' for x in h)}")

        # 2) Esperar el "[TX-ACK]idx" que envíe el Arduino TX
        while True:
            linea = ser.readline().decode(errors='ignore').strip()
            if linea.startswith("[TX-ACK]"):
                try:
                    ack_seq = int(linea.replace("[TX-ACK]", ""))
                except ValueError:
                    continue
                if ack_seq == idx:
                    # Recibimos el ACK correcto
                    break
            # Repetir hasta recibir el ACK correcto

    print("[Python] Ronda completa. Pausando 2 segundos antes de la siguiente ronda.")
    time.sleep(2.0)

# ima.py (corregido)
# ------------------
# 1) Redimensiona la imagen a 32×32
# 2) Convierte a escala de grises ('L') y aplica un umbral para binarizar
# 3) Empaca cada 8 bits en un byte (128 bytes en total)
# 4) Crea 128 paquetes de 6 bytes: [0xAA, 0x01, 0x02, seq, data, checksum]
# 5) Envía cada paquete en orden 0→127 por Serial al Arduino TX

from PIL import Image
import numpy as np
import serial
import time

# ——————————————————————————————————————————
# Parámetros de la imagen y protocolo de 6 bytes
# ——————————————————————————————————————————
INPUT_PATH   = r'C:\Users\sebas\Downloads\Tarea2_Comunica-main (1)\Tarea2_Comunica-main\espada.png'
WIDTH        = 32
HEIGHT       = 32
PIXELS       = WIDTH * HEIGHT          # 1024 bits totales
BITS_PER_PKT = 8                       # 8 bits = 1 byte de datos por paquete
TOTAL_PKTS   = PIXELS // BITS_PER_PKT  # 1024/8 = 128 paquetes

START_BYTE   = 0xAA
SENDER_ID    = 0x01
RECEIVER_ID  = 0x02

# ——————————————————————————————————————————
# Paso 1: Cargar imagen, redimensionar a 32×32 y convertir a escala de grises
# ——————————————————————————————————————————
img = Image.open(INPUT_PATH).resize((WIDTH, HEIGHT)).convert('L')
arr = np.array(img)  # valores 0–255 en escala de grises

# ——————————————————————————————————————————
# Paso 2: Binarizar mediante umbral (por ejemplo, 128)
# ——————————————————————————————————————————
binary_arr = (arr > 128).astype(np.uint8)  # 1 si >128, 0 si ≤128

# DEBUG: imprimir los primeros 8 bits binarios para verificar
print("[Python DEBUG] Primeros 8 bits binarios:",
      binary_arr.flatten()[:8])

# ——————————————————————————————————————————
# Paso 3: Empacar cada 8 bits en un byte → lista de 128 bytes
# ——————————————————————————————————————————
packed_bytes = []
for i in range(0, PIXELS, BITS_PER_PKT):
    byte = 0
    for b in range(BITS_PER_PKT):
        byte = (byte << 1) | int(binary_arr.flatten()[i + b])
    packed_bytes.append(byte)

# DEBUG: mostrar los primeros 4 bytes empacados
print("[Python DEBUG] Primeros 4 bytes empacados:",
      [f"{b:02X}" for b in packed_bytes[:4]])

# ——————————————————————————————————————————
# Paso 4: Construir los 128 paquetes de 6 bytes cada uno
# ——————————————————————————————————————————
packets = []
for seq in range(TOTAL_PKTS):
    data = packed_bytes[seq]
    packet = [
        START_BYTE,
        SENDER_ID,
        RECEIVER_ID,
        seq,
        data
    ]
    checksum = sum(packet) & 0xFF
    packet.append(checksum)
    packets.append(packet)

print(f"[Python] Total de paquetes a enviar: {len(packets)}")
# ——————————————————————————————————————————
# Paso 5: Abrir Serial y enviar cada paquete en orden 0→127
# ——————————————————————————————————————————
PUERTO    = 'COM6'   # Cambia por tu puerto TX real
BAUD_RATE = 9600

ser = serial.Serial(PUERTO, BAUD_RATE, timeout=1)
time.sleep(2.5)               # Esperar que Arduino TX arranque
ser.reset_input_buffer()
ser.reset_output_buffer()

repeticiones = 3
for rep in range(repeticiones):
    print(f"[Python] Enviando repetición {rep+1}/{repeticiones}")
    for idx, p in enumerate(packets):
        ser.write(bytes(p))
        ser.flush()
        print(f"[Python] Enviado paquete {idx:03d}/127  Bytes = {' '.join(f'{b:02X}' for b in p)}")
        time.sleep(0.10)          # 100 ms de retardo para evitar solapamientos

print("[Python] Todos los paquetes enviados 3 veces. Cerrando Serial.")
ser.close()

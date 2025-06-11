from PIL import Image
import numpy as np
import serial
import time

# —— Cifrado por multiplicación ——
e = 3
n = 257

WIDTH, HEIGHT = 32, 32
PIXELS = WIDTH * HEIGHT
BITS_PER_PKT = 8
TOTAL_PKTS = PIXELS // BITS_PER_PKT
START_BYTE, SENDER_ID, RECEIVER_ID = 0xAA, 0x01, 0x03
PUERTO, BAUD_RATE = 'COM4', 9600  # AJUSTA ESTO SEGÚN TU COMPUTADOR

# Ruta de la imagen que quieres enviar
INPUT_PATH = r'C:\Users\benja\Downloads\safeimagekit-image (3).png'

def cifrar_multiplicacion(byte):
    cifrado = (byte * e) % n
    print(f"[Python] Original: {byte:03} → Cifrado: {cifrado:03}")
    return cifrado

# Abrir y procesar imagen real
img = Image.open(INPUT_PATH).resize((WIDTH, HEIGHT)).convert('L')
arr = np.array(img)

# Mostrar algunos valores para verificar que no son ceros
print("[Python] Muestra de matriz de imagen (grises):")
print(arr[:4, :8])

# Convertimos a binario (1 si <128, 0 si ≥128): negro = 1
binary_arr = (arr < 128).astype(np.uint8)

# Empaquetar bits en bytes
packed_bytes = []
for i in range(0, PIXELS, BITS_PER_PKT):
    byte = 0
    for b in range(BITS_PER_PKT):
        byte = (byte << 1) | int(binary_arr.flatten()[i + b])
    packed_bytes.append(cifrar_multiplicacion(byte))

# Construir headers de paquetes
headers = [[START_BYTE, SENDER_ID, RECEIVER_ID, seq, data] for seq, data in enumerate(packed_bytes)]

# Inicializar comunicación serial
try:
    ser = serial.Serial(PUERTO, BAUD_RATE, timeout=0.1)
except serial.SerialException as e:
    print(f"[ERROR] {e}")
    exit(1)

# Espera inicial
time.sleep(2.5)
ser.reset_input_buffer()
ser.reset_output_buffer()
print(f"[Python] Envío activo con cifrado multiplicativo (x{e} mod {n})")

# Envío en bucle con confirmación por ACK
while True:
    print("[Python] Nueva ronda de envío")
    for idx, h in enumerate(headers):
        ser.write(bytes(h))
        ser.flush()
        print(f"[Python] Enviado header {idx:03d} → {' '.join(f'{x:02X}' for x in h)}")
        while True:
            ack = ser.readline().decode(errors='ignore').strip()
            if ack.startswith("[TX-ACK]") and ack.endswith(str(idx)):
                break
    print("[Python] Ronda completa. Pausa 2 s.")
    time.sleep(2.0)
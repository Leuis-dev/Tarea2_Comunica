from PIL import Image
import numpy as np
import serial
import time

# ——— CIFRADO ASIMÉTRICO  ———
e = 7       # Clave pública
n = 187     # Módulo (producto de dos primos)

INPUT_PATH = r'C:/Users/sebas/OneDrive/Escritorio/archgithub/cofre.png'
WIDTH, HEIGHT = 32, 32
PIXELS = WIDTH * HEIGHT
BITS_PER_PKT = 8
TOTAL_PKTS = PIXELS // BITS_PER_PKT
START_BYTE, SENDER_ID, RECEIVER_ID = 0xAA, 0x01, 0x03
PUERTO, BAUD_RATE = 'COM6', 9600

def cifrar_rsa(byte):
    return pow(byte, e, n)

img = Image.open(INPUT_PATH).resize((WIDTH, HEIGHT)).convert('L')
arr = np.array(img)
binary_arr = (arr > 128).astype(np.uint8)

packed_bytes = []
for i in range(0, PIXELS, BITS_PER_PKT):
    byte = 0
    for b in range(BITS_PER_PKT):
        byte = (byte << 1) | int(binary_arr.flatten()[i + b])
    packed_bytes.append(cifrar_rsa(byte))

headers = [[START_BYTE, SENDER_ID, RECEIVER_ID, seq, data] for seq, data in enumerate(packed_bytes)]

try:
    ser = serial.Serial(PUERTO, BAUD_RATE, timeout=0.1)
except serial.SerialException as e:
    print(f"[ERROR] {e}")
    exit(1)

time.sleep(2.5)
ser.reset_input_buffer()
ser.reset_output_buffer()
print(f"[Python] Cifrado RSA activo (e = {e}, n = {n})")

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

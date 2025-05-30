import serial
import numpy as np
import matplotlib.pyplot as plt

# Parámetros
PUERTO      = 'COM5'
BAUD_RATE   = 9600
WIDTH       = 32
PIXELS      = WIDTH * WIDTH
BITS_PER_P  = 24
BYTES_PER_P = BITS_PER_P // 8    # =3
PACKETS     = PIXELS // BITS_PER_P

# Protocolo
START_BYTE  = 0xAA
SENDER_ID   = 0x01
RECEIVER_ID = 0x02

ser = serial.Serial(PUERTO, BAUD_RATE, timeout=1)
img = np.zeros((WIDTH, WIDTH), dtype=np.uint8)

print("Esperando paquetes...")

received = 0
while received < PACKETS:
    b = ser.read(1)
    if not b or b[0] != START_BYTE:
        continue

    resto = ser.read(1 + 1 + BYTES_PER_P + 1)
    if len(resto) != (1 + 1 + BYTES_PER_P + 1):
        continue

    sender, receiver, seq, b0, b1, b2, checksum = resto
    if sender != SENDER_ID or receiver != RECEIVER_ID:
        continue
    if seq >= PACKETS:
        continue

    suma = (START_BYTE + sender + receiver + seq + b0 + b1 + b2) & 0xFF
    if suma != checksum:
        print(f"Paquete {seq} corrupto")
        continue

    bits = (b0 << 16) | (b1 << 8) | b2
    for bit in range(BITS_PER_P):
        gbit = seq * BITS_PER_P + bit
        row = gbit // WIDTH
        col = gbit % WIDTH
        img[row, col] = (bits >> (BITS_PER_P - 1 - bit)) & 0x01

    received += 1
    print(f"Recibido paquete {seq+1}/{PACKETS}")

plt.figure(figsize=(4,4))
plt.imshow(img, cmap='gray', vmin=0, vmax=1, interpolation='nearest')
plt.axis('off')
plt.show()

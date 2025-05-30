from PIL import Image
import numpy as np

# Parámetros
INPUT_PATH  = 'mi_imagen.png'    # imagen B/N que quieras usar
WIDTH        = 32
HEIGHT       = 32
BITS_PER_P   = 24
BYTES_PER_P  = BITS_PER_P // 8   # =3
PIXELS       = WIDTH * HEIGHT
PACKETS      = PIXELS // BITS_PER_P

# Carga y convierte a B/N de 32×32
img = Image.open(INPUT_PATH).convert('1').resize((WIDTH, HEIGHT))
arr = np.array(img, dtype=np.uint8) // 255  # 0/1

# Aplanar bits
flat = arr.flatten()  # 1024 valores 0/1

# Empacar en bytes
packed_bytes = []
for i in range(0, PIXELS, 8):
    byte = 0
    for bit in range(8):
        byte = (byte << 1) | int(flat[i + bit])
    packed_bytes.append(byte)

# Construir paquetes
START_BYTE, SENDER_ID, RECEIVER_ID = 0xAA, 0x01, 0x02
packets = []

for seq in range(PACKETS):
    base = seq * BYTES_PER_P
    data_bytes = packed_bytes[base:base + BYTES_PER_P]
    packet = [START_BYTE, SENDER_ID, RECEIVER_ID, seq] + data_bytes
    checksum = sum(packet) & 0xFF
    packet.append(checksum)
    packets.append(packet)

print(f"Generados {len(packets)} paquetes.")
# Ahora puedes enviarlos con pyserial o guardarlos en un archivo.

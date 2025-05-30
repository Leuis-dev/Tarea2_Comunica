import serial
import numpy as np
import matplotlib.pyplot as plt

# Ajusta el puerto al tuyo
ser = serial.Serial('COM5', 9600, timeout=1)

while True:
    # 1) Leer encabezado y quitar solo '\r' y '\n'
    header = ser.readline().decode('utf-8', errors='ignore').rstrip('\r\n')
    # print("DEBUG header:", repr(header))

    if header.startswith('===== IMAGEN RECIBIDA ====='):
        print("Imagen recibida, procesando...")
        data = []
        for _ in range(32):
            # 2) Leer la fila y quitar solo saltos de línea
            row = ser.readline().decode('utf-8', errors='ignore').rstrip('\r\n')
            # 3) Si quedó corta (menos de 32), rellenar con espacios
            if len(row) < 32:
                row = row.ljust(32)
            # 4) Convertir cada uno de los 32 caracteres
            data.append([1 if c == '#' else 0 for c in row[:32]])
        img = np.array(data)

        # 5) Mostrar la imagen
        plt.figure()
        plt.imshow(img, cmap='gray', interpolation='nearest')
        plt.axis('off')
        plt.show()

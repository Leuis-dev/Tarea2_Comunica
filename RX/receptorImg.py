import serial
import numpy as np
import matplotlib.pyplot as plt

# Configura tu puerto
puerto = 'COM6'   # Cambia según tu sistema
baudrate = 9600

def leer_imagen_serial():
    try:
        ser = serial.Serial(puerto, baudrate, timeout=1)
    except serial.SerialException as e:
        print(f"ERROR: No se pudo abrir el puerto serial: {e}")
        return

    imagen = []
    leyendo = False
    lineas_leidas = 0
    timeout_counter = 0
    max_timeout = 1000

    print("Esperando imagen desde Arduino...")

    while True:
        linea = ser.readline().decode('utf-8', errors='ignore').strip()
        if linea.startswith("[RX]"):
                    print(f"[PAQUETE] {linea}") 
        if not linea:
            timeout_counter += 1
            if timeout_counter > max_timeout:
                print("ERROR: Timeout esperando datos del Arduino.")
                break
            continue
        else:
            timeout_counter = 0

        if not leyendo:
            if linea == "----- Imagen 32x32 reconstruida -----":


                leyendo = True
                imagen = []
                lineas_leidas = 0
                print("Inicio de imagen")
            continue

        # Ya estamos leyendo la imagen
        if leyendo:
            if linea == "----- FIN -----":
                if len(imagen) == 32:
                    print("Imagen completa recibida.")
                    mostrar_imagen(imagen)
                else:
                    print(f"ERROR: Imagen incompleta. Se recibieron {len(imagen)} líneas.")
                leyendo = False
                imagen = []
                continue

            # Validar línea válida (32 caracteres 0/1)
            if len(linea) == 32 and all(c in '01' for c in linea):
                fila = [int(bit) for bit in linea]
                imagen.append(fila)
                lineas_leidas += 1
            else:
                print(f"WARNING: Línea inválida recibida: '{linea}'")

def mostrar_imagen(imagen):
    matriz = np.array(imagen)
    print("Matriz recibida:")
    plt.imshow(matriz, cmap='gray', interpolation='nearest')  # 1-bit: 0=negro, 1=blanco
    plt.title("Imagen recibida desde Arduino")
    plt.axis('off')
    plt.show()

if __name__ == "__main__":
    leer_imagen_serial()

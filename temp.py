import time
import serial
import threading

def read_thread(ser):
    while True:
        num_available = ser.in_waiting
        if (num_available > 0):
            print(ser.read(num_available))
        else:
            time.sleep(0.01)

with serial.Serial('/dev/ttyUSB0', baudrate=250000) as ser:
    read_thread = threading.Thread(target=read_thread, args=(ser,))
    read_thread.start()  # Start a new thread that runs the read_thread function
    read_thread.join()  # Wait for the new thread to finish

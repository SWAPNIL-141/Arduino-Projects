import serial
import time
from datetime import datetime

ser = serial.Serial("COM9", 9600)  
time.sleep(2)

while True:
    now = datetime.now()
    saat = now.strftime("%H:%M:%S")  # Saat: Dakika: Saniye
    tarih = now.strftime("%d-%m-%y")  # Gün-Ay-YIL (2 Haneli Yıl)

    veri = f"{saat}|{tarih}\n"  
    ser.write(veri.encode())  
    time.sleep(1)

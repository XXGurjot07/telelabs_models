import serial
import time
sum = 15
ser = serial.Serial('/dev/ttyACM1', 115200, timeout=1)
ser.flush()
while True :
    
    time.sleep(2)
    ser.write(sum)
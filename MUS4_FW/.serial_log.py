import serial
import sys
import time

PORT = 'COM21'
BAUD = 115200
LOG = 'ota_serial.log'

try:
    ser = serial.Serial()
    ser.port = PORT
    ser.baudrate = BAUD
    ser.dtr = False
    ser.rts = False
    ser.timeout = 1
    ser.open()
    print(f'[monitor] opened {PORT}', flush=True)
    with open(LOG, 'wb') as f:
        start = time.time()
        while time.time() - start < 180:
            try:
                data = ser.read(ser.in_waiting or 1)
                if data:
                    f.write(data)
                    f.flush()
            except Exception as e:
                print(f'[monitor] read error: {e}', flush=True)
                break
except Exception as e:
    print(f'[monitor] open failed: {e}', flush=True)
    sys.exit(1)

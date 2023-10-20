import time
import board
import busio
from digitalio import DigitalInOut, Direction, Pull
import adafruit_rfm9x
import config

print("Intializing µPico")

rf1 = DigitalInOut(board.GP23)
rf1.direction = Direction.OUTPUT
rf1.value = True
time.sleep(0.25)

rf2 = DigitalInOut(board.GP22)
rf2.direction = Direction.OUTPUT
rf2.value = True
time.sleep(0.25)

rf3 = DigitalInOut(board.GP14)
rf3.direction = Direction.OUTPUT
rf3.value = True
time.sleep(0.25)

rf4 = DigitalInOut(board.GP13)
rf4.direction = Direction.OUTPUT
rf4.value = True
time.sleep(0.25)

rf5 = DigitalInOut(board.GP0) # 23
rf5.direction = Direction.OUTPUT
rf5.value = True
time.sleep(0.25)

rf6 = DigitalInOut(board.GP1) # 22
rf6.direction = Direction.OUTPUT
rf6.value = True
time.sleep(0.25)

rf7 = DigitalInOut(board.GP2) # 14
rf7.direction = Direction.OUTPUT
rf7.value = True
time.sleep(0.25)

rf8 = DigitalInOut(board.GP3) # 13
rf8.direction = Direction.OUTPUT
rf8.value = True
time.sleep(0.25)

if (config.DEFAULT is not 8):
    rf8.value = False
if (config.DEFAULT is not 7):
    rf7.value = False
if (config.DEFAULT is not 6):
    rf6.value = False
if (config.DEFAULT is not 5):
    rf5.value = False
if (config.DEFAULT is not 4):
    rf4.value = False
if (config.DEFAULT is not 3):
    rf3.value = False
if (config.DEFAULT is not 2):
    rf2.value = False
if (config.DEFAULT is not 1):
    rf1.value = False

# Lora Stuff
RADIO_FREQ_MHZ = 868.000
CS = DigitalInOut(board.GP21)
RESET = DigitalInOut(board.GP20)
spi = busio.SPI(board.GP10, MOSI=board.GP11, MISO=board.GP8)
rfm9x = adafruit_rfm9x.RFM9x(spi, CS, RESET, RADIO_FREQ_MHZ, baudrate=1000000, agc=False,crc=True)
rfm9x.tx_power = 5

while True:
    print(f"loraRunner: Waiting for LoRa packet ...\r", end="")
    packet = rfm9x.receive(with_header=True,timeout=10)

    if packet is not None:
        print(packet)
        if packet[:3] == (b'<\xaa\x01'):
                rawdata = bytes(packet[3:]).decode('utf-8')
                if rawdata.startswith(config.DEVICE):
                    port = int(rawdata[-1])
                    print(port)
                    if port is 1:
                        rf1.value = True
                        rf2.value = False
                        rf3.value = False
                        rf4.value = False
                        rf5.value = False
                        rf6.value = False
                        rf7.value = False
                        rf8.value = False
                    
                    if port is 2:
                        rf1.value = False
                        rf2.value = True
                        rf3.value = False
                        rf4.value = False
                        rf5.value = False
                        rf6.value = False
                        rf7.value = False
                        rf8.value = False
                    
                    if port is 3:
                        rf1.value = False
                        rf2.value = False
                        rf3.value = True
                        rf4.value = False
                        rf5.value = False
                        rf6.value = False
                        rf7.value = False
                        rf8.value = False
                    
                    if port is 4:
                        rf1.value = False
                        rf2.value = False
                        rf3.value = False
                        rf4.value = True
                        rf5.value = False
                        rf6.value = False
                        rf7.value = False
                        rf8.value = False

                    if port is 5:
                        rf1.value = False
                        rf2.value = False
                        rf3.value = False
                        rf4.value = False
                        rf5.value = True
                        rf6.value = False
                        rf7.value = False
                        rf8.value = False
                    
                    if port is 6:
                        rf1.value = False
                        rf2.value = False
                        rf3.value = False
                        rf4.value = False
                        rf5.value = False
                        rf6.value = True
                        rf7.value = False
                        rf8.value = False
                    
                    if port is 7:
                        rf1.value = False
                        rf2.value = False
                        rf3.value = False
                        rf4.value = False
                        rf5.value = False
                        rf6.value = False
                        rf7.value = True
                        rf8.value = False
                    
                    if port is 8:
                        rf1.value = False
                        rf2.value = False
                        rf3.value = False
                        rf4.value = False
                        rf5.value = False
                        rf6.value = False
                        rf7.value = False
                        rf8.value = True
        else:
            print("Unknown packet")
import time
import board
import busio
from digitalio import DigitalInOut, Direction, Pull
import adafruit_rfm9x
import config

print("Intializing µPico")

rf1 = DigitalInOut(board.GP5)
rf1.direction = Direction.OUTPUT

rf1.value = True
time.sleep(0.25)
rf1.value = False

rf2 = DigitalInOut(board.GP6)
rf2.direction = Direction.OUTPUT

rf2.value = True
time.sleep(0.25)
rf2.value = False

rf3 = DigitalInOut(board.GP7)
rf3.direction = Direction.OUTPUT

rf3.value = True
time.sleep(0.25)
rf3.value = False

rf4 = DigitalInOut(board.GP8)
rf4.direction = Direction.OUTPUT

rf4.value = True
time.sleep(0.25)
rf4.value = False

# Lora Stuff
RADIO_FREQ_MHZ = 868.000
CS = DigitalInOut(board.GP21)
RESET = DigitalInOut(board.GP20)
spi = busio.SPI(board.GP18, MOSI=board.GP19, MISO=board.GP16)
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
                    
                    if port is 2:
                        rf1.value = False
                        rf2.value = True
                        rf3.value = False
                        rf4.value = False
                    
                    if port is 3:
                        rf1.value = False
                        rf2.value = False
                        rf3.value = True
                        rf4.value = False
                    
                    if port is 4:
                        rf1.value = False
                        rf2.value = False
                        rf3.value = False
                        rf4.value = True
        else:
            print("Unknown packet")
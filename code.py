import time
import board
import busio
from digitalio import DigitalInOut, Direction, Pull
import adafruit_rfm9x
import config

def purple(data):
  stamp = time.time()
  return "\x1b[38;5;104m[" + str(stamp) + "] " + data + "\x1b[0m"

def yellow(data):
  return "\x1b[38;5;220m" + data + "\x1b[0m"

def red(data):
  return "\x1b[1;5;31m -- " + data + "\x1b[0m"

# our version
VERSION = "RF.Guru_8P_Switch_LoRa 0.1" 

rf1 = DigitalInOut(board.GP23)
rf1.direction = Direction.OUTPUT
rf1.value = True
time.sleep(0.01)

rf2 = DigitalInOut(board.GP22)
rf2.direction = Direction.OUTPUT
rf2.value = True
time.sleep(0.01)

rf3 = DigitalInOut(board.GP14)
rf3.direction = Direction.OUTPUT
rf3.value = True
time.sleep(0.01)

rf4 = DigitalInOut(board.GP13)
rf4.direction = Direction.OUTPUT
rf4.value = True
time.sleep(0.01)

rf5 = DigitalInOut(board.GP0) 
rf5.direction = Direction.OUTPUT
rf5.value = True
time.sleep(0.01)

rf6 = DigitalInOut(board.GP1) 
rf6.direction = Direction.OUTPUT
rf6.value = True
time.sleep(0.01)

rf7 = DigitalInOut(board.GP2) 
rf7.direction = Direction.OUTPUT
rf7.value = True
time.sleep(0.01)

rf8 = DigitalInOut(board.GP3) 
rf8.direction = Direction.OUTPUT
rf8.value = True
time.sleep(0.01)


ports = {
  "1": rf1,
  "2": rf2,
  "3": rf3,
  "4": rf4,
  "5": rf5,
  "6": rf6,
  "7": rf7,
  "8": rf8,
}

for number, port in ports.items():
    port.value = False
    time.sleep(0.01)

ports[str(int(config.default_port))].value = True

print(red(config.name + " -=- " + VERSION))

# Lora Stuff
RADIO_FREQ_MHZ = 868.000
CS = DigitalInOut(board.GP21)
RESET = DigitalInOut(board.GP20)
spi = busio.SPI(board.GP10, MOSI=board.GP11, MISO=board.GP8)
rfm9x = adafruit_rfm9x.RFM9x(spi, CS, RESET, RADIO_FREQ_MHZ, baudrate=1000000, agc=False,crc=True)
rfm9x.tx_power = 5

while True:
    msg = yellow("Waiting for LoRa packet ...")
    print(f"{msg}\r", end="")
    packet = rfm9x.receive(with_header=True,timeout=10)

    if packet is not None:
        #print(packet)
        if packet[:3] == (b'<\xaa\x01'):
                rawdata = bytes(packet[3:]).decode('utf-8')
                if rawdata.startswith(config.name):
                    for number, port in ports.items():
                        port.value = False
                    try:
                        ports[str(int(rawdata[-1]))].value = True
                        print(purple("PORT REQ: Turned port " + str(int(rawdata[-1])) + " on"))
                    except:
                        print(purple("PORT REQ: Wrong Port NR Turned default port " + str(int(config.default_port)) + " on"))
                        for number, port in ports.items():
                            port.value = False
                        ports[str(int(config.default_port))].value = True
                else:
                    print(yellow("Received another switch port req packet: " + str(rawdata)))
        else:
            print(yellow("Received an unknown packet: " + str(packet)))
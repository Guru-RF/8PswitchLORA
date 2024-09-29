import time

import adafruit_rfm9x
import board
import busio
from digitalio import DigitalInOut, Direction
from microcontroller import watchdog as w
from watchdog import WatchDogMode

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

for _number, port in ports.items():
    port.value = False
    time.sleep(0.01)

ports[str(int(config.default_port))].value = True

att1 = DigitalInOut(board.GP6)
att1.direction = Direction.OUTPUT
att1.value = False
time.sleep(0.01)

att2 = DigitalInOut(board.GP5)
att2.direction = Direction.OUTPUT
att2.value = False
time.sleep(0.01)

att3 = DigitalInOut(board.GP26)
att3.direction = Direction.OUTPUT
att3.value = False
time.sleep(0.01)

att4 = DigitalInOut(board.GP27)
att4.direction = Direction.OUTPUT
att4.value = False
time.sleep(0.01)

att5 = DigitalInOut(board.GP28)
att5.direction = Direction.OUTPUT
att5.value = False
time.sleep(0.01)

att6 = DigitalInOut(board.GP29)
att6.direction = Direction.OUTPUT
att6.value = False
time.sleep(0.01)

attAct = DigitalInOut(board.GP25)
attAct.direction = Direction.OUTPUT
attAct.value = False
time.sleep(0.01)


def round_to_half(num):
    return round(num * 2) / 2


def att(value):
    if value == "0.0":
        attAct.value = False
    elif value == "0.5":
        attAct.value = True
        att1.value = True
        att2.value = True
        att3.value = True
        att4.value = True
        att5.value = True
        att6.value = False

    elif value == "1.0":
        attAct.value = True
        att1.value = True
        att2.value = True
        att3.value = True
        att4.value = True
        att5.value = False
        att6.value = True

    elif value == "1.5":
        attAct.value = True
        att1.value = True
        att2.value = True
        att3.value = True
        att4.value = True
        att5.value = False
        att6.value = False

    elif value == "2.0":
        attAct.value = True
        att1.value = True
        att2.value = True
        att3.value = True
        att4.value = False
        att5.value = True
        att6.value = True

    elif value == "2.5":
        attAct.value = True
        att1.value = True
        att2.value = True
        att3.value = True
        att4.value = False
        att5.value = True
        att6.value = False

    elif value == "3.0":
        attAct.value = True
        att1.value = True
        att2.value = True
        att3.value = True
        att4.value = False
        att5.value = False
        att6.value = True

    elif value == "3.5":
        attAct.value = True
        att1.value = True
        att2.value = True
        att3.value = True
        att4.value = False
        att5.value = False
        att6.value = False

    elif value == "4.0":
        attAct.value = True
        att1.value = True
        att2.value = True
        att3.value = False
        att4.value = True
        att5.value = True
        att6.value = True

    elif value == "4.5":
        attAct.value = True
        att1.value = True
        att2.value = True
        att3.value = False
        att4.value = True
        att5.value = True
        att6.value = False

    elif value == "5.0":
        attAct.value = True
        att1.value = True
        att2.value = True
        att3.value = False
        att4.value = True
        att5.value = False
        att6.value = True

    elif value == "5.5":
        attAct.value = True
        att1.value = True
        att2.value = True
        att3.value = False
        att4.value = True
        att5.value = False
        att6.value = False

    elif value == "6.0":
        attAct.value = True
        att1.value = True
        att2.value = True
        att3.value = False
        att4.value = False
        att5.value = True
        att6.value = True

    elif value == "6.5":
        attAct.value = True
        att1.value = True
        att2.value = True
        att3.value = False
        att4.value = False
        att5.value = True
        att6.value = False

    elif value == "7.0":
        attAct.value = True
        att1.value = True
        att2.value = True
        att3.value = False
        att4.value = False
        att5.value = False
        att6.value = True

    elif value == "7.5":
        attAct.value = True
        att1.value = True
        att2.value = True
        att3.value = False
        att4.value = False
        att5.value = False
        att6.value = False

    elif value == "8.0":
        attAct.value = True
        att1.value = True
        att2.value = False
        att3.value = True
        att4.value = True
        att5.value = True
        att6.value = True

    elif value == "8.5":
        attAct.value = True
        att1.value = True
        att2.value = False
        att3.value = True
        att4.value = True
        att5.value = True
        att6.value = False

    elif value == "9.0":
        attAct.value = True
        att1.value = True
        att2.value = False
        att3.value = True
        att4.value = True
        att5.value = False
        att6.value = True

    elif value == "9.5":
        attAct.value = True
        att1.value = True
        att2.value = False
        att3.value = True
        att4.value = True
        att5.value = False
        att6.value = False

    elif value == "10.0":
        attAct.value = True
        att1.value = True
        att2.value = False
        att3.value = True
        att4.value = False
        att5.value = True
        att6.value = True

    elif value == "10.5":
        attAct.value = True
        att1.value = True
        att2.value = False
        att3.value = True
        att4.value = False
        att5.value = True
        att6.value = False

    elif value == "11.0":
        attAct.value = True
        att1.value = True
        att2.value = False
        att3.value = True
        att4.value = False
        att5.value = False
        att6.value = True

    elif value == "11.5":
        attAct.value = True
        att1.value = True
        att2.value = False
        att3.value = True
        att4.value = False
        att5.value = False
        att6.value = False

    elif value == "12.0":
        attAct.value = True
        att1.value = True
        att2.value = False
        att3.value = False
        att4.value = True
        att5.value = True
        att6.value = True

    elif value == "12.5":
        attAct.value = True
        att1.value = True
        att2.value = False
        att3.value = False
        att4.value = True
        att5.value = True
        att6.value = False

    elif value == "13.0":
        attAct.value = True
        att1.value = True
        att2.value = False
        att3.value = False
        att4.value = True
        att5.value = False
        att6.value = True

    elif value == "13.5":
        attAct.value = True
        att1.value = True
        att2.value = False
        att3.value = False
        att4.value = True
        att5.value = False
        att6.value = False

    elif value == "14.0":
        attAct.value = True
        att1.value = True
        att2.value = False
        att3.value = False
        att4.value = False
        att5.value = True
        att6.value = True

    elif value == "14.5":
        attAct.value = True
        att1.value = True
        att2.value = False
        att3.value = False
        att4.value = False
        att5.value = True
        att6.value = False

    elif value == "15.0":
        attAct.value = True
        att1.value = True
        att2.value = False
        att3.value = False
        att4.value = False
        att5.value = False
        att6.value = True

    elif value == "15.5":
        attAct.value = True
        att1.value = True
        att2.value = False
        att3.value = False
        att4.value = False
        att5.value = False
        att6.value = False

    elif value == "16.0":
        attAct.value = True
        att1.value = False
        att2.value = True
        att3.value = True
        att4.value = True
        att5.value = True
        att6.value = True

    elif value == "16.5":
        attAct.value = True
        att1.value = False
        att2.value = True
        att3.value = True
        att4.value = True
        att5.value = True
        att6.value = False

    elif value == "17.0":
        attAct.value = True
        att1.value = False
        att2.value = True
        att3.value = True
        att4.value = True
        att5.value = False
        att6.value = True

    elif value == "17.5":
        attAct.value = True
        att1.value = False
        att2.value = True
        att3.value = True
        att4.value = True
        att5.value = False
        att6.value = False

    elif value == "18.0":
        attAct.value = True
        att1.value = False
        att2.value = True
        att3.value = True
        att4.value = False
        att5.value = True
        att6.value = True

    elif value == "18.5":
        attAct.value = True
        att1.value = False
        att2.value = True
        att3.value = True
        att4.value = False
        att5.value = True
        att6.value = False

    elif value == "19.0":
        attAct.value = True
        att1.value = False
        att2.value = True
        att3.value = True
        att4.value = False
        att5.value = False
        att6.value = True

    elif value == "19.5":
        attAct.value = True
        att1.value = False
        att2.value = True
        att3.value = True
        att4.value = False
        att5.value = False
        att6.value = False

    elif value == "20.0":
        attAct.value = True
        att1.value = False
        att2.value = True
        att3.value = False
        att4.value = True
        att5.value = True
        att6.value = True

    elif value == "20.5":
        attAct.value = True
        att1.value = False
        att2.value = True
        att3.value = False
        att4.value = True
        att5.value = True
        att6.value = False

    elif value == "21.0":
        attAct.value = True
        att1.value = False
        att2.value = True
        att3.value = False
        att4.value = True
        att5.value = False
        att6.value = True

    elif value == "21.5":
        attAct.value = True
        att1.value = False
        att2.value = True
        att3.value = False
        att4.value = True
        att5.value = False
        att6.value = False

    elif value == "22.0":
        attAct.value = True
        att1.value = False
        att2.value = True
        att3.value = False
        att4.value = False
        att5.value = True
        att6.value = True

    elif value == "22.5":
        attAct.value = True
        att1.value = False
        att2.value = True
        att3.value = False
        att4.value = False
        att5.value = True
        att6.value = False

    elif value == "23.0":
        attAct.value = True
        att1.value = False
        att2.value = True
        att3.value = False
        att4.value = False
        att5.value = False
        att6.value = True

    elif value == "23.5":
        attAct.value = True
        att1.value = False
        att2.value = True
        att3.value = False
        att4.value = False
        att5.value = False
        att6.value = False

    elif value == "24.0":
        attAct.value = True
        att1.value = False
        att2.value = False
        att3.value = True
        att4.value = True
        att5.value = True
        att6.value = True

    elif value == "24.5":
        attAct.value = True
        att1.value = False
        att2.value = False
        att3.value = True
        att4.value = True
        att5.value = True
        att6.value = False

    elif value == "25.0":
        attAct.value = True
        att1.value = False
        att2.value = False
        att3.value = True
        att4.value = True
        att5.value = False
        att6.value = True

    elif value == "25.5":
        attAct.value = True
        att1.value = False
        att2.value = False
        att3.value = True
        att4.value = True
        att5.value = False
        att6.value = False

    elif value == "26.0":
        attAct.value = True
        att1.value = False
        att2.value = False
        att3.value = True
        att4.value = False
        att5.value = True
        att6.value = True

    elif value == "26.5":
        attAct.value = True
        att1.value = False
        att2.value = False
        att3.value = True
        att4.value = False
        att5.value = True
        att6.value = False

    elif value == "27.0":
        attAct.value = True
        att1.value = False
        att2.value = False
        att3.value = True
        att4.value = False
        att5.value = False
        att6.value = True

    elif value == "27.5":
        attAct.value = True
        att1.value = False
        att2.value = False
        att3.value = True
        att4.value = False
        att5.value = False
        att6.value = False

    elif value == "28.0":
        attAct.value = True
        att1.value = False
        att2.value = False
        att3.value = False
        att4.value = True
        att5.value = True
        att6.value = True

    elif value == "28.5":
        attAct.value = True
        att1.value = False
        att2.value = False
        att3.value = False
        att4.value = True
        att5.value = True
        att6.value = False

    elif value == "29.0":
        attAct.value = True
        att1.value = False
        att2.value = False
        att3.value = False
        att4.value = True
        att5.value = False
        att6.value = True

    elif value == "29.5":
        attAct.value = True
        att1.value = False
        att2.value = False
        att3.value = False
        att4.value = True
        att5.value = False
        att6.value = False

    elif value == "30.0":
        attAct.value = True
        att1.value = False
        att2.value = False
        att3.value = False
        att4.value = False
        att5.value = True
        att6.value = True

    elif value == "30.5":
        attAct.value = True
        att1.value = False
        att2.value = False
        att3.value = False
        att4.value = False
        att5.value = True
        att6.value = False

    elif value == "31.0":
        attAct.value = True
        att1.value = False
        att2.value = False
        att3.value = False
        att4.value = False
        att5.value = False
        att6.value = True

    elif value == "31.5":
        attAct.value = True
        att1.value = False
        att2.value = False
        att3.value = False
        att4.value = False
        att5.value = False
        att6.value = False
    else:
        attAct.value = False


print(red(config.name + " -=- " + VERSION))

# Lora Stuff
RADIO_FREQ_MHZ = 868.000
CS = DigitalInOut(board.GP21)
RESET = DigitalInOut(board.GP20)
spi = busio.SPI(board.GP10, MOSI=board.GP11, MISO=board.GP8)
rfm9x = adafruit_rfm9x.RFM9x(
    spi, CS, RESET, RADIO_FREQ_MHZ, baudrate=1000000, agc=False, crc=True
)
rfm9x.tx_power = 5

# configure watchdog
w.timeout = 5
w.mode = WatchDogMode.RESET
w.feed()

while True:
    msg = yellow("Waiting for LoRa packet ...")
    print(f"{msg}\r", end="")
    packet = rfm9x.receive(w, with_header=True, timeout=10)

    if packet is not None:
        # print(packet)
        if packet[:3] == (b"<\xaa\x01"):
            rawdata = bytes(packet[3:]).decode("utf-8")
            name, setport, strsetatt = rawdata.split("/", 3)
            setatt = round_to_half(float(strsetatt))
            print(
                purple(
                    "PORT REQ: Name: "
                    + name
                    + " Port: "
                    + setport
                    + " Attenuator: "
                    + str(setatt)
                )
            )

            if name == config.name:
                for _number, port in ports.items():
                    port.value = False
                try:
                    att(str(setatt))
                    ports[str(int(setport))].value = True
                    print(purple("PORT REQ: Turned port " + str(int(setport)) + " on"))
                except:
                    print(
                        purple(
                            "PORT REQ: Wrong Port NR Turned default port "
                            + str(int(config.default_port))
                            + " on"
                        )
                    )
                    for _number, port in ports.items():
                        port.value = False
                    ports[str(int(config.default_port))].value = True
            else:
                print(
                    yellow("Received another switch port req packet: " + str(rawdata))
                )
        else:
            print(yellow("Received an unknown packet: " + str(packet)))

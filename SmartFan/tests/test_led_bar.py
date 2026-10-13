"""Standalone LED test using the project's remapped Orin Nano BOARD pins."""
import argparse
import time

parser = argparse.ArgumentParser(description=__doc__)
# LED1..LED8; retain the four original pins that do not conflict.
DEFAULT_LED_PINS = [7, 12, 19, 16, 21, 23, 37, 31]
parser.add_argument("--pins", type=int, nargs=8, default=DEFAULT_LED_PINS,
                    help="Override LED1..LED8 BOARD pins (default: %(default)s)")
args = parser.parse_args()
pins = args.pins
# Existing sensor, encoder, motor direction and common PWM header pins.
reserved = {11, 13, 15, 18, 22, 29, 32, 33, 36}
gpio_pins = {7, 11, 12, 13, 15, 16, 18, 19, 21, 22, 23, 24,
             26, 29, 31, 32, 33, 35, 36, 37, 38, 40}
if len(set(pins)) != 8 or not set(pins) <= gpio_pins or set(pins) & reserved:
    parser.error("Choose eight distinct GPIO pins unused by the fan devices.")

import Jetson.GPIO as GPIO

GPIO.setmode(GPIO.BOARD)
initialized = False
try:
    GPIO.setup(pins, GPIO.OUT, initial=GPIO.LOW)
    initialized = True
    for level in (2, 5, 8):
        for index, pin in enumerate(pins):
            GPIO.output(pin, index < level)
        time.sleep(1.5)
finally:
    try:
        if initialized:
            GPIO.output(pins, GPIO.LOW)
    finally:
        GPIO.cleanup(pins)

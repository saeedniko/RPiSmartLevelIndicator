import time
import RPi.GPIO as GPIO
from luma.core.interface.serial import i2c
from luma.oled.device import sh1107
from luma.core.render import canvas
from PIL import ImageFont

serial = i2c(port=1, address=0x3C)
device = sh1107(serial, rotate=0, height=128, width=128)
font = ImageFont.load_default()

GPIO.setmode(GPIO.BCM)
GPIO.setup(17, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(27, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(22, GPIO.IN, pull_up_down=GPIO.PUD_UP)

def read_distance():
    try:
        with open("/proc/hc_sr04_distance", "r") as f:
            distance = f.read().strip()
            return float(distance) if distance else None
    except Exception as e:
        print(f"Error reading distance: {e}")
        return None

def calculate_percentage(water_height, max_height):
    if water_height is not None and max_height > 0:
        return ((max_height - water_height) / max_height) * 100
    return 0

def adjust_value(initial_value, step, min_value, max_value, prompt_text):
    value = initial_value
    while True:
        with canvas(device) as draw:
            draw.text((10, 10), prompt_text, font=font, fill="white")
            draw.text((10, 30), f"Value: {value:.2f}", font=font, fill="white")
        
        if not GPIO.input(17):
            value = min(value + step, max_value)
            time.sleep(0.2)
        elif not GPIO.input(27):
            value = max(value - step, min_value)
            time.sleep(0.2)
        elif not GPIO.input(22):
            time.sleep(0.2)
            return value

def display_distance_and_visual_percentage(max_height, diameter):
    while True:
        distance = read_distance()
        if distance is not None:
            if distance < 10:
                distance_in_meters = 0
            else:
                distance_in_meters = (distance - 10) / 100.0
            
            percentage_full = calculate_percentage(distance_in_meters, max_height)
            percentage_full = max(0, min(percentage_full, 100))
            
            radius = diameter / 2
            volume = 3.14159 * (radius ** 2) * (max_height - distance_in_meters) * 1000
            volume = max(0, volume)
            
            visual_height = int((percentage_full / 100) * 100)
            visual_height = max(0, min(visual_height, 100))

            with canvas(device) as draw:
                draw.text((10, 0), f"Tank: {percentage_full:.2f}%", font=font, fill="white")
                draw.text((10, 13), f"Vol: {volume:.2f}L", font=font, fill="white")

                draw.rectangle((10, 25, 118, 128), outline="white", width=1)
                draw.rectangle((12, 126 - visual_height, 116, 127), fill="blue")
        else:
            with canvas(device) as draw:
                draw.text((10, 10), "Error: No data", font=font, fill="white")

        time.sleep(1)

try:
    max_height = adjust_value(1.0, 0.1, 0.1, 2.0, "Set max height (m):")
    diameter = adjust_value(0.5, 0.1, 0.1, 2.0, "Set diameter (m):")
    display_distance_and_visual_percentage(max_height, diameter)
except KeyboardInterrupt:
    print("Program interrupted")
finally:
    GPIO.cleanup()

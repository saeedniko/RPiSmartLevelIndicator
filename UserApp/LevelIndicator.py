import time
from luma.core.interface.serial import i2c
from luma.oled.device import sh1107
from luma.core.render import canvas
from PIL import ImageFont

serial = i2c(port=1, address=0x3C)
device = sh1107(serial, rotate=0, height=128, width=128)

font = ImageFont.load_default()

def read_distance():
    try:
        with open("/proc/hc_sr04_distance", "r") as f:
            distance = f.read().strip()
            
            return float(distance) if distance else None
            
    except Exception as e:
        print(f"Error reading distance: {e}")
        return None

def calculate_percentage(water_height):
    if water_height is not None:
        percentage_full = (100 - water_height)
        return percentage_full
    return 0

def display_distance_and_visual_percentage():
    while True:
        distance = read_distance()

        if distance is not None:
            percentage_full = calculate_percentage(distance)

            visual_height = int((percentage_full / 100) * 100)

            visual_height = max(0, min(visual_height, 100))

            with canvas(device) as draw:
                draw.text((10, 10), f"Dist: {distance:.2f} cm", font=font, fill="white")
                draw.text((10, 30), f"Tank: {percentage_full:.2f}%", font=font, fill="white")

                draw.rectangle((10, 40, 118, 140), outline="white", width=1)
                draw.rectangle((12, 140 - visual_height, 116, 140), fill="blue")

        else:
            with canvas(device) as draw:
                draw.text((10, 10), "Error: No data", font=font, fill="white")

        time.sleep(1)

display_distance_and_visual_percentage()

import time
from luma.core.interface.serial import i2c
from luma.oled.device import sh1107
from luma.core.render import canvas
from PIL import ImageFont

# Initialize I2C connection for the OLED
serial = i2c(port=1, address=0x3C)  # Ensure this is the correct I2C address
device = sh1107(serial, rotate=0, height=128, width=128)

# Load the default font for text
font = ImageFont.load_default()

# Function to read the distance (height of water) from /proc/hc_sr04_distance
def read_distance():
    try:
        with open("/proc/hc_sr04_distance", "r") as f:
            # Read the numeric value directly
            distance = f.read().strip()
            
            # Convert the value to float (assuming it's a valid number)
            return float(distance) if distance else None
            
    except Exception as e:
        print(f"Error reading distance: {e}")
        return None

# Function to calculate the percentage of the tank that is full
def calculate_percentage(water_height):
    # Since 100cm is full, calculate percentage based on the current distance
    if water_height is not None:
        percentage_full = (100 - water_height)  # Higher distance means lower percentage
        return percentage_full
    return 0

# Function to display the distance, tank percentage, and visual representation on the OLED
def display_distance_and_visual_percentage():
    while True:
        # Read the current distance value
        distance = read_distance()

        if distance is not None:
            # Calculate the percentage of the tank that is full
            percentage_full = calculate_percentage(distance)

            # Calculate the height of the visual representation (100 pixels for 100% tank)
            visual_height = int((percentage_full / 100) * 100)  # Fill the rectangle based on percentage

            # Ensure the visual height is within bounds (0 to 100 pixels)
            visual_height = max(0, min(visual_height, 100))

            # Clear the screen and display the distance, percentage, and visual representation
            with canvas(device) as draw:
                # Display the distance value on the first line
                draw.text((10, 10), f"Dist: {distance:.2f} cm", font=font, fill="white")
                # Display the percentage on the second line
                draw.text((10, 30), f"Tank: {percentage_full:.2f}%", font=font, fill="white")

                # Draw the visual representation of the tank (filled rectangle)
                # The rectangle will be filled based on the percentage of the tank that is full
                draw.rectangle((10, 40, 118, 140), outline="white", width=1)  # Tank outline
                draw.rectangle((12, 140 - visual_height, 116, 140), fill="blue")  # Water level

        else:
            # If no distance data is available, show an error message
            with canvas(device) as draw:
                draw.text((10, 10), "Error: No data", font=font, fill="white")

        # Wait for a short time before updating again
        time.sleep(1)  # Update every 1 second

# Run the function to display distance, percentage, and visual representation
display_distance_and_visual_percentage()

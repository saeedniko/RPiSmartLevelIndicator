import os
import time

ULTRASONIC_DEVICE = "/dev/ultrasonic"

def read_distance():
    with open(ULTRASONIC_DEVICE, 'r') as dev:
        distance_str = dev.read().strip()
        return int(distance_str)

def main():
    print("Ultrasonic Measurement App")

    try:
        while True:
            distance = read_distance()
            print(f"Distance: {distance} cm")
            time.sleep(1)
    except KeyboardInterrupt:
        print("Measurement stopped by user")

if __name__ == "__main__":
    main()

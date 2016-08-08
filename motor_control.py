from __future__ import print_function
import sys
from pololu_drv8835_rpi import motors

set_Speed = int(sys.argv[1])

if set_Speed > 100 or set_Speed < 0:
	sys.exit("Invalid speed input! Only 0-100 accepted")

int_List = [0, 4, 9, 14, 19, 24, 28, 33, 38, 43, 48, 52, 57, 62, 67,\
	72, 76, 81, 86, 91, 96, 100, 105, 110, 115, 120, 124, 129, 134,\
	139, 144, 148, 153, 158, 163, 168, 172, 177, 182, 187, 192, 196,\
	201, 206, 211, 216, 220, 225, 230, 235, 240, 244, 249, 254, 259, 264,\
	268, 273, 278, 283, 288, 292, 297, 302, 307, 312, 316, 321, 326, 331, 336,\
	340, 345, 350, 355, 360, 364, 369, 374, 379, 384, 388, 393, 398, 403, 408, 412,\
	417, 422, 427, 432, 436, 441, 446, 451, 456, 460, 465, 470, 475, 480]

try:
	motors.motor1.setSpeed(int_List[set_Speed])
		
except Exception as e:
	print("Error")

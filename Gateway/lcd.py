#!/usr/bin/python
from time import sleep,strftime
from subprocess import *
from datetime import datetime
from Adafruit_CharLCD import Adafruit_CharLCD

# instantiate lcd and specify pins
lcd = Adafruit_CharLCD(rs=21, en=20, d4=26, d5=19, d6=13, d7=6, cols=16, lines=2)

lcd.clear()
# display text on LCD display \n = new line
lcd.message('-----')

cmd = "ip addr show eth0 | grep inet | awk '{print $2}' | cut -d/ -f1 "

def runCommand(cmd):
	p = Popen(cmd, shell=True,stdout = PIPE)
	output = p.communicate()[0]
	return output

try:
	while True:
		lcd.clear()
		ipAddr = runCommand(cmd)
		lcd.message(datetime.now().strftime('%b %d  %H:%M:%S\n'))
		lcd.message("IP %s" % (ipAddr))
		try:
			sleep(1)
		except KeyboardInterrupt as e:
			print "sleep broken"
			exit()
except Exception as e:
	print "shutting down..."
	exit()
finally:
	print "bye."

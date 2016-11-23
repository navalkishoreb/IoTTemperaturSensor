#!/usr/bin/python

import serial
import config_uart as uart
import config_rootPath as path
import os

from sets import Set

sensorDataPath = path.ROOT_PATH + path.SENSOR_DATA
sensorIdSet = Set()
sizeOfSensorSet = len(sensorIdSet)

def getUARTSerial():
	return serial.Serial(
	    port=uart.PORT,
	    baudrate = uart.BAUD_RATE,
	    parity=serial.PARITY_NONE,
	    stopbits=serial.STOPBITS_ONE,
	    bytesize=serial.EIGHTBITS,
	    timeout=0
	    ) 

def openSerial():
	try:
		print "creating UART serial..."
		ser = getUARTSerial()
		print "opening UART port..."
		ser.open()
		print "uart port opened."
		return ser
	except Exception as e:
		print "Problem in uart."
		print str(e)

def printInHexFormat(binData):
        for ch in binData:
            print "0x"+getHex(ch),
        print("")

def getHex(inBytes):
	if len(inBytes) == 1:
		return "%0*X"%(2,ord(inBytes))

def getSensorId(binSensorId):
	if len(binSensorId) == 4:
		sensorId =""
		for byte in binSensorId:
			sensorId = sensorId + getHex(byte)
		return sensorId
	else:
		print "sensor id in not in correct format"
			
def validate(input):
	status = len(input) != 0
	status = status and len(input) == 8
	status = status and input != "\n"
	return status
	
def parseInput(input):
	try:
		parsedData = {"mfcId":None,"sensorId":None,"value":None}
		parsedData["mfcId"] = input[0:2]
		parsedData["sensorId"] = input[2:6]
		parsedData["value"]= input[6:8]
		return parsedData
	except Exception as e:
		print "Problem in parsing data..."
		print str(e)
		
def addSensorId(sensorId):
	try:	
		if sensorId not in sensorIdSet:
			print "adding sensor to set"
			sensorIdSet.add(sensorId)
		else:
			print "_\r",
	except Exception as e:
		print "Problem in adding sensorId to set"
		print str(e)

def hasSensorSetModified():
	global sizeOfSensorSet
	status = sizeOfSensorSet < len(sensorIdSet)
	return status

def updateSensorSetSize():
	global sizeOfSensorSet
	sizeOfSensorSet = len(sensorIdSet)
		
def writeData(sensorId):
	try:	
		with open(sensorDataPath,"a+") as sensorData:
			sensorData.write(sensorId+",,\n")
			print "Writing sensor "+sensorId
			sensorData.close()
	except Exception as e:
		print "Problem in writing data."
		print str(e)

def removePreviousSensorData():
	try:
		if os.path.isfile(sensorDataPath):
			os.remove(sensorDataPath)
			print "sensorData deleted."
		else:
			print "No sensorData found."
	except Exception as e:
		print "Problem while removing sensor data"
		print str(e)

def readSerialData():
	try:
		ser = openSerial()
		if ser:
			removePreviousSensorData()
			while True:
				try:
					data = ser.readline()
					if validate(data):
						parsedData = parseInput(data)
						_sensorId = parsedData["sensorId"]
						_formattedSensorId = getSensorId(_sensorId)
						addSensorId(_formattedSensorId)
						if hasSensorSetModified():
							updateSensorSetSize()
							writeData(_formattedSensorId)
					else:
						print "_\r",
				except KeyboardInterrupt as key:
					print "shutting down.."
					exit()
	except Exception as e:
			print "Problem in reading serial"
			print str(e)
			

def init():
	readSerialData()	
			
if __name__ == "__main__":
	init()
			

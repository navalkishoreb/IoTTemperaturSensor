#!/usr/bin/python
import time
import serial
import socket
import requests
import sys
import paho.mqtt.client as mqtt
import config_broker as broker
import threading
import config_rootPath as path
from multiprocessing import Process
import os
threadLock = threading.Lock()
client = mqtt.Client()

sensorDataPath = path.ROOT_PATH + path.SENSOR_DATA
credentialsPath = path.ROOT_PATH + path.CREDENTIALS

"""usb = serial.Serial(
    port='/dev/ttyACM0',\
    baudrate=115200,\
    parity=serial.PARITY_NONE,\
    stopbits=serial.STOPBITS_ONE,\
    bytesize=serial.EIGHTBITS,\
    timeout=0)
"""

def getUARTSerial():
	return serial.Serial(
	    port='/dev/ttyAMA0',
	    baudrate = 9600,
	    parity=serial.PARITY_NONE,
	    stopbits=serial.STOPBITS_ONE,
	    bytesize=serial.EIGHTBITS,
	    timeout=0
	    )


def on_connect(client, userdata, flags,resultCode):
	print("connected with result code "+str(resultCode))
	if resultCode == 0:
		threadLock.release()
	else:
		print "Not connecting. Authentication failed."
#	client.publish("room/temperature/status","online",2,True)

def on_message(client, userdata, messsage):
	print(message.topic+" : "+str(message.payload))


def printInHexFormat(binData):
	for ch in binData:
		print '0x%0*X'%(2,ord(ch)),
	print("")

def sendTemperatureToServer(temp):
	r=requests.get("http://api.thingspeak.com/update?key=9W55W474GLEBNBLC&field1="+str(ord(temp)))
	print 'sending...'+ str(r.status_code) +" -- "+ str(r.json())



def publishTemperature(topic,temp):
	try:
		client.publish(topic,str(temp))
	except Exception as e:
		print "Problem while publishing temperature"
		print str(e)


def fetchSensorData():
#	print "fetching sensor and topic data..."
        sensorTopicDict = {}
        try:
                with open(sensorDataPath,"r") as sensorData:
                        for line in sensorData:
                                data = line.rstrip("\n").split(",")
                                sensorTopicDict[data[0]] = data[1]
#                print sensorTopicDict
        except Exception as e:
                print "problem while reading saved sensor data"
                print str(e)
	finally:
		return sensorTopicDict	 


def getSensorId(data):
	sensorId = ""
	try:
		if(len(data) ==4):
			for ch in data:
				sensorId = sensorId + "%0*X"%(2,ord(ch))
			print "Sensor Id: "+sensorId
		else:
			print "data for sensorId not in correct format"
	except Exception as e:
		print "error while fetching sensorId"
		print str(e)
	finally:
		return sensorId

def getTemperatureFromData(data):
	temperature = 0.0
	try:
		if len(data)==2 :
			temperature = (((0xFF & ord(data[0:1]))<<8) | (0xFF & ord(data[1:2])))/10.0
			print "Temperature: "+str(temperature)
		else:
			print "data not in appropriate form"
	except Exception as e:
		print "error occured while fetching temperature"
		print str(e)
	finally:
		return temperature


def displayReceivedData(data):
	try:	
		print "_____________________Received Data___________________________"
                print "Manufacture Id: ",
                printInHexFormat(data[0:2])
                print "Sensor Id: ",
                printInHexFormat(data[2:6])
                print "Temperature Data: ",
		temperature = getTemperatureFromData(data[6:8])
		print str(temperature) + " degree Celsius"
		
	except Exception as e:
		print "There is some problem while printing received data."
		print str(e)
	finally:
		print "---------------------x-x-x----------------------------"
	

	

def getAssociatedTopic(sensorId):
	topic = ""
	try:
		sensorTopicDict = fetchSensorData()
		if sensorId in sensorTopicDict:
			topic = sensorTopicDict[sensorId]
			print "Associated topic: "+topic
		else:
			print "sensorId not found."
	except Exception as e:
		print "error while fetching associated topic."
		print str(e)
	finally:
		return topic

def startObserving():
	try: 	
		print "wating client to connect..."
		threadLock.acquire()
		sensorTopicDict = fetchSensorData()
		print "creating UART serial..."
		ser = getUARTSerial()
		print "opening UART serial..."
		ser.open()
		print "serial port opened."
		newLine = None
		print "waiting to read data..."
		while True:
			data= ser.readline()
			if(data != '') and (data !="\n") and len(data)==8:
				displayReceivedData(data)
#				sendTemperatureToServer(data[-1:])
				sensorId = getSensorId(data[2:6])
				topic = getAssociatedTopic(sensorId)
				if topic:
					temperature = getTemperatureFromData(data[6:8])
					publishTemperature(topic,temperature)
					newLine = True;
				else:
					print "please, associate topic to this sensor."
			elif(data == '\n'):
				print ("")			
			else:
				if newLine:
					newLine = False;
	except KeyboardInterrupt:
		print "shutting down..."
	finally:
		print "bye."



def fetchCredentials():
	credentials= open(credentialsPath,'r')
	data = credentials.read().rstrip("\n").split(",")
	return data
	 

def  init():
        try:
		client.on_connect = on_connect
		client.on_message = on_message
		data = fetchCredentials()
		_username = data[0]
		_password = data[1]
		if _username and _password:
			print "Username: "+_username,
			print "Password: "+_password
			client.username_pw_set(_username,_password)
	                client.connect(broker.MQTT_HOST,broker.PORT,broker.KEEP_ALIVE)
			print "starting client loop..."
			client.loop_start()
			threadLock.acquire()
			startObserving()
        	        
		else:
			print "credentials file exists, but contains no data"
			exit()
        except Exception as e:
                print "Not able to connect to broker."
		print str(e)	

def info(title):
	print(title)
	print('module name:', __name__)
	print('parent process:', os.getppid())
	print('process id:', os.getpid())

if __name__ == "__main__":
	init()

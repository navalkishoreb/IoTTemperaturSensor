from AWSIoTPythonSDK.MQTTLib import AWSIoTMQTTClient
import serial
import config_broker as config
import config_rootPath as path
import json

sensorDataPath = path.ROOT_PATH + path.SENSOR_DATA
credentialsPath = path.ROOT_PATH + path.CREDENTIALS

message = {"value":0,
        "sensorId":"UNKNOWN",
        "topic": "/room/kitchen/temp",
        "sensor_type": "TEMP",
        "user_id":1902890023
        }

print "\ncreating iot client with id: "+config.CLIENT_ID
client = AWSIoTMQTTClient(config.CLIENT_ID)

print "\n-------------configuration--------------"
print "ca_certs: %r" % (config.CA_PATH)
print "certfile: %r" % config.CERT
print "keyfile: %r" % config.KEY
print "host: "+config.HOST
print "port: "+str(config.PORT)
print "-----------------------------------------"

client.configureEndpoint(config.HOST,config.PORT)
client.configureCredentials(config.CA_PATH,config.KEY,config.CERT)
print "AWS IOT client is set."

def publishTemperature(topic,temp,sensorId):
	try:
			message["value"] = temp
			message["sensorId"] = sensorId
                        print "submitting value..."
                        result = client.publish(config.TOPIC, json.dumps(message), config.QOS)
                        if result:
                                print "published successfully."
                        else:
                                print "message not published"
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


def printInHexFormat(binData):
	for ch in binData:
		print '0x%0*X'%(2,ord(ch)),
	print("")

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

def getUARTSerial():
	return serial.Serial(
	    port='/dev/ttyAMA0',
	    baudrate = 9600,
	    parity=serial.PARITY_NONE,
	    stopbits=serial.STOPBITS_ONE,
	    bytesize=serial.EIGHTBITS,
	    timeout=0
	)

def startObserving():
	try: 	
		print "wating client to connect..."
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
				sensorId = getSensorId(data[2:6])
				topic = getAssociatedTopic(sensorId)
				if topic:
					temperature = getTemperatureFromData(data[6:8])
					publishTemperature(topic,temperature,sensorId)
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



def  init():
        try:		
			client.connect()
			startObserving()
        except Exception as e:
                print "Not able to connect to broker."
		print str(e)	



if __name__ == "__main__":
	init()

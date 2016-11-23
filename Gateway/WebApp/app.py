#!/usr/bin/python
from flask import Flask, render_template, request, json, redirect, session, send_from_directory, url_for
from flaskext.mysql import MySQL
from flask_script import Manager
from hashing_passwords import make_hash,check_hash
import config_mysql
import serial_read as publisher
import os.path
import os
import subprocess
import atexit

app = Flask(__name__)
app.secret_key = "Hello_this_is_a_secret_key"

app.config['SEND_FILE_MAX_AGE_DEFAULT'] = 10
mysql = MySQL()
manager = Manager(app)

app.config['MYSQL_DATABASE_USER']=config_mysql.USER
app.config['MYSQL_DATABASE_PASSWORD'] = config_mysql.PASSWORD
app.config['MYSQL_DATABASE_DB']= config_mysql.DATABASE
app.config['MYSQL_DATABASE_HOST']=config_mysql.HOST
app.config['MYSQL_DATABASE_PORT'] = config_mysql.PORT


mysql.init_app(app)

PATH_TO_CREDENTIALS = os.path.join(app.root_path,"userData/credentials")
PATH_TO_SENSOR_DATA = os.path.join(app.root_path,"userData/sensorData")





def fetchCredentials():
	result={"username":None,"password":None}
	try:
		with open(PATH_TO_CREDENTIALS,"r") as credentials:
			data = credentials.read().rstrip("\n").split(",")
			result["username"] = data[0]
			result["password"] = data[1]
			credentials.close()
		print result
	except Exception as e:
		print str(e)
	except IOError as io:
		print str(io)
	finally:
		return result

@app.route("/")
def main():
	credentials = fetchCredentials()
	_username = credentials["username"]
	_password = credentials["password"]
	loginResponse = performLogin(_username,_password)
	if loginResponse["error"]:
		return render_template("index.html")
	else:
		setSession(loginResponse)
		return redirect("/userHome")

@app.route("/favicon.ico")
def favicon():
	return send_from_directory(os.path.join(app.root_path,"static"),"favicon.ico")

@app.route("/showSignUp")
def showSignUp():
	return render_template("signup.html")

@app.route("/showSignIn")
def showSignIn():
	return render_template("signin.html")


def showErrorPage(errorMessage):
	return render_template("error.html",error=errorMessage)

@app.route("/signUp",methods=["POST"])
def signup():
	
	try:	
		_username = request.form["inputName"]
		_password = request.form["inputPassword"]
		print ('serving signup request...')
				
		if _username and _password:
			print "Username["+_username+"] password["+_password+"]"
			conn = mysql.connect()
			cursor = conn.cursor()
			_hashed_password = make_hash(_password)
			cursor.callproc('sp_createUser',(_username,_hashed_password,0))
			data = cursor.fetchall()
			print data

			if len(data) is 0:
				conn.commit()
				return redirect("/showSignIn")
			#	return json.dumps({'message':'User created successfully'})
			else:
				return showErrorPage("Username already exists!!")
#				return json.dumps({'error':str(data[0])})
			conn.close()
		else:
			return showErrorPage("Enter required field")
#			return json.dumps({'html':'<span>Enter the required fields</span>'})
	except Exception as e:
		return showErrorPage(str(e))
#		return json.dumps({'error':str(e)})


def saveUserCredentials(username,password):
	print "saving user credentials..."
	try:
		with open(PATH_TO_CREDENTIALS,"w+") as file:
			file.write(username +","+password)
			file.close()
		print "credential saved at "+file.name
	except Exception as e:
		print str(e)
	except IOError as io:
		print str(io)
		
		
def performLogin(username,password):
	result = {"error":True,"errorMessage":"Unknown error","user":0,"username":""}
	try:
		if username and password:
			print "performing login operation: USER["+username+"] Password["+password+"]"
			conn = mysql.connect()
			cursor = conn.cursor()
			cursor.callproc('sp_validateLogin',(username,))
			data = cursor.fetchall()		
		
			if len(data)>0:
				_storedPassword = data[0][2]
				_storedUserId = data[0][0]
				if check_hash(password,_storedPassword):
					 result["error"]= False
					 result["user"] = _storedUserId
					 result["username"] = username
					 result["errorMessage"] = ""
				else:
					result["errorMessage"]= "Username or Password is wrong!!"
			else:
				result["errorMessage"]="Username not found!!"
				
			conn.close()
		else:
			result["errorMessage"] = "Invalid data!!"
	
	except Exception as e:
		print "error while logging in."
		print str(e)
		result["errorMessage"] = str(e)	
	finally:
		print result
		return result	
		

def setSession(loginResponse):
	session["user"] = loginResponse["user"]
	session["username"] = loginResponse["username"]

@app.route("/validateLogin",methods=["POST"])
def validateLogin():
	print "validating user..."
	
	try:
		_username = request.form["inputName"]
		_password = request.form["inputPassword"]
		result = performLogin(_username,_password)		
		if result["error"]:
			return render_template("error.html",error=result["errorMessage"])     
		else:
			setSession(result)
			saveUserCredentials(_username,_password)
			return redirect("/userHome")						
	except Exception as e:
		print "Exception"
		return render_template("error.html",error=str(e))
		

@app.route("/userHome" , methods=["GET"])
def userHome():
	if session.get("user"):
		print "showing userHome"
		return render_template("userHome.html")
	else:
		return render_template("error.html",error="Unauthorized access")


@app.route("/showAddSensor",methods=["GET"])
def showAddSensor():
	if session.get("user"):
		return render_template("addSensor.html");
	else:
		return render_template("error.html",error="Unauthorized access")


def saveSensorData(_sensorId,_sensorTopic,_readWrite):
	print "saving sensor data...",
	print " sensorId: ",
	print _sensorId,
	print " topic: ",
	print _sensorTopic,
	print " readWrite: ",
	print _readWrite
	try:
		with open(PATH_TO_SENSOR_DATA, 'a') as file:
			file.write(_sensorId+","+_sensorTopic+","+_readWrite+"\n");
			file.close()
	except:
		print "error occured"	

@app.route("/addSensor",methods=["POST"])
def addSensor():
	if session.get("user"):
		try:
			_sensorId = request.form["sensorId"]
			_sensorTopic = request.form["sensorTopic"]
			_readWrite = request.form["readWrite"]	
			if _sensorId and _sensorTopic:
				print "sensorId: "+_sensorId+" sensorTopic: "+_sensorTopic
				conn = mysql.connect()	
				cursor = conn.cursor()
				_username = session.get("username")
				cursor.callproc('sp_insertacl',(_username,_sensorTopic,_readWrite))
				data = cursor.fetchall()
				if len(data) is 0:
					conn.commit()
					saveSensorData(_sensorId,_sensorTopic,_readWrite)
					return redirect("/userHome")
				else:
					return render_template("error.html",error="some error occurred")
	
			else:
				return render_template("error.html",error = "Not valid data")	
	
		except Exception as e:
			print "exception "+str(e)
			return render_template("error.html",error = str(e))		
		finally:
			cursor.close()
			conn.close()
		
	
	else:
		return render_template("error.html", error = "Unauthorized access")


def getSensorDataFromCloud():
	print "fetching sensor data from cloud..."
	topicDict = []
	_username = session.get("username")
	try:
		conn = mysql.connect()
		cursor = conn.cursor()
                cursor.callproc('sp_getTopics',(_username,))
                topics = cursor.fetchall()
                topicDict =[]
                for topic in topics:
	                topicObject = {
				"sensorId":"",
                                "topic":topic[0],
                                "readWrite":topic[1]
                                }
                        topicDict.append(topicObject)
	except:
		print "error fetching from cloud"
	print "Cloud Data: ",
	print topicDict
	return topicDict

def getSavedSensorData():
	print "fetching saved sensor data..."
	topicDict = []
	try:
		with open(PATH_TO_SENSOR_DATA,'r') as file:
			for line in file:
				data =line.split(',')
				topicObject = {
						"sensorId":data[0],
						"topic":data[1],
						"readWrite":data[2]
						}
				topicDict.append(topicObject)
	except Exception as e:
		print str(e)

	print "Local Data: ",
	print topicDict
	return topicDict


def compareSensorData():
	print "comparing cloud and local data..."
	topicFromCloud = getSensorDataFromCloud()
	topicFromLocal = getSavedSensorData()
	flag = False
			
	
	if tuple(topicFromCloud) == tuple(topicFromLocal):
		print "Correct sensor data"
		return topicFromLocal			
	else:
		print "There is some insconsistencies in data between cloud and local"
	
@app.route("/getTopics", methods=["GET"])
def getTopics():
	print "fetching topics..."
	try:
		print "user id "+str(session.get("user"))
		if session.get("user"):
			topicDict = getSavedSensorData()
			return json.dumps(topicDict)
		else:
			print "user is not authorized"
			return json.dumps({"error":"Unauthorized access"})
	except Exception as e:
		print "exception occured: "+str(e)
		return json.dumps({"error":str(e)})
	#finally:
		#cursor.close()
		#conn.close()



def removeCredentials():
	if os.path.isfile(PATH_TO_CREDENTIALS):
		os.remove(PATH_TO_CREDENTIALS)
		print "credentials removed."
	else:
		print "There is no credentials file."
		
def removeSensorData():
	if os.path.isfile(PATH_TO_SENSOR_DATA):
		os.remove(PATH_TO_SENSOR_DATA)
		print "sensor data removed."
	else:
		print "There is no sensor data."	
			
			
@app.route("/logout", methods=["GET"])
def logout():	
	try:
		print "logging out..."
		session.pop("user",None)
		removeCredentials()
		removeSensorData()		
		return redirect("/")
	except Exception as e:
		print "problem in logging out"
		print str(e)

@app.route("/launchPublisher", methods=["POST"])
def launchPublisher():
	if session.get("user") and os.path.isfile(PATH_TO_SENSOR_DATA):
		print "launching publisher..."
		try:
			print "creating command..."
			command = "screen -d -m -S serialUART -t serialUART python "+os.path.join(app.root_path,"serial_read.py")
			print command
			print "forking subprocess..."
			p = subprocess.Popen(command,shell=True)
			print "publisher launched."
			atexit.register(p.kill)
			print "subprocess id "+str(p.pid)
			return json.dumps({"response":"publisher launched"})
		except Exception as e:
			print "Error on launching publisher: "+str(e)
			return	json.dumps({"error":str(e)})
	else:
		print "either user or sensor data not available."
		return json.dumps({"error":"there is some problem"})

if __name__ == "__main__":
	try:
#		app.run(debug = True)
		manager.run()
	except Exception as e:
		print "Error occured "+str(e)
	finally:
		print "quitting web framework..."

from flask import Flask, render_template
import paho.mqtt.client as mqtt 
import subprocess
from datetime import datetime

app = Flask(__name__)
mqttBroker ="0.0.0.0" 

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    if(msg.topic == "/connect"):
        subprocess.run(["systemctl", "stop", "HighSpeedAlliedVision.service"])
        subprocess.run(["systemctl", "start", "HighSpeedAlliedVision.service"])
        client.publish("/LOG", "Connecting to camera...")
        print("Connecting...")

def on_disconnect(client, userdata, rc):
    print("Disconnection detected... Try to connect")
    attemptMQTTConnection(client)

def attemptMQTTConnection(client):
    try: 
        client.connect(mqttBroker, port=1883, keepalive=60)
    except:
        attemptMQTTConnection(client)
   
def on_connect(client, userdata, flags, rc):
    if rc==0:
        print("connected OK")
        client.subscribe("/connect")
        print("subscribed OK")
    else:
        print("Bad connection Returned code=",rc)
client = mqtt.Client("dfn"+datetime.now().strftime("%H:%M:%S"))
client.on_message = on_message
client.on_disconnect = on_disconnect
client.on_connect = on_connect
client.connect(mqttBroker, port=1883, keepalive=60)
client.loop_start()

@app.route('/')
def index():
    return render_template('index.html')

if __name__ == '__main__':
    print("Attemping connection")

    app.run(host='0.0.0.0', port=1000, debug=False)
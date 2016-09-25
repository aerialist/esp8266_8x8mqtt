#Majority of MQTT code taken from
#https://github.com/adafruit/io-client-python/blob/master/examples/mqtt_client.py

from __future__ import absolute_import, division, print_function
from builtins import (bytes, str, open, super, range,
                      zip, round, input, int, pow, object)

from Adafruit_IO import MQTTClient
import pyowm

# Set to your Adafruit IO key & username below.
ADAFRUIT_IO_KEY      = ''
ADAFRUIT_IO_USERNAME = ''  # See https://accounts.adafruit.com
                                                    # to find your username.
# Set OpenWehatherMap API key
OPENWEATHERMAP_API_KEY = ''

# above keys are acturally defined in secret.py
from secret import *

# Define callback functions which will be called when certain events happen.
def connected(client):
    # Connected function will be called when the client is connected to Adafruit IO.
    # This is a good place to subscribe to feed changes.  The client parameter
    # passed to this function is the Adafruit IO MQTT client so you can make
    # calls against it easily.
    print('Connected to Adafruit IO!  Listening for matrix_debug messages...')
    # Subscribe to changes on a feed named DemoFeed.
    client.subscribe('matrix_debug')

def disconnected(client):
    # Disconnected function will be called when the client disconnects.
    print('Disconnected from Adafruit IO!')
    #sys.exit(1)
    #TODO: handle disconnection

def message(client, feed_id, payload):
    # Message function will be called when a subscribed feed has a new value.
    # The feed_id parameter identifies the feed, and the payload parameter has
    # the new value.
    print('Feed {0} received new value: {1}'.format(feed_id, payload))


# Create an MQTT client instance.
client = MQTTClient(ADAFRUIT_IO_USERNAME, ADAFRUIT_IO_KEY)

# Setup the callback functions defined above.
client.on_connect    = connected
client.on_disconnect = disconnected
client.on_message    = message

# Connect to the Adafruit IO server.
client.connect()

# Now the program needs to use a client loop function to ensure messages are
# sent and received.  There are a few options for driving the message loop,
# depending on what your program needs to do.

# The first option is to run a thread in the background so you can continue
# doing things in your program.
client.loop_background()
# Now send new values every 10 seconds.
#print('Publishing a new message every 10 seconds (press Ctrl-C to quit)...')
#while True:
#    value = random.randint(0, 100)
#    print 'Publishing {0} to DemoFeed.'.format(value)
#    client.publish('DemoFeed', value)
#    time.sleep(10)


# work with Open Weather Map
owm = pyowm.OWM(OPENWEATHERMAP_API_KEY)

place_name = 'Urayasu,JP'
place_id = 1849186 # Urayasu

# Current observation
obs = owm.weather_at_place(place_name)
obs = owm.weather_at_id(place_id)

w = obs.get_weather()
w.get_weather_icon_name()

# forecast
# Query for 3 hours weather forecast for the next 5 days over London
fc = owm.three_hours_forecast(place_name)
fc = owm.three_hours_forecast_at_id(place_id)

f = fc.get_forecast()
ws = f.get_weathers()
w = ws[2] # 0:0-2 hours later, 1: 3-5 hours later, 2: 6-8 hours later

def getIconData(icon_name):
    if len(icon_name) != 3:
        raise ValueError
    if icon_name[0:2] in [u'01', u'02']:
        # Sunny
        print("It's sunny.")
        return '08221C551C220800'
    elif icon_name[0:2] in [u'03', u'04']:
        # Cloudy
        print("It's cloudy.")
        return '0018264399660000'
    elif icon_name[0:2] in [u'09', u'10', u'11', u'13', u'50']:
        # Rainy
        print("It's rainy.")
        return '0814225508082810'
    else:
        raise ValueError

# cmd 0x00 Test commands
def cmdTest_display_faceSmiley():
    client.publish('matrix_command', '0000')

def cmdTest_display_faceNeutral():
    client.publish('matrix_command', '0001')

def cmdTest_display_faceFrown():
    client.publish('matrix_command', '0002')

def cmdTest_display_allFullBrightness():
    # good for current draw test
    client.publish('matrix_command', '00FE')

def cmdTest_display_standby():
    # Set 8x8 LED matrix to default standby state; turn on only one LED at
    # lower right corner, set brightness to 5, set blink rate to 3.
    client.publish('matrix_command', '00FF')

def cmdTest_display_weather():
    # show rain icon at default brightness and blink rate
    client.publish('matrix_command', '00FD')

def cmdTest_display_A():
    client.publish('matrix_command', '00FC')


# cmd 0x01 display icon commands
def cmdDisplayIcon(icon_data):
    # Send arbitrary icon display command
    #aio.send('matrix_command', '0108221C551C22080000')
    msg = '01'+icon_data+'00'
    client.publish('matrix_command', msg)
    print('published: {}'.format(msg))

# cmd 0x02 Text scrolling commands
def cmdTextScroll(message):
    # mosquitto_pub -t DEVICEID/feeds/matrix_command -m "02Max 17 characters"
    # max characters are not 17 anymore as it's not BLE
    msg = '02' + message
    client.publish('matrix_command', msg)
    print('published: {}'.format(msg))

# cmd 0x03 Text scrolling control commands
def cmdTextScroll_setting(speed=25, repeat=3):
    # Speed 0 to 255
    # repeat 0 to 255
    msg = '03' + '{:02X}'.format(speed) + '{:02X}'.format(repeat)
    client.publish('matrix_command', msg)
    print('published: {}'.format(msg))

def cmdTextScroll_set_repeat():
    pass

# cmd 0xFF setting commands
def cmdSetting_set_brightness(brightness):
    # brightness: integer value from 0 to 15
    # 0: dimmest, 15: brightest
    if (brightness>=0 and brightness<=15):
        command = 'FF00' + '0' + hex(brightness)[-1].upper()
        client.publish('matrix_command', command)

def cmdSetting_set_rotation(rotation):
    # rotation: integer value from 0 to 3
    if (rotation>=0 and rotation<=3):
        command = 'FF01' + '0' + hex(rotation)[-1].upper()
        client.publish('matrix_command', command)

def cmdSetting_set_blinkRate(blink):
    # blink: integer value from 0 to 3
    # 0x00: blink off (always on)
    # 0x01: 2Hz fast 0x02: 1Hz
    # 0x03: 0.5Hz slow
    if (blink>=0 and blink<=3):
        command = 'FF02' + '0' + hex(blink)[-1].upper()
        client.publish('matrix_command', command)

icon_name = w.get_weather_icon_name() # u'03n', u'02n'
icon_data = getIconData(icon_name)
cmdSetting_set_blinkRate(0)
cmdDisplayIcon(icon_data)
#client.disconnect()

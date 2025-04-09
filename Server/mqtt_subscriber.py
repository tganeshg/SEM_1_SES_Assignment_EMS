import sqlite3
import json
import configparser
from flask import Flask, render_template, jsonify
from flask_mqtt import Mqtt
from flask_socketio import SocketIO
import plotly.express as px
import pandas as pd

# Read configuration
config = configparser.ConfigParser()
config.read('config.ini')

# MQTT Configuration
mqtt_broker = config['MQTT']['broker']
mqtt_port = int(config['MQTT']['port'])
mqtt_username = config['MQTT'].get('username', None)
mqtt_password = config['MQTT'].get('password', None)
mqtt_topic = config['MQTT']['topic']

# Database Configuration
db_name = config['DATABASE']['name']

# Web Configuration
web_host = config['WEB']['host']
web_port = int(config['WEB']['port'])

# Initialize Flask app
app = Flask(__name__)
app.config['MQTT_BROKER_URL'] = mqtt_broker
app.config['MQTT_BROKER_PORT'] = mqtt_port
app.config['MQTT_KEEPALIVE'] = 60
app.config['MQTT_TLS_ENABLED'] = False

if mqtt_username and mqtt_password:
    app.config['MQTT_USERNAME'] = mqtt_username
    app.config['MQTT_PASSWORD'] = mqtt_password

mqtt = Mqtt(app)
socketio = SocketIO(app)

# Initialize SQLite database
def init_db():
    conn = sqlite3.connect(db_name)
    cursor = conn.cursor()
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS SensorData (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            sensorID INTEGER,
            power INTEGER,
            timestamp TEXT
        )
    ''')
    conn.commit()
    conn.close()

init_db()

# MQTT message handler
@mqtt.on_message()
def handle_mqtt_message(client, userdata, message):
    data = json.loads(message.payload.decode())
    conn = sqlite3.connect(db_name)
    cursor = conn.cursor()
    for entry in data:
        cursor.execute('''
            INSERT INTO SensorData (sensorID, power, timestamp)
            VALUES (?, ?, ?)
        ''', (entry['sensorID'], entry['power'], entry['Timestamp']))
    conn.commit()
    conn.close()
    socketio.emit('update', data)

# Web routes
@app.route('/')
def index():
    return render_template('index.html')

@app.route('/current_values')
def current_values():
    conn = sqlite3.connect(db_name)
    cursor = conn.cursor()
    cursor.execute('''
        SELECT sensorID, power, timestamp
        FROM SensorData
        WHERE timestamp = (SELECT MAX(timestamp) FROM SensorData WHERE sensorID = SensorData.sensorID)
    ''')
    data = cursor.fetchall()
    conn.close()
    return jsonify(data)

@app.route('/line_graph')
def line_graph():
    conn = sqlite3.connect(db_name)
    df = pd.read_sql_query('SELECT * FROM SensorData', conn)
    conn.close()
    df['sensorID'] = df['sensorID'].map({1: 'Fan', 2: 'Air Conditioner', 3: 'Refrigerator'})
    fig = px.line(df, x='timestamp', y='power', color='sensorID', title='Power Consumption Over Time')
    return fig.to_html()

@app.route('/bar_graph')
def bar_graph():
    conn = sqlite3.connect(db_name)
    df = pd.read_sql_query('''
        SELECT sensorID, AVG(power) as avg_power, strftime('%Y-%m-%d %H:00:00', timestamp) as hour
        FROM SensorData
        GROUP BY sensorID, hour
    ''', conn)
    conn.close()
    df['sensorID'] = df['sensorID'].map({1: 'Fan', 2: 'Air Conditioner', 3: 'Refrigerator'})
    fig = px.bar(df, x='hour', y='avg_power', color='sensorID', barmode='group', title='Average Power Consumption Per Hour')
    return fig.to_html()

if __name__ == '__main__':
    mqtt.subscribe(mqtt_topic)
    socketio.run(app, host=web_host, port=web_port)
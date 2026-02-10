import paho.mqtt.client as mqtt
import ssl
import time
import json

# --- CONFIGURARE ---
HIVE_BROKER = "broker.hivemq.com"
AWS_ENDPOINT = "" 

TOPIC_TELEMETRY = "factory/line1/telemetry"
TOPIC_COMMANDS = "factory/line1/commands"

CA_PATH = "root-CA.pem"
CERT_PATH = "certificate.pem.crt"
KEY_PATH = "private.pem.key"

# --- CLIENT AWS ---
aws_client = mqtt.Client(client_id="Python_Bridge_Server")

def on_aws_connect(client, userdata, flags, rc):
    if rc == 0:
        print("\n [SISTEM] Conectat cu succes la AWS IoT Core!")
        client.subscribe(TOPIC_COMMANDS)
        print(f" [SISTEM] Subscris la topicul de comenzi AWS: {TOPIC_COMMANDS}")
    else:
        print(f" [EROARE] Conectare AWS eșuată. Cod: {rc}")

def on_aws_message(client, userdata, message):
    # Această funcție procesează mesajele venite de la Lambda
    payload = message.payload.decode()
    
    print("\n" + "="*50)
    print(f" [AWS -> BRIDGE] Semnal primit de la Lambda!")
    print(f" Mesaj: {payload}")
    
    # RETRANSMISIE către Wokwi via HiveMQ
    hive_client.publish(TOPIC_COMMANDS, payload)
    
    print(f" [BRIDGE -> WOKWI] Comandă retransmisă către simulator.")
    print("="*50 + "\n")

aws_client.on_connect = on_aws_connect
aws_client.on_message = on_aws_message

aws_client.tls_set(
    ca_certs=CA_PATH,
    certfile=CERT_PATH,
    keyfile=KEY_PATH,
    cert_reqs=ssl.CERT_REQUIRED,
    tls_version=ssl.PROTOCOL_TLSv1_2
)

# --- CLIENT HIVE (WOKWI) ---
hive_client = mqtt.Client(client_id="Bridge_Subscriber")

def on_hive_message(client, userdata, message):
    # Această funcție procesează telemetria venită de la ESP32
    payload = message.payload.decode()
    
    print(f" [WOKWI -> BRIDGE] Date senzori primite.")
    
    # RETRANSMISIE către AWS
    aws_client.publish(TOPIC_TELEMETRY, payload)
    
    print(f" [BRIDGE -> AWS] Telemetrie trimisă pentru analiză și stocare.")

hive_client.on_message = on_hive_message

# --- LOGICA DE PORNIRE ---
print("\n" + "*"*40)
print("  PORNIRE BRIDGE BIDIRECȚIONAL IIOT  ")
print("*"*40)

try:
    aws_client.connect(AWS_ENDPOINT, 8883)
    aws_client.loop_start() 

    hive_client.connect(HIVE_BROKER, 1883)
    hive_client.subscribe(TOPIC_TELEMETRY)
    
    print(" Bridge activ. Aștept fluxul de date...")
    hive_client.loop_forever() 
except Exception as e:
    print(f" [EROARE FATALĂ] Sistemul s-a oprit: {e}")
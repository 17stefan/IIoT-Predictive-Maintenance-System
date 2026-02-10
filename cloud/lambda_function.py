import json
import boto3
from boto3.dynamodb.conditions import Key
from statistics import mean

# Inițializăm clienții AWS
dynamodb = boto3.resource('dynamodb')
sns = boto3.client('sns')
iot_client = boto3.client('iot-data', region_name='us-east-1')

# Configurații resurse
table = dynamodb.Table('Factory_Telemetry')
SNS_TOPIC_ARN = 'arn:aws:sns:us-east-1:316956664897:Sistem_Alerta_Mentenanta'

def lambda_handler(event, context):
    # 1. Extragem datele curente trimise de ESP32 prin IoT Core
    device_id = event.get('device_id', 'ESP32_PLANT_01')
    current_temp = float(event['telemetry']['temp'])
    
    # 2. Interogăm DynamoDB pentru istoricul acestui motor
    # Luăm ultimele 10 înregistrări pentru a calcula media "normală"
    response = table.query(
        KeyConditionExpression=Key('device_id').eq(device_id),
        Limit=10,
        ScanIndexForward=False 
    )
    items = response.get('Items', [])
    
    # Avem nevoie de un istoric minim pentru a fi predictivi
    if len(items) >= 3:
        past_temps = [float(i['temperature']) for i in items if 'temperature' in i]
        avg_temp = mean(past_temps)
        
        print(f"Device: {device_id} | Temp Curenta: {current_temp} | Media Istorica: {avg_temp:.2f}")

        # 3. LOGICA DE ANALIZĂ: Dacă temperatura crește cu 10% față de medie
        # Chiar dacă e sub limita de 80°C, o creștere bruscă indică uzură.
        if current_temp > (avg_temp * 1.10):
            reason_msg = f"Detectata crestere termica neobisnuita ({current_temp}C vs medie {avg_temp:.2f}C)."
            
            # A. Trimitem E-mail de mentenanță prin SNS
            sns.publish(
                TopicArn=SNS_TOPIC_ARN,
                Subject=f"MENTENANTA PREDICTIVA - {device_id}",
                Message=f"ALERTA: {reason_msg} Sistemul a fost oprit preventiv pentru inspectie."
            )
            
            # B. Trimitem comanda de STOP înapoi la Wokwi
            command = {
                "action": "STOP",
                "reason": "ANOMALIE_CLOUD",
                "avg_history": round(avg_temp, 2)
            }
            
            iot_client.publish(
                topic='factory/line1/commands',
                qos=1,
                payload=json.dumps(command)
            )
            
            return {"status": "ANOMALY_DETECTED", "action": "STOP_SENT"}

    return {"status": "NORMAL_OPERATION", "temp": current_temp}
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>

// ==========================================
// CONFIGURARE AWS IOT
// ==========================================
const char* MQTT_TOPIC_PUB = "factory/line1/telemetry";
const char* MQTT_TOPIC_SUB = "factory/line1/commands";

// Variabila globala pentru controlul sistemului
bool sistemActiv = true; 

// Certificatele raman neschimbate
const char* AWS_CERT_CA = R"KEY()KEY";

const char* AWS_CERT_CRT = R"KEY()KEY";

const char* AWS_CERT_PRIVATE = R"KEY()KEY";

// ==========================================
// HARDWARE SETUP
// ==========================================
const int DHT_PIN = 15;
const int POT_PIN = 34;
const int LED_PIN = 2;
const int TRIG_PIN = 5; 
const int ECHO_PIN = 18;

DHTesp dht;
WiFiClient espClient; 
PubSubClient client(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);

unsigned long lastMsg = 0;

// NOTE: This device connects to HiveMQ (Public Broker) as a simulation gateway.
// The Python Bridge handles the secure TLS 1.2 connection to AWS IoT Core.
void setup_wifi() {
  delay(10);
  lcd.clear();
  lcd.print("Connecting WiFi");
  WiFi.begin("Wokwi-GUEST", "", 6); 
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  lcd.clear();
  lcd.print("WiFi Connected!");
  Serial.println("\nWiFi conectat!");
}

// ==========================================
// CALLBACK: RECEPTIE COMENZI AWS
// ==========================================
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("\n[!] Mesaj primit de la AWS pe topicul de comenzi");

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print("Eroare la parsarea JSON: ");
    Serial.println(error.c_str());
    return;
  }

  const char* action = doc["action"];

  if (action != nullptr && String(action) == "STOP") {
    // 1. Oprim bucla de trimitere date
    sistemActiv = false; 
    
    // 2. Actiune vizuala LED
    digitalWrite(LED_PIN, HIGH); 
    
    // 3. Update LCD cu mesajul cerut
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("OPRIT DIN AWS");
    lcd.setCursor(0, 1);
    lcd.print("MENTENANTA RECOM");
    
    Serial.println("###################################");
    Serial.println("#   SISTEM OPRIT DE AWS CLOUD     #");
    Serial.println("#   ANOMALIE PREDICTIVA DETECTATA #");
    Serial.println("###################################");
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Incercare MQTT HiveMQ...");
    String clientId = "ESP32_Plant_01_";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("Conectat!");
      client.subscribe(MQTT_TOPIC_SUB); // Ne abonam la topicul de comenzi
    } else {
      Serial.print("Eroare, rc=");
      Serial.print(client.state());
      Serial.println(" Reincercare in 5 secunde...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  dht.setup(DHT_PIN, DHTesp::DHT22);
  lcd.init();
  lcd.backlight();
  
  setup_wifi();
  client.setServer("broker.hivemq.com", 1883); 
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // Trimitem date DOAR daca sistemul este ACTIV
  if (sistemActiv) {
    unsigned long now = millis();
    if (now - lastMsg > 5000) { 
      lastMsg = now;

      // 1. Citire Senzori
      TempAndHumidity data = dht.getTempAndHumidity();
      int potValue = analogRead(POT_PIN);
      int vibrationLevel = map(potValue, 0, 4095, 0, 100); 

      // 2. Senzor Ultrasonic (Mentenanta)
      digitalWrite(TRIG_PIN, LOW);
      delayMicroseconds(2);
      digitalWrite(TRIG_PIN, HIGH);
      delayMicroseconds(10);
      digitalWrite(TRIG_PIN, LOW);
      long duration = pulseIn(ECHO_PIN, HIGH);
      int distance = duration * 0.034 / 2; 

      // Health Score (400mm max distanta conform cerintei)
      int healthScore = map(constrain(distance, 2, 400), 2, 400, 0, 100);

      // 3. Status Local
      String status = "NORMAL";
      if (data.temperature > 80.0) status = "OVERHEAT"; // Max 80 grade cerut

      // 4. Construire JSON
      StaticJsonDocument<512> doc;
      doc["device_id"] = "ESP32_PLANT_01";
      doc["timestamp"] = millis();
      
      JsonObject telemetry = doc.createNestedObject("telemetry");
      telemetry["temp"] = serialized(String(data.temperature, 2));
      telemetry["hum"] = serialized(String(data.humidity, 1));
      telemetry["vibrations"] = vibrationLevel;
      telemetry["maintenance_dist"] = distance;
      telemetry["health_score"] = healthScore;
      
      doc["status"] = status;

      char buffer[512];
      serializeJson(doc, buffer);
      client.publish(MQTT_TOPIC_PUB, buffer);

      // 5. Update LCD Operational
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("T:"); lcd.print(data.temperature, 1);
      lcd.print("C H:"); lcd.print(healthScore); lcd.print("%");
      lcd.setCursor(0, 1);
      lcd.print("Stat: "); lcd.print(status);
      
      Serial.print("Telemetrie trimisa: ");
      Serial.println(buffer);
    }
  } else {
    // Daca sistemul este oprit, LED-ul ramane aprins si nu mai trimitem nimic
    digitalWrite(LED_PIN, HIGH);
  }
}

# IIoT-Predictive-Maintenance-System
This project is a Closed-Loop IIoT Predictive Maintenance system that monitors industrial motor health in real-time. It utilizes AWS Lambda to perform statistical anomaly detection on sensor data, automatically triggering a remote STOP command to prevent hardware failure. 
IIoT Predictive Maintenance & Closed-Loop Control System
This project demonstrates a full-scale Industrial IoT (IIoT) architecture designed for real-time motor health monitoring and autonomous intervention. It bridges the gap between simulated hardware (Edge) and advanced analytics (Cloud) to perform Predictive Maintenance and Remote Actuation.

       Project Overview
The system monitors a simulated industrial motor's temperature, vibration, and wear. Unlike standard systems that use fixed thresholds, this solution uses AWS Lambda to analyze historical data trends. If a significant deviation is detected, the Cloud sends a command back to the device to trigger an emergency stop, preventing hardware failure.
  
       Tech Stack
Edge: ESP32 (Wokwi), DHT22, Ultrasonic HC-SR04, I2C LCD.

Gateway: Bidirectional Python Bridge (MQTT, SSL/TLS).

Cloud (AWS): IoT Core, Lambda, DynamoDB, SNS.

Languages: C++ (Arduino/ESP32), Python (Bridge & Lambda).

       Key Features
1. Predictive Maintenance Logic
Anomaly Detection: The AWS Lambda function queries DynamoDB for the last 10 telemetry records to calculate a historical average.

Smart Thresholds: A "STOP" command is triggered if the current temperature exceeds the historical average by more than 10%, even if it is below the local critical limit.

2. Closed-Loop Feedback (Remote Stop)
Cloud-to-Edge: Upon anomaly detection, Lambda publishes a JSON payload to the factory/line1/commands topic.

Local Intervention: The ESP32 receives the command, halts data transmission, illuminates a red warning LED, and updates the LCD with "STOPPED FROM AWS".

3. Real-Time Alerting
SNS Integration: Automated email notifications are sent to the maintenance team immediately when a predictive anomaly is identified.

      System Architecture
Telemetry Flow: ESP32 ➡️ HiveMQ ➡️ Python Bridge ➡️ AWS IoT Core ➡️ DynamoDB.

Command Flow: AWS Lambda ➡️ AWS IoT Core ➡️ Python Bridge ➡️ HiveMQ ➡️ ESP32.
![IIoT System Demo](iiot-system-demo.gif)

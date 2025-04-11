#include "Wire.h"
#include <HTTPClient.h>
#include <Arduino.h>

#define I2C_SDA 21 //I2C address for ESP32 controller and ESP32 peripheral devices  (GSM)
#define I2C_SCL 22 //I2C address for ESP32 controller and ESP32 peripheral devices  (GSM)
#define SIM7600_RX 16  // ESP32 TX pin connected to SIM7600 RX
#define SIM7600_TX 17  // ESP32 RX pin connected to SIM7600 TX

#define I2C_DEV_GSM 0x54

HardwareSerial simSerial(1); 

bool setupComplete = false;
bool sendComplete = true;

String gpsData = "";
String slopeData = "";
String collisionData = "";
String turningData = "";
String fallData = "";
String encoderData = "";
String url;

uint32_t i = 0;
float pitch, currentMagnitude, roll, sos, gpsLocation, speed, distance;;
const float DIFFERENCE_THRESHOLD = 10.0; // m/s**2
float previousMagnitude = 0; 


const String slopeUrl = "http://45.77.22.181/wheelchair_tracking/h_sendlocation/wh1slope_v2.php"; 
const String collisionUrl = "http://45.77.22.181/wheelchair_tracking/h_sendlocation/wh1collision_v2.php"; 
const String fallUrl = "http://45.77.22.181/wheelchair_tracking/h_sendlocation/wh1FallDtection.php";
const String sosUrl = "http://45.77.22.181/wheelchair_tracking/h_sendlocation/wh1gps_sos.php";
const String encoderUrl = "http://45.77.22.181/wheelchair_tracking/h_sendlocation/wh1encoder.php";
const String gpsUrl = "http://45.77.22.181/wheelchair_tracking/h_sendlocation/wh1gps.php";

enum DataType {
    DATA_PITCH_STATE,
    DATA_COLLISION,
    DATA_FALL,
    DATA_SOS,
    DATA_ENCODER,
    DATA_GPS
};

enum WheelchairState {
    GOING_UP,
    GOING_DOWN,
    LEVEL_GROUND
};

DataType dataType;
WheelchairState currentState = LEVEL_GROUND;

void sendData(DataType dataType); 

String sendATCommandhttp(const char* command, const char* expectedResponse) {
    simSerial.flush(); /
    simSerial.print(command);
    simSerial.print("\r");
    delay(1000); 
    Serial.print("Command: ");
    Serial.println(command);

    unsigned long startTime = millis();
    String response = "";
    while (millis() - startTime < 2000) { 
        while (simSerial.available()) {
            char c = simSerial.read();
            response += c;
        }
        if (response.indexOf(expectedResponse) != -1) {
            Serial.println("Response: " + response);
            return response;
        }
    }
    Serial.println("No response or unexpected response.");
    Serial.println("Response: " + response);
    return "";
}

// Function to send AT command and wait for response
String sendATCommand(const char* command, const char* expectedResponse) {
    simSerial.flush(); 
    simSerial.print(command);
    simSerial.print("\r");
    delay(1000); 
    Serial.print("Command: ");
    Serial.println(command);

    unsigned long startTime = millis();
    String response = "";
    while (millis() - startTime < 1000) {
        while (simSerial.available()) {
            char c = simSerial.read();
            response += c;
        }
        if (response.indexOf(expectedResponse) != -1) {
            Serial.println("Response: " + response);
            return response; 
        }
    }
    Serial.println("No response or unexpected response.");
    Serial.println("Response: " + response);
    return ""; 
}

void onRequest() {
  if (!setupComplete) return; 

  char buffer [20];
  String response = sendATCommand("AT+CGPSINFO", "OK");
  Serial.println(response);

  if (response.indexOf("+CGPSINFO:") != -1) {
    int startIndex = response.indexOf("+CGPSINFO:") + strlen("+CGPSINFO: ");
    Serial.println(startIndex); 
    gpsData = response.substring(startIndex); 
    Serial.println(gpsData);
    String a = gpsData;
    Serial.println(a); 
    Serial.println("gps_data: " + gpsData);
    if (gpsData.equals(",,,,,,,,") || gpsData.equals(" ,,,,,,,,") || gpsData.equals(" ,,,,,,,, ")){
      gpsLocation = 0;
      snprintf(buffer, sizeof(buffer), "%.2f", gpsLocation);
      Wire.write((uint8_t*)buffer, strlen(buffer));
      Serial.print("Buffer:");
      Serial.println(buffer);
      Serial.print(" ");
    } else {
      gpsLocation = 1;
      snprintf(buffer, sizeof(buffer), "%.2f", gpsLocation);
      Wire.write((uint8_t*)buffer, strlen(buffer));
      Serial.print("Buffer:");
      Serial.println(buffer);
      Serial.print(" ");
      dataType = DATA_GPS;
      sendData(dataType); 
    }
  } else {
    Serial.println("No valid GPS data to send.");
    gpsLocation = 0;
      snprintf(buffer, sizeof(buffer), "%.2f", gpsLocation);
      Wire.write((uint8_t*)buffer, strlen(buffer));
      Serial.print("Buffer:");
      Serial.println(buffer);
      Serial.print(" ");
  }}

void onReceive(int len) {
    if (!setupComplete) return; 

    Serial.printf("onReceive[%d]: ", len);

    if (len > 0) { 
        char buffer[81]; 
        Wire.readBytes(buffer, len); 
        buffer[len] = '\0'; 
        
        Serial.print("Received string: ");
        Serial.println(buffer);
  
        char* token = strtok(buffer, ",");
        float values[7];
        int i = 0;

        while (token != NULL && i < 7) {
            values[i] = atof(token);
            token = strtok(NULL, ",");
            i++;
        }

        if (i >= 7) { 
            pitch = values[0];
            currentMagnitude = values[1];
            previousMagnitude = values[2];
            roll = values[3];
            sos = values[4];
            distance = values[5];
            speed = values[6];

            for (int j = 0; j < i; j++) {
                Serial.print("Value ");
                Serial.print(j);
                Serial.print(": ");
                Serial.println(values[j], 2);
            }

            Serial.print("Pitch: "); Serial.print(pitch, 2); Serial.println("degree");
            Serial.print("Current Magnitude: "); Serial.println(currentMagnitude, 2);
            Serial.print("Previous Magnitude: "); Serial.println(previousMagnitude, 2);
            Serial.print("Roll:"); Serial.print(roll, 2); Serial.println("degree");
            Serial.print("Encoder Distance"); Serial.print(distance, 2); Serial.println("unit");
            Serial.print("Speed:"); Serial.println(speed, 2);
            Serial.print("Have SOS Alert? "); Serial.println(sos, 2);

            encoderData = String(distance) + "," + String(speed);
            Serial.println("encoderData: " + encoderData);
            dataType = DATA_ENCODER;
            sendData(dataType);

            if ((pitch > 5) && (pitch < 30)) {
                currentState = GOING_UP;
                dataType = DATA_PITCH_STATE;
                slopeData = String(pitch) + "," + String(currentState);
                Serial.println("Going upward");
                sendData(dataType);
            } else if ((pitch < -5) && (pitch > -30)) {
                currentState = GOING_DOWN;
                dataType = DATA_PITCH_STATE;
                slopeData = String(pitch) + "," + String(currentState);
                Serial.println("Going downward");
                sendData(dataType);
            } else {
                currentState = LEVEL_GROUND;
            }

            bool collisionDetected = false;

            if (abs(currentMagnitude - previousMagnitude) > DIFFERENCE_THRESHOLD) {
              if (previousMagnitude!= 0){
                Serial.println("Collision detected!");
                collisionDetected = true; 
              }  
            }

            if (collisionDetected) {
                dataType = DATA_COLLISION;
                collisionData = String(currentMagnitude) + "," + String(previousMagnitude);
                sendData(dataType);
            }

            if ((pitch > 30) || (pitch < -30) || (roll > 30) || (roll < -30)) {
              if ((pitch > 30) || (pitch < -30)){
                Serial.println("Fall alert!!");
                dataType = DATA_FALL;
                fallData = String(pitch);
                Serial.print("fallData"); Serial.println(fallData);
                sendData(dataType);  
              } else {
                Serial.println("Fall alert!!");
                dataType = DATA_FALL;
                fallData = String(roll);
                Serial.print("fallData"); Serial.println(fallData);
                sendData(dataType);
              }
            }

            if (sos == 1) { // Use == for comparison
                Serial.println("SOS ALERT");
                dataType = DATA_SOS;
                sendData(dataType);
            }

            previousMagnitude = currentMagnitude;
        }
    } else {
        Serial.println("NO data received.");
    }

    delay(5000); 
}

void sendData(DataType dataType) {
    if (!sendComplete) return;
    sendComplete = false;
    String url;
    sendATCommandhttp("AT+HTTPINIT", "OK");
    sendATCommandhttp("AT+HTTPPARA=\"CID\",1", "OK");
    delay(2000);

    String postData;
    if (dataType == DATA_PITCH_STATE) {
        postData = "slopeData=" + slopeData;
        url = slopeUrl;
    } else if (dataType == DATA_COLLISION) {
        postData = "collisionData=" + collisionData;
        url = collisionUrl;
    } else if (dataType == DATA_FALL) {
        postData = "fallData=" + fallData;
        url = fallUrl;
    } else if (dataType == DATA_SOS) {
        postData = "gps_data=" + gpsData;
        url = sosUrl;
    } else if (dataType == DATA_GPS) {
        postData = "gps_data=" + gpsData;
        url = gpsUrl;
    } else if (dataType == DATA_ENCODER){
        url = encoderUrl;
        postData = "encoderData=" + encoderData;
    } else {
        Serial.println("Invalid data type.");
        return;
    }

    sendATCommandhttp(("AT+HTTPPARA=\"URL\",\"" + url + "\"").c_str(), "OK");
    sendATCommandhttp(("AT+HTTPPARA=\"CONTENT\",\"" + String("application/x-www-form-urlencoded") + "\"").c_str(), "OK");
    delay(2000);
    sendATCommandhttp(("AT+HTTPDATA=" + String(postData.length()) + ",10000").c_str(), "DOWNLOAD");

    simSerial.print(postData);
    delay(2000);

    String actionResponse = sendATCommandhttp("AT+HTTPACTION=1", "OK");
    delay(2000);
    
    if (actionResponse.indexOf("ERROR") != -1) {
        Serial.println("HTTP action failed.");
    } else {
        Serial.println("HTTP action successful.");
        sendATCommandhttp("AT+HTTPREAD", ""); // Read the HTTP response
    }

    sendATCommandhttp("AT+HTTPTERM", "OK"); // Terminate HTTP service
    sendComplete = true;
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Wire.onReceive(onReceive);
  Wire.onRequest(onRequest);
  //Wire.begin();
  Wire.begin((uint8_t)I2C_DEV_GSM, (int)I2C_SDA, (int)I2C_SCL, (uint32_t)100000);

  simSerial.begin(115200, SERIAL_8N1, SIM7600_RX, SIM7600_TX);
  Serial.println("SIM7600 HTTP Request Test");
    while (sendATCommand("AT", "OK") == "") {
        Serial.println("Retrying AT command...");
        delay(1000); 
    }

    
    sendATCommand("AT+CSQ", ""); 
    sendATCommand("AT+CREG?", ""); 
    sendATCommand("AT+CGPS=1", "OK");
    sendATCommand("AT+NETOPEN", ""); 
    sendATCommand("AT+CGDCONT=1,\"IP\",\"SmarTone\"", "OK"); 
    setupComplete = true; 

}

void loop (){

}

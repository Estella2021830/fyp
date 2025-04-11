#include "Wire.h"
#include <WiFi.h>
#include <HTTPClient.h>

#define I2C_DEV_Indoor 0x57 // ESP32 controller and ESP32 peripheral devices (Indoor tracker)
#define I2C_DEV_Encoder 0x56  // ESP32 controller and ESP32 peripheral devices (encoder)
#define I2C_DEV_ADDR 0x55 // ESP32 controller and ESP32 peripheral devices  (IMU)
#define I2C_DEV_GSM 0x54 //// ESP32 controller and ESP32 peripheral devices (GSM)
#define I2C_SDA 21 // I2C pin
#define I2C_SCL 22 // I2C pin


const char* ssid = "Emi"; // Wi-Fi SSID
const char* password = "xgsc5746"; // Wi-Fi password

//PHP links
const String slopeUrl = "https://10cd-45-64-240-215.ngrok-free.app/wheelchair_tracking/h_sendlocation/wh1slope_v2.php"; 
const String collisionUrl = "https://10cd-45-64-240-215.ngrok-free.app/wheelchair_tracking/h_sendlocation/wh1collision_v2.php"; 
const String fallUrl = "https://10cd-45-64-240-215.ngrok-free.app/wheelchair_tracking/h_sendlocation/wh1FallDtection.php";
const String sosUrl = "https://10cd-45-64-240-215.ngrok-free.app/wheelchair_tracking/h_sendlocation/wh1gps_sos.php";
const String encoderUrl = "https://10cd-45-64-240-215.ngrok-free.app/wheelchair_tracking/h_sendlocation/wh1encoder.php";

uint32_t i = 0;
float pitch, currentMagnitude, roll, sos, speed, distance;
const float DIFFERENCE_THRESHOLD = 10.0; // m/s**2
float previousMagnitude = 0; 
const int buttonPin = 25; 
volatile unsigned long buttonPressTime = 0;
volatile bool buttonPressed = false;

String gpsData = "0,0,0,0";
String slopeData = "";
String collisionData = "";
String fallData = "";
String encoderData = "";
String tag_id;

unsigned long lastGpsRequestTime = 0;
unsigned long lastIndoorRequestTime = 0;

enum DataType {
    DATA_PITCH_STATE,
    DATA_COLLISION,
    DATA_FALL,
    DATA_SOS,
    DATA_ENCODER
};

enum WheelchairState {
    GOING_UP,
    GOING_DOWN,
    LEVEL_GROUND
};


DataType dataType;
WheelchairState currentState = LEVEL_GROUND;

void connectToWiFi() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.print("Connecting to WiFi...");
        WiFi.begin(ssid, password);

        for (int attempts = 0; attempts < 3; attempts++) {
            
            delay(1000);
            Serial.print(".");

            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("Connected to WiFi!");
                return; 
            }
        }

        Serial.println("Failed to connect to WiFi after 3 attempts. Sending data via GSM.");
        
    }
}

void sendDataUsingWifi(uint8_t dataType) {
    connectToWiFi();
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        if (dataType == DATA_PITCH_STATE) {
            http.begin(slopeUrl);
        } else if (dataType == DATA_COLLISION) {
            http.begin(collisionUrl);
        } else if (dataType == DATA_FALL) {
            http.begin(fallUrl);
        } else if (dataType == DATA_SOS){
            http.begin(sosUrl);
        } else if (dataType == DATA_ENCODER){
          http.begin(encoderUrl);
        } else {
            Serial.println("Invalid URL.");
            return; 
        }

        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        String postData;
        if (dataType == DATA_PITCH_STATE) {
            postData = "slopeData=" + slopeData;
        } else if (dataType == DATA_COLLISION) {
            postData = "collisionData=" + collisionData;
        } else if (dataType == DATA_FALL) {
            postData = "fallData=" + fallData;
        } else if (dataType == DATA_SOS) {
            postData = "gps_data=" + gpsData; 
        } else if (dataType == DATA_ENCODER) {
            postData = "encoderData=" + encoderData;
            Serial.println("encoderData: " + encoderData);
            
        } else {
            Serial.println("Invalid postData.");
            return; 
        }
        
        int maxRetries = 3; 
        int attempts = 0;
        int httpResponseCode;

        do {
            httpResponseCode = http.POST(postData);
            attempts++;

            if (httpResponseCode > 0) {
                String response = http.getString();
                Serial.println(httpResponseCode);
                Serial.println(response);
                break; 
            } else {
                Serial.print("Error on sending POST: ");
                Serial.println(httpResponseCode);
                delay(2000); 
            }
        } while (httpResponseCode == -1 && attempts < maxRetries);

        http.end();
    } else {
        Serial.println("WiFi Disconnected");
    }
}

void requestIndoor (){
    if (millis() - lastIndoorRequestTime < 60000) {
        Serial.println("Indoor request skipped: less than one minute since last request.");
        return; 
    }
    uint8_t bytesReceived = Wire.requestFrom(I2C_DEV_Indoor, 20);
    Serial.printf("requestFrom: %u\n", bytesReceived);  
    if ((bool)bytesReceived) { 
      char buffer[21]; 
      Wire.readBytes(buffer, bytesReceived); 
      buffer[bytesReceived] = '\0'; 
      Serial.print("Received bytes: ");
      for (uint8_t i = 0; i < bytesReceived; i++) {
          Serial.print(" ");
      }
      Serial.println(); 
      char filteredBuffer[21] = {0};
      uint8_t j = 0;

      for (uint8_t i = 0; i < bytesReceived; i++) {
          if (buffer[i] >= 0 && buffer[i] <= 127) { // Check if it's a valid ASCII character
              filteredBuffer[j++] = buffer[i];
          }
      }
      filteredBuffer[j] = '\0'; 

      Serial.println(filteredBuffer); 
      float indoor = atof(filteredBuffer);
      if (indoor == 1) {
        Serial.print("Indoor location received?"); Serial.println(indoor);
        Serial.println("Received indoor location data.");
      } else {
        Serial.print("Indoor location received"); Serial.println(indoor);
        Serial.println("No indoor location data received.");
      }
  } else {
        Serial.println("NO data received.");
  }
  
  lastIndoorRequestTime = millis();
}

void requestIMU (){
  uint8_t bytesReceived = Wire.requestFrom(I2C_DEV_ADDR, 80);
    Serial.printf("requestFrom: %u\n", bytesReceived);  

    if ((bool)bytesReceived) {      
      char buffer[81];
      Wire.readBytes(buffer, bytesReceived);
      buffer[bytesReceived] = '\0'; 
  
      Serial.print("Received string: ");
      Serial.println(buffer);
      char* token = strtok(buffer, ",");
      float values[4];
      int i = 0;

      while (token != NULL && i < 4) {
        values[i] = atof(token);
        token = strtok(NULL, ",");
        i++;
      }

      if (i == 4) { 
        pitch = values[0];
        currentMagnitude = values[1];
        previousMagnitude = values[2];
        roll = values[3];
      }

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
    } else {
      Serial.println("NO data received.");
    }
    delay(5000);
}

void requestEncoderData() {
    uint8_t bytesReceived = Wire.requestFrom(I2C_DEV_Encoder, 80);
    Serial.printf("requestFrom: %u\n", bytesReceived);  

    if ((bool)bytesReceived) {        
        char buffer[81];
        Wire.readBytes(buffer, bytesReceived);
        buffer[bytesReceived] = '\0';
        
        Serial.print("Received string: ");
        Serial.println(buffer);

        char *tag_str = strtok(buffer, "|");
        char *distance_str = strtok(NULL, "|");
        char *speed_str = strtok(NULL, "|");

        if (distance_str && speed_str) {
            
            
            char *dist_val = strchr(distance_str, '=') + 1;
            distance = atof(dist_val);
            
            char *speed_val = strchr(speed_str, '=') + 1;
            speed = atof(speed_val);

            //Serial.printf("Tag ID: %s\n", tag_id.c_str());
            Serial.printf("Distance: %.2f m\n", distance);
            Serial.printf("Speed: %.2f m/s\n", speed);
        }
    } else {
        Serial.println("No data received Encoder.");
    }
    delay(5000);
}

void requestGPS() {
  
    if (millis() - lastGpsRequestTime < 60000) {
        Serial.println("GPS request skipped: less than one minute since last request.");
        return; 
    }
  uint8_t bytesReceived = Wire.requestFrom(I2C_DEV_GSM, 20);
  Serial.printf("requestFrom: %u\n", bytesReceived);  

  if ((bool)bytesReceived) {    
      char buffer[21]; 
      Wire.readBytes(buffer, bytesReceived);
      buffer[bytesReceived] = '\0'; 
        
      Serial.print("Received bytes: ");
      for (uint8_t i = 0; i < bytesReceived; i++) {
          Serial.print(" ");
      }
      Serial.println(); 
        
      char filteredBuffer[21] = {0};
      uint8_t j = 0;
      
      for (uint8_t i = 0; i < bytesReceived; i++) {
          if (buffer[i] >= 0 && buffer[i] <= 127) { 
              filteredBuffer[j++] = buffer[i];
          }
      }
      filteredBuffer[j] = '\0'; 

      Serial.println(filteredBuffer); 
      float gps = atof(filteredBuffer); 
      if (gps == 1) {
        Serial.print("GPS received?"); Serial.println(gps);
        Serial.println("Received GPS data.");
      } else {
        Serial.print("GPS received?"); Serial.println(gps);
        Serial.println("No GPS data received.");
      }
  } else {
        Serial.println("NO data received.");
  }
  
  lastGpsRequestTime = millis();
}

void IRAM_ATTR handleButtonPress() {
    if (digitalRead(buttonPin) == HIGH) {
        buttonPressTime = millis(); 
        buttonPressed = true;
    }
}

void checkSOS() {
    if (buttonPressed) {
        if (millis() - buttonPressTime >= 5000) {
            Serial.println("SOS Alert!"); 
            delay(5000);
            sos = 1;
            buttonPressTime = millis(); 
        }
    }
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Wire.begin();
  pinMode(buttonPin, INPUT_PULLDOWN); 
  attachInterrupt(digitalPinToInterrupt(buttonPin), handleButtonPress, RISING); 

}

void loop() {
    delay(5000);

    Wire.beginTransmission(I2C_DEV_Indoor);
    Wire.printf("Hello World! %lu", i++);
    uint8_t error = Wire.endTransmission(true);
    Serial.printf("endTransmission: %u\n", error);
    
    requestIMU();
    checkSOS();

    connectToWiFi();
    if (WiFi.status() == WL_CONNECTED) {
        requestIndoor ();
        requestEncoderData();
        encoderData = String(distance) + "," + String(speed);
        Serial.println("encoderData: " + encoderData);
        dataType = DATA_ENCODER;
        sendDataUsingWifi(dataType);

        if ((pitch > 4) && (pitch < 30)) {
            currentState = GOING_UP;
            dataType = DATA_PITCH_STATE;
            slopeData = String(pitch) + "," + String(currentState);
            sendDataUsingWifi(dataType);
        } else if ((pitch < -4) && (pitch > -30)) {
            currentState = GOING_DOWN;
            dataType = DATA_PITCH_STATE;
            slopeData = String(pitch) + "," + String(currentState);
            sendDataUsingWifi(dataType);
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
            sendDataUsingWifi(dataType);
        }

        if ((pitch > 30) || (pitch < -30) || (roll > 30) || (roll < -30)) {
          if ((pitch > 30) || (pitch < -30)){
            Serial.println("Fall alert!!");
            dataType = DATA_FALL;
            fallData = String(pitch);
            Serial.print("fallData"); Serial.println(fallData);
            sendDataUsingWifi(dataType);  
          } else {
            Serial.println("Fall alert!!");
            dataType = DATA_FALL;
            fallData = String(roll);
            Serial.print("fallData"); Serial.println(fallData);
            sendDataUsingWifi(dataType);
          }
        }

        if (buttonPressed) { 
            dataType = DATA_SOS;
            sendDataUsingWifi(dataType);
        }
        
    } else {
        requestGPS();
        Wire.beginTransmission(I2C_DEV_GSM);
        char buffer[80];
        snprintf(buffer, sizeof(buffer), "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f", pitch, currentMagnitude, previousMagnitude, roll, sos, distance, speed);
        Serial.print("Pitch: "); Serial.print(pitch, 2); Serial.println("degree");
        Serial.print("Current Magnitude: "); Serial.println(currentMagnitude, 2);
        Serial.print("Previous Magnitude: "); Serial.println(previousMagnitude, 2);
        Serial.print("Roll:"); Serial.print(roll, 2); Serial.println("degree");
        Serial.print("SOS Alert? "); Serial.println(sos);
        Serial.print("Encoder Distance"); Serial.print(distance, 2); Serial.println("unit");
        Serial.print("Speed:"); Serial.println(speed, 2);
        Wire.write((uint8_t*)buffer, strlen(buffer));
        Serial.print("Buffer: ");
        Serial.println(buffer);
        uint8_t error = Wire.endTransmission(true);
        Serial.printf("endTransmission: %u\n", error);
    }
    
    previousMagnitude = currentMagnitude;
    buttonPressed = false; 
    sos = 0;
}
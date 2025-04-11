#include "Wire.h"
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

#define I2C_SDA 21 // SDA pin for communication with master
#define I2C_SCL 22 // SCL pin for communication with master
#define ENCODER_SDA 26 // SDA pin for MPU6050 
#define ENCODER_SCL 25 // SCL pin for MPU6050
#define I2C_DEV_ENCODER 0x56 // I2C address for ESP32 master and ESP32 slave 

// Wi-Fi settings
const char* ssid = "lti";      
const char* password = "jutc3065";           

// Wheel parameters
const float WHEEL_DIAMETER_CM = 35.56;
const float WHEEL_CIRCUMFERENCE_M = (WHEEL_DIAMETER_CM * PI) / 100.0;

// for encoder
volatile long encoderCount = 0;
volatile int lastStateA = 0;
volatile int lastStateB = 0;

// for distance and speed calculation
const int PULSES_PER_REVOLUTION = 1000;
const int MULTIPLIER = 4;
unsigned long lastTime = 0;
long lastPosition = 0;
float distance_traveled = 0.0;  
float current_speed = 0.0;      
const float SPEED_SMOOTHING_FACTOR = 0.2;  

const char* tag_id = "tag1";  

// Time settings
const char* ntpServer = "asia.pool.ntp.org"; 
const long gmtOffset_sec = 28800;           
const int daylightOffset_sec = 0; 

unsigned long lastInterruptTimeA = 0;
unsigned long lastInterruptTimeB = 0;
const unsigned long debounceDelay = 5; 

TwoWire I2CENCODER = TwoWire(0); 
TwoWire I2CSlave = TwoWire(1); 

// connect to Wi-Fi
void connectToWiFi() {
    Serial.print("Connecting to Wi-Fi");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println(" Connected to Wi-Fi!");
}

// Interrupt handler for encoder A
void IRAM_ATTR handleEncoderA() {
    unsigned long currentTime = millis();
    if (currentTime - lastInterruptTimeA > debounceDelay) {
        int currentStateA = digitalRead(ENCODER_SDA);
        int currentStateB = digitalRead(ENCODER_SCL);

        if (lastStateA != currentStateA) {
            if (currentStateA != currentStateB) {
                encoderCount += MULTIPLIER;
            } else {
                encoderCount -= MULTIPLIER;
            }
        }

        lastStateA = currentStateA;
    }
    lastInterruptTimeA = currentTime;
}

// Interrupt handler for encoder B
void IRAM_ATTR handleEncoderB() {
    unsigned long currentTime = millis();
    if (currentTime - lastInterruptTimeB > debounceDelay) {
        int currentStateA = digitalRead(ENCODER_SDA);
        int currentStateB = digitalRead(ENCODER_SCL);

        if (lastStateB != currentStateB) {
            if (currentStateA == currentStateB) {
                encoderCount += MULTIPLIER;
            } else {
                encoderCount -= MULTIPLIER;
            }
        }

        lastStateB = currentStateB;
    }
    lastInterruptTimeB = currentTime;
}

// calculate distance and speed
void calculateDistanceAndSpeed() {
    unsigned long currentTime = millis();
    unsigned long timeInterval = currentTime - lastTime;

    if (timeInterval >= 1000) { 
        long positionChange = encoderCount - lastPosition;
        float revolutions = abs(positionChange) / (float)PULSES_PER_REVOLUTION;
        float distance_increment = revolutions * (0.065 * PI /WHEEL_CIRCUMFERENCE_M) * WHEEL_CIRCUMFERENCE_M ;
        distance_traveled += distance_increment;

        float instantaneous_speed = (distance_increment * 1000.0) / timeInterval;
        current_speed = (SPEED_SMOOTHING_FACTOR * instantaneous_speed) + 
                       ((1 - SPEED_SMOOTHING_FACTOR) * current_speed);

        lastTime = currentTime;
        lastPosition = encoderCount;
    }
}

void onRequest() {
    char buffer[80] = {0};  // Initialize the buffer to zero
    calculateDistanceAndSpeed();

    // Format the data string
    snprintf(buffer, sizeof(buffer), "tag_id=%s|distance=%.2f|speed=%.2f", 
             tag_id, distance_traveled, current_speed);
    buffer[sizeof(buffer) - 1] = '\0';  // Null-terminate the buffer

    // Calculate the actual length of the data
    size_t dataLength = strlen(buffer);

    // Debugging output
    Serial.printf("Sending %d bytes: %s\n", dataLength, buffer);

    // Send only the valid portion of the buffer
    I2CSlave.write((uint8_t*)buffer, dataLength);
}

void onReceive(int len) {
    Serial.printf("onReceive[%d]: ", len);
    while (I2CSlave.available()) {
        Serial.write(I2CSlave.read());
    }
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    connectToWiFi();

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time. Setting manual time as fallback.");
    } else {
        Serial.printf("Time synchronized: %02d:%02d:%02d\n",
                      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }

    pinMode(ENCODER_SDA, INPUT_PULLUP);
    pinMode(ENCODER_SCL, INPUT_PULLUP);

    lastStateA = digitalRead(ENCODER_SDA);
    lastStateB = digitalRead(ENCODER_SCL);

    attachInterrupt(digitalPinToInterrupt(ENCODER_SDA), handleEncoderA, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_SCL), handleEncoderB, CHANGE);

    lastTime = millis();

    I2CSlave.onReceive(onReceive);
    I2CSlave.onRequest(onRequest);
    I2CSlave.begin((uint8_t)I2C_DEV_ENCODER, (int)I2C_SDA, (int)I2C_SCL, (uint32_t)100000);

    I2CENCODER.begin(ENCODER_SDA, ENCODER_SCL, 100000);
}

void loop() {
    
}

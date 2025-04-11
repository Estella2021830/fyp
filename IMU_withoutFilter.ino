#include "Wire.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define I2C_SDA 21 // SDA pin for communication with master
#define I2C_SCL 22 // SCL pin for communication with master
#define IMU_SCL 33 // SCL pin for MPU6050 
#define IMU_SDA 32 // SDA pin for MPU6050
#define I2C_DEV_ADDR 0x55 //I2C address for ESP32 controller and ESP32 peripheral devices  

Adafruit_MPU6050 mpu; 
TwoWire I2CMPU = TwoWire(0); // I2C bus for MPU6050
TwoWire I2CSlave = TwoWire(1); // I2C bus for ESP32 controller and ESP32 peripheral devices communication

uint32_t i = 0;
const float DIFFERENCE_THRESHOLD = 10.0; // m/s**2
const float TURN_THRESHOLD = 0.6;
float previousMagnitude = 0;
float pitch, roll, yawRate, currentMagnitude;

void onRequest() {
  char buffer[80];  


  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);


  pitch = atan2(a.acceleration.y, sqrt(a.acceleration.x * a.acceleration.x + a.acceleration.z * a.acceleration.z)) * 180.0 / PI;
  roll = atan2(a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180.0 / PI;// tilt angle in the y-axis in degree
  
  currentMagnitude = sqrt(a.acceleration.x * a.acceleration.x + 
                          a.acceleration.y * a.acceleration.y + 
                          a.acceleration.z * a.acceleration.z);


  
  snprintf(buffer, sizeof(buffer), "%.2f,%.2f,%.2f,%.2f", pitch, currentMagnitude, previousMagnitude, roll);
  Serial.print("Pitch:"); Serial.print(pitch); Serial.println("degree");
  Serial.print("Current Magnitude:"); Serial.println(currentMagnitude);
  Serial.print("Previous Magnitude:"); Serial.println(previousMagnitude);
  Serial.print("Roll:"); Serial.print(roll); Serial.println("degree");

  
  I2CSlave.write((uint8_t*)buffer, strlen(buffer));
  Serial.print("Buffer:");
  Serial.println(buffer);
  Serial.print(" ");

  previousMagnitude = currentMagnitude;
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
  Serial.setDebugOutput(true);
  I2CSlave.onReceive(onReceive);
  I2CSlave.onRequest(onRequest);
  
  I2CSlave.begin((uint8_t)I2C_DEV_ADDR, (int)I2C_SDA, (int)I2C_SCL, (uint32_t)100000);

  I2CMPU.begin(IMU_SDA, IMU_SCL, 100000);
  bool status;

  status = mpu.begin(0x68, &I2CMPU); // I2C address of IMU: 0x68 
    
  
  if (!status) { 
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
      }
  }
  Serial.println("MPU6050 found!");
}

void loop() {
  
}
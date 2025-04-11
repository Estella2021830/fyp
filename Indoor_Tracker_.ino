#include <WiFi.h>
#include <HTTPClient.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <map>
#include <vector>
#include <cmath>

const char* ssid = "LeungSK";
const char* password = "92519664Sk";

const char* server_url = "http://192.168.1.130:5000/upload_data";


#define TARGET_UUID_1 "8ec76ea3-6668-48da-9866-75be8bc86f4d"
#define TARGET_UUID_2 "39652f48-4584-4042-ac01-7d10d48df558"
#define TARGET_UUID_3 "d3ccf3d6-3974-4104-b2ad-a0d9ef45bdff"
#define TARGET_UUID_4 "9626ac99-09b0-4d7c-8554-7d3055842b45"


BLEScan* pBLEScan;

const float RSSI_REFERENCE[4] = { -66.4, -66.4, -62.85, -64.45 };
const float PATH_LOSS_EXPONENT[4] = { 3.94, 3.94, 2.36, 3.33 };
const int MAX_RSSI_SAMPLES = 5;

std::map<String, std::vector<int>> rssiBuffers;
std::map<String, float> beaconDistances;

String getBeaconName(const String& uuid) {
   if (uuid == TARGET_UUID_1) return "beacon1";
   else if (uuid == TARGET_UUID_2) return "beacon2";
   else if (uuid == TARGET_UUID_3) return "beacon3";
   else if (uuid == TARGET_UUID_4) return "beacon4";
   return "";
}


float calculateDistance(const String &beaconName, int rssi) {
   int beaconIndex = -1;
   if (beaconName == "beacon1") beaconIndex = 0;
   else if (beaconName == "beacon2") beaconIndex = 1;
   else if (beaconName == "beacon3") beaconIndex = 2;
   else if (beaconName == "beacon4") beaconIndex = 3;


   if (beaconIndex == -1) return -1;


   return pow(10, (RSSI_REFERENCE[beaconIndex] - rssi) / (10 * PATH_LOSS_EXPONENT[beaconIndex]));
}


float getFilteredDistance(const String &beaconName, int rssi) {
   std::vector<int>& buffer = rssiBuffers[beaconName];
   buffer.push_back(rssi);
   if (buffer.size() > MAX_RSSI_SAMPLES) {
       buffer.erase(buffer.begin());
   }


   float sum = 0;
   for (int val : buffer) {
       sum += val;
   }
   float avgRssi = sum / buffer.size();


   return calculateDistance(beaconName, avgRssi);
}

void setup_wifi() {
   Serial.print("Connecting to WiFi...");
   WiFi.begin(ssid, password);
   while (WiFi.status() != WL_CONNECTED) {
       delay(1000);
       Serial.print(".");
   }
   Serial.println("\nConnected to WiFi");
   Serial.print("ESP32 IP Address: ");
   Serial.println(WiFi.localIP());
}


void sendBeaconData() {
   if (WiFi.status() == WL_CONNECTED) {
       Serial.println("WiFi is connected. Preparing to send data...");


       HTTPClient http;
       http.begin(server_url);
       http.addHeader("Content-Type", "application/json");


       String jsonPayload = "{\"device\":\"ESP32_Tracker\",\"distances\":{";


       bool firstItem = true;
       for (const auto& entry : beaconDistances) {
           if (!firstItem) jsonPayload += ",";
           jsonPayload += "\"" + entry.first + "\":" + String(entry.second, 2);
           firstItem = false;
       }
       jsonPayload += "}}";


       Serial.println("Sending HTTP POST: " + jsonPayload);
       int httpResponseCode = http.POST(jsonPayload);


       if (httpResponseCode > 0) {
           Serial.println("Data sent successfully, Response Code: " + String(httpResponseCode));
           Serial.println("Server Response: " + http.getString());
       } else {
           Serial.println("HTTP POST Failed, Error Code: " + String(httpResponseCode));
       }
       http.end();


       beaconDistances.clear();
   } else {
       Serial.println("WiFi Not Connected, Skipping HTTP POST");
   }
}


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
   void onResult(BLEAdvertisedDevice advertisedDevice) {
       String uuid = advertisedDevice.getServiceUUID().toString().c_str();
       uuid.trim();


       if (uuid == TARGET_UUID_1 || uuid == TARGET_UUID_2 || uuid == TARGET_UUID_3 || uuid == TARGET_UUID_4) {
           String beaconName = getBeaconName(uuid);
           float distance = getFilteredDistance(beaconName, advertisedDevice.getRSSI());
           beaconDistances[beaconName] = distance;


           Serial.println("Detected " + beaconName +
                          ", Filtered Distance: " + String(distance) + "m");
       }
   }
};


void setup() {
   Serial.begin(115200);


   setup_wifi();


   BLEDevice::init("ESP32_Scanner");
   pBLEScan = BLEDevice::getScan();
   pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
   pBLEScan->setActiveScan(true);
   pBLEScan->setInterval(100);
   pBLEScan->setWindow(99);
}


void loop() {
   Serial.println("Starting BLE scan...");
   pBLEScan->start(5, false);
   pBLEScan->stop();


   if (!beaconDistances.empty()) {
       Serial.println("Sending data after scan...");
       sendBeaconData();
   } else {
       Serial.println("No beacons detected. Skipping data send.");
   }


   delay(1000); 
}
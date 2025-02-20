#include <WiFi.h>
#include <Preferences.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
// BLE Service and Characteristic UUIDs
const BLEUUID SERVICE_UUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
const BLEUUID CHAR_UUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");
// BLE Server and Characteristic
BLEServer *pServer = nullptr;
BLECharacteristic *pCharacteristic = nullptr;
Preferences preferences;
String ssid = "";
String password = "";

// Define GPIO pins for the 4-channel relay
const int relayPins[4] = {14, 16, 18, 21}; // Update these pins as per your wiring

// Function to handle relay ON requests
void handleRelayOn(int relay) {
  if (relay >= 0 && relay < 4) {
    digitalWrite(relayPins[relay], LOW);  // Turn relay ON (LOW activates relay)
  }
}
// Function to handle relay OFF requests
void handleRelayOff(int relay) {
  if (relay >= 0 && relay < 4) {
    digitalWrite(relayPins[relay], HIGH);  // Turn relay OFF (HIGH deactivates relay)
  }
}

bool deviceConnected = false;
// Callback: Handle BLE device connection and disconnection
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) override {
      deviceConnected = true;
      Serial.println("[DEBUG] Device connected via BLE.");
    }
    void onDisconnect(BLEServer *pServer) override {
      deviceConnected = false;
      Serial.println("[DEBUG] Device disconnected. Restarting BLE advertising...");
      BLEDevice::startAdvertising();
    }
};

// Callback: Handle BLE write events for Wi-Fi credentials
class MyCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
      String value = pCharacteristic->getValue().c_str();
      Serial.println("[DEBUG] Received Wi-Fi credentials via BLE: " + value);
      // Parse Wi-Fi credentials (Format: "SSID,PASSWORD")
      int separatorIndex = value.indexOf(',');
      if (separatorIndex > 0) {
        ssid = value.substring(0, separatorIndex);
        password = value.substring(separatorIndex + 1);
        // Save credentials to NVS
        preferences.putString("ssid", ssid);
        preferences.putString("password", password);
        // Attempt to connect to Wi-Fi
        Serial.println("[DEBUG] Connecting to Wi-Fi...");
        WiFi.begin(ssid.c_str(), password.c_str());
      } else {
        Serial.println("[ERROR] Invalid format. Expected 'SSID,PASSWORD'.");
      }
    }
};

// Connect to Wi-Fi and wait for connection
void connectToWiFi() {
  Serial.print("[DEBUG] Connecting to Wi-Fi");
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.println(WiFi.localIP());
  }
  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED && retryCount < 20) { // Retry for ~10 seconds
    delay(500);
    Serial.print(".");
    retryCount++;
  }

}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  Serial.println("\n[DEBUG] Starting BLE Wi-Fi Config...");
  // Initialize Preferences (NVS)
  preferences.begin("wifi_creds", false);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  if (!ssid.isEmpty() && !password.isEmpty()) {
    Serial.println("[DEBUG] Found stored Wi-Fi credentials. Connecting...");
    connectToWiFi();
  } else {
    Serial.println("[DEBUG] No Wi-Fi credentials found. Waiting for BLE input...");
  }
  // Initialize BLE
  BLEDevice::init("ESP32_WIFI_Config");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  // Create BLE Service and Characteristic
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHAR_UUID,
                      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
                    );
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());
  // Start the BLE Service and Advertising
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("[DEBUG] BLE advertising started. Waiting for BLE connection...");
}

void loop() {
  connectToWiFi();
  // Log Wi-Fi status periodically
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[DEBUG] Wi-Fi connected. IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("[DEBUG] Wi-Fi not connected.");
  }
  Serial.println("[DEBUG] ssid: " + ssid);
  Serial.println("[DEBUG] password: " + password);
  // Initialize relay pins as outputs
  for (int i = 0; i < 4; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH);  // Start with the relay OFF
  }
  delay(5000); // Avoid spamming the Serial Monitor
}void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}

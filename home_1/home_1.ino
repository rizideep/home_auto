#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <AceButton.h>
#include <Preferences.h>
#include <ArduinoJson.h>

using namespace ace_button;

// BLE Service and Characteristic UUIDs
const BLEUUID SERVICE_UUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
const BLEUUID CHAR_UUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");
// Flask server URL
const char* serverUrl = "https://DeepSingh617.pythonanywhere.com";

#define DEBUG_SW 1
const uint8_t NUM_RELAYS = 8;
const uint8_t relayPins[NUM_RELAYS]  = {5, 18, 19, 21, 22, 23, 25, 26};
const uint8_t switchPins[NUM_RELAYS] = {4 , 12, 13,  14, 15, 27, 32, 33 };
//const uint8_t gpio_reset = 2;

bool toggleStates[NUM_RELAYS] = {0};
ButtonConfig buttonConfigs[NUM_RELAYS];
AceButton buttons[NUM_RELAYS];

BLEServer *pServer = nullptr;
BLECharacteristic *pCharacteristic = nullptr;
Preferences pref;
WebServer server(80);

String ssid = "";
String password = "";
bool newCredentialsReceived = false;
bool isDeviceRegister = false;
bool deviceConnected = false;

String ownerId = "";
String houseId = "";
String floorId = "";
String roomId = "";
String houseName = "";
String floorName = "";
String roomName = "";
String devicesId = "";

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) override {
      deviceConnected = true;
      if (DEBUG_SW) Serial.println("[DEBUG] Device connected via BLE.");
    }
    void onDisconnect(BLEServer *pServer) override {
      deviceConnected = false;
      if (DEBUG_SW) Serial.println("[DEBUG] Device disconnected. Restarting BLE advertising...");
      BLEDevice::startAdvertising();
    }
};

void saveCredentials() {
  pref.putString("ssid", ssid);
  pref.putString("password", password);
  newCredentialsReceived = true;
  if (DEBUG_SW) Serial.println("Wifi credentials saved.");
  pref.end();
}

void addCredentials() {
  pref.putString("devices_name", "Home" + devicesId);
  pref.putString("devices_id", devicesId);
  pref.putString("owner_id", ownerId);
  pref.putString("house_id", houseId);
  pref.putString("floor_id", floorId);
  pref.putString("room_id", roomId);
  pref.putString("house_name", houseName);
  pref.putString("floor_name", floorName);
  pref.putString("room_name", roomName);
  isDeviceRegister = true;
  if (DEBUG_SW) Serial.println("Home data saved.");
  pref.end();
}

class MyCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
      String  value = pCharacteristic->getValue();
      // Parse JSON data
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, value);
      if (error) {
        if (DEBUG_SW)Serial.println("[ERROR] Failed to parse JSON");
        return;
      }
      // Extract key-value pairs
      if (doc.containsKey("wifi_ssid") && doc.containsKey("wifi_pass")    ) {
        if (DEBUG_SW) Serial.println("[DEBUG] Received Wi-Fi credentials via BLE: " + value);
        ssid = doc["wifi_ssid"].as<String>();
        password = doc["wifi_pass"].as<String>();
        saveCredentials();
        if (DEBUG_SW)Serial.println("[INFO] Wifi data received and saved.");
      } else {
        if (DEBUG_SW)Serial.println("[ERROR] Wifi data missing somthing in JSON.");
      }


      // Extract key-value pairs
      if (doc.containsKey("owner_id") && doc.containsKey("device_id") && doc.containsKey("home_name")
          && doc.containsKey("floor_name") && doc.containsKey("room_name")  ) {
        ownerId = doc["owner_id"].as<String>();
        devicesId = doc["device_id"].as<String>();
        houseName = doc["home_name"].as<String>();
        floorName = doc["floor_name"].as<String>();
        roomName = doc["room_name"].as<String>();
        houseId = doc["home_id"].as<String>();
        floorId = doc["floor_id"].as<String>();
        roomId = doc["room_id"].as<String>();
        addCredentials();
        if (DEBUG_SW)Serial.println("[INFO] Home data received and saved.");
      } else {
        if (DEBUG_SW)Serial.println("[ERROR] Home data missing somthing in JSON.");
      }
    }
};


void sendDeviceRegistration() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(String(serverUrl) + "/register_device");
    http.addHeader("Content-Type", "application/json");
    // Create JSON payload
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["devices_name"] = pref.getString("devices_name", "");
    jsonDoc["owner_id"] = pref.getString("owner_id", "");
    jsonDoc["devices_id"] = pref.getString("devices_id", "");
    jsonDoc["house_id"] = pref.getString("house_id", "");
    jsonDoc["floor_id"] = pref.getString("floor_id", "");
    jsonDoc["room_id"] = pref.getString("room_id", "");
    jsonDoc["house_name"] = pref.getString("house_name", "");
    jsonDoc["floor_name"] = pref.getString("floor_name", "");
    jsonDoc["room_name"] = pref.getString("room_name", "");

    String jsonString;
    serializeJson(jsonDoc, jsonString);
    if (DEBUG_SW) {
      Serial.println("[DEBUG] JSON Payload:");
      Serial.println(jsonString);
    }

    // Send POST request
    int httpResponseCode = http.POST(jsonString);

    if (httpResponseCode > 0) {
      String response = http.getString();
      if (DEBUG_SW) {
        Serial.print("[DEBUG] Server Response Code: ");
        Serial.println(httpResponseCode);
        Serial.println("[DEBUG] Server Response: " + response);
      }
      isDeviceRegister = false;
    } else {
      if (DEBUG_SW) {
        Serial.print("[ERROR] HTTP Request failed, code: ");
        Serial.println(httpResponseCode);
        Serial.print("[ERROR] Error Message: ");
        Serial.println(http.errorToString(httpResponseCode).c_str());
      }
    }

    http.end();
  } else {
    if (DEBUG_SW)Serial.println("WiFi not connected.");
  }
}


void buttonHandler(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  if (eventType == AceButton::kEventClicked) {
    int buttonPin = button->getPin();

    String owner_id = pref.getString("owner_id", "");
    String devices_id = pref.getString("devices_id", "");
    String house_id = pref.getString("house_id", "");
    String floor_id = pref.getString("floor_id", "");
    String room_id = pref.getString("room_id", "");

    for (int i = 0; i < NUM_RELAYS; i++) {
      if (buttonPin == switchPins[i]) {
        toggleStates[i] = !toggleStates[i];
        digitalWrite(relayPins[i], toggleStates[i]);
        Serial.printf("Button %d toggled Relay %d\n", (int)(i + 1), (int)(i + 1));
        String switchNo = String(switchPins[i]);  // ✅ FIXED conversion
        updateRelayStateToServer(owner_id, devices_id, house_id, floor_id, room_id, relayPins[i], switchNo, toggleStates[i] );
        break;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  if (DEBUG_SW)Serial.println("setup start >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
  for (int i = 0; i < NUM_RELAYS; i++) {
    pinMode(switchPins[i], INPUT_PULLUP);
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW);
    buttonConfigs[i].setEventHandler(buttonHandler);
    buttons[i].init(switchPins[i]);
  }
  pref.begin("wifi_creds", false);
  ssid = pref.getString("ssid", "");
  password = pref.getString("password", "");
  if (!ssid.isEmpty() && !password.isEmpty()) {
    connectToWiFi();
  }
  bluetoothConfig();
  if (DEBUG_SW) Serial.println("[DEBUG] Setup complete. Waiting for connections...");
  if (WiFi.status() == WL_CONNECTED) {
    server.on("/update", HTTP_GET, handleRelayControl);
    server.on("/wifi", HTTP_POST, handleWiFiCredentials);  // Handle Wi-Fi credentials via /wifi
  }
  // getRelayStateFromServer();  // Fetch relay state from server
  if (DEBUG_SW)Serial.println("setup start >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
}

void bluetoothConfig() {
  BLEDevice::init("BLE_Config");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  pServer->getAdvertising()->setMinInterval(0x20); // Faster connection
  pServer->getAdvertising()->setMaxInterval(0x40);
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHAR_UUID,
                      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
                    );
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  BLEDevice::startAdvertising();
}

void loop() {
  if (DEBUG_SW)Serial.println("loop start >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
  for (int i = 0; i < NUM_RELAYS; i++) {
    buttons[i].check();
  }

  connectToWiFi();
  if (newCredentialsReceived) {
    ESP.restart();
  }
  //  if (digitalRead(gpio_reset) == LOW) {
  //    delay(100);
  //    int startTime = millis();
  //    while (digitalRead(gpio_reset) == LOW) delay(50);
  //    int endTime = millis();
  //    if ((endTime - startTime) > 5000) {
  //      if (DEBUG_SW) Serial.println("Reset to factory settings.");
  //    }
  //  }
  delay(100);
  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  }
  delay(100);
  if (WiFi.status() == WL_CONNECTED) {
    if (isDeviceRegister) {
      sendDeviceRegistration();
    }
  }

  if (DEBUG_SW)Serial.println("loop end >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> ");
}



void handleRelayControl() {
  if (!server.hasArg("owner_id") || !server.hasArg("devices_id") || !server.hasArg("house_id") ||
      !server.hasArg("floor_id") || !server.hasArg("room_id") || !server.hasArg("eqp_no") ||
      !server.hasArg("eqp_name") || !server.hasArg("eqp_state")) {
    server.send(400, "text/plain", "Missing required parameters");
    return;
  }

  String owner_id = server.arg("owner_id");
  String devices_id = server.arg("devices_id");
  String house_id = server.arg("house_id");
  String floor_id = server.arg("floor_id");
  String room_id = server.arg("room_id");
  String eqp_name = server.arg("eqp_name");
  int eqp_no = server.arg("eqp_no").toInt();
  bool eqp_state = server.arg("eqp_state");

  if (eqp_no < 0 || eqp_no >= 8) {
    server.send(400, "text/plain", "Invalid equipment number (eqp_no)");
    return;
  }
  // Control relay based on eqp_state (ON = LOW, OFF = HIGH)
  digitalWrite(relayPins[eqp_no], eqp_state == true ? LOW : HIGH);
  updateRelayStateToServer(owner_id, devices_id, house_id, floor_id, room_id, eqp_no, eqp_name, eqp_state );
}


void handleWiFiCredentials() {
  if (!server.hasArg("ssid") || !server.hasArg("password")) {
    server.send(400, "text/plain", "Missing SSID or password");
    return;
  }
  ssid = server.arg("ssid");
  password = server.arg("password");
  saveCredentials();
  server.send(200, "text/plain", "Wi-Fi credentials saved. Restarting...");
  ESP.restart();
}

void connectToWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    ssid = pref.getString("ssid", "");
    password = pref.getString("password", "");

    if (ssid.isEmpty() || password.isEmpty()) {
      if (DEBUG_SW)Serial.println("creadential not found: ");
    }

    if (!ssid.isEmpty() && !password.isEmpty()) {
      if (DEBUG_SW)Serial.println("ssid: " + ssid);
      if (DEBUG_SW)Serial.println("password: " + password);
      WiFi.begin(ssid, password);
    }
  }

  for (int retryCount = 0; WiFi.status() != WL_CONNECTED && retryCount < 20; retryCount++) {
    Serial.printf("Attempt %d: Connecting...\n", retryCount + 1);
    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) {
    server.begin();
  }
}

void getRelayStateFromServer() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      String response = http.getString();
      if (DEBUG_SW)Serial.println("Relay States from Server: " + response);
    } else {
      if (DEBUG_SW)Serial.print("Error fetching relay states, Code: ");
      if (DEBUG_SW)Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    if (DEBUG_SW)Serial.println("WiFi Disconnected!");
  }
}

void updateRelayStateToServer(String owner_id, String devices_id, String house_id, String floor_id, String room_id, int eqp_no, String eqp_name, bool eqp_state ) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(String(serverUrl) + "/update_eqp");
    http.addHeader("Content-Type", "application/json");

    // Create JSON object
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["owner_id"] = owner_id;
    jsonDoc["devices_id"] = devices_id;
    jsonDoc["house_id"] = house_id;
    jsonDoc["floor_id"] = floor_id;
    jsonDoc["room_id"] = room_id;
    jsonDoc["eqp_no"] = eqp_no;
    jsonDoc["eqp_name"] = eqp_name;
    jsonDoc["eqp_state"] = eqp_state;

    String requestBody;

    serializeJson(jsonDoc, requestBody);
    int httpResponseCode = http.POST(requestBody);
    if (httpResponseCode > 0) {
      String response = http.getString();
      if (DEBUG_SW) Serial.println("Server Response: " + response);
    } else {
      if (DEBUG_SW) Serial.print("Error updating relay state, Code: ");
      if (DEBUG_SW) Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    if (DEBUG_SW) Serial.println("WiFi Disconnected!");
  }
}

#include <WiFi.h>
#include <Preferences.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

Preferences preferences;
String ssid = "";
String password = "";
bool newCredentialsReceived = false;

class BLECallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      int separator = value.find(",");
      if (separator != std::string::npos) {
        ssid = value.substr(0, separator).c_str();
        password = value.substr(separator + 1).c_str();
        newCredentialsReceived = true;
        Serial.println("Received credentials via BLE.");
      }
    }
  }
};

void setupBLE() {
  BLEDevice::init("ESP32_WiFi_Config");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(BLEUUID("12345678-1234-5678-1234-56789abcdef0"));

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      BLEUUID("12345678-1234-5678-1234-56789abcdef1"),
      BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic->setCallbacks(new BLECallbacks());
  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();
}

void connectWiFi() {
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.println("Connecting to Wi-Fi...");
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 10) {
    delay(1000);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to Wi-Fi!");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Failed to connect. Reverting to BLE mode.");
    setupBLE();
  }
}

void saveCredentials() {
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();
  Serial.println("Credentials saved.");
}

bool loadCredentials() {
  preferences.begin("wifi", true);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  preferences.end();

  return (ssid.length() > 0 && password.length() > 0);
}

void setup() {
  Serial.begin(115200);

  if (loadCredentials()) {
    Serial.println("Loaded Wi-Fi credentials from memory.");
    connectWiFi();
  } else {
    Serial.println("No Wi-Fi credentials found. Starting BLE mode.");
    setupBLE();
  }
}

void loop() {
  if (newCredentialsReceived) {
    saveCredentials();
    newCredentialsReceived = false;
    ESP.restart(); // Restart to connect with new credentials
  }
}

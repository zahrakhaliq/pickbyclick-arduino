/*
  Example from WiFi > WiFiScan
  Complete details at https://RandomNerdTutorials.com/esp32-useful-wi-fi-functions-arduino/
*/

#include "WiFi.h"
#include <HTTPClient.h>
String idPath;

const char* ssid = "Abiding Tech B";
const char* password = "nomorefreewifi";
float id;
#define BUZ_PIN 23

String preAlertValue = "-1";

WiFiServer server(80);
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void setup() {
  idPath = "https://pick-by-click-default-rtdb.firebaseio.com/Devices/" + String(ESP.getEfuseMac()) + "/alert.json";
  Serial.println(idPath);
  Serial.begin(115200);

  pinMode(BUZ_PIN, OUTPUT);
  digitalWrite(BUZ_PIN, HIGH);
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  //  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  //  delay(100);
  Serial.println();
  //  Serial.println("Setup done");
  initWiFi();
  // server.begin();

  preAlertValue = getAlert();
}
void buzOn() {
  digitalWrite(BUZ_PIN, LOW);
}
void buzOff() {
  digitalWrite(BUZ_PIN, HIGH);
}

String getAlert() {
  HTTPClient http;
  http.begin(idPath.c_str());
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();

    Serial.println(payload);
    return payload;
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  return "-1";
  // Free resources
  http.end();
}
void loop() {

  String value = getAlert();
  if (value != "-1" && value != preAlertValue) {
    for (int i = 0; i < 3; i++) {
      buzOn();
      delay(1500);
      buzOff();
      delay(1000);
    }
    preAlertValue = value;
  }
  delay(5000);

  //  WiFiClient client = server.available();
  //
  //              client.println("<!DOCTYPE html><html>");
  //            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  //            client.println("<link rel=\"icon\" href=\"data:,\">");
  //            // CSS to style the on/off buttons
  //            // Web Page Heading
  //            client.println("<body><h1>ESP32 Web Server</h1>");
  //            client.stop();
  //  Serial.println("scan start");
  //
  //  // WiFi.scanNetworks will return the number of networks found
  //  int n = WiFi.scanNetworks();
  //  Serial.println("scan done");
  //  if (n == 0) {
  //      Serial.println("no networks found");
  //  } else {
  //    Serial.print(n);
  //    Serial.println(" networks found");
  //    for (int i = 0; i < n; ++i) {
  //      // Print SSID and RSSI for each network found
  //      Serial.print(i + 1);
  //      Serial.print(": ");
  //      Serial.print(WiFi.SSID(i));
  //      Serial.print(" (");
  //      Serial.print(WiFi.RSSI(i));
  //      Serial.print(")");
  //      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
  //      delay(10);
  //    }
  //  }
  //  Serial.println("");
  //
  //  // Wait a bit before scanning again
  //  delay(5000);
}

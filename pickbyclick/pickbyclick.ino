#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/Picopixel.h>
#include <Fonts/Org_01.h>
#include <WiFi.h>
#include "time.h"
#include <HTTPClient.h>
#include "iot_iconset_16x16.c"
#include <WiFiManager.h>


// Variables will change:
int lastSteadyStateBtn1 = LOW;       // the previous steady state from the input pin
int lastFlickerableStateBtn1 = LOW;  // the previous flickerable state from the input pin
int lastSteadyStateBtn2 = LOW;       // the previous steady state from the input pin
int lastFlickerableStateBtn2 = LOW;
int currentStateBtn1;
int currentStateBtn2;  // the current reading from the input pin

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long lastAlertTime = 0;

uint32_t chipId = 0;

const char* ssid = "Abiding Tech EDU";
const char* password = "nomorefreewifi";
const char* ntpServer = "pool.ntp.org";
const int gmtOffset_sec = 18000;
const int daylightOffset_sec = 0;

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define BUZZER 13
#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define SDA 4
#define SCL 15

WiFiManager wm;
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BUTTON_PIN 23  // GIOP21 pin connected to button
#define BTN_PIN 2
#define DEBOUNCE_TIME 50  // the debounce time in millisecond, increase this time if it still chatter
int preAlert;
// 'logob', 32x15px
const unsigned char mylogob[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0x00, 0x00, 0xf0, 0x07, 0xc0,
  0x03, 0x8f, 0xe0, 0xf0, 0x06, 0x07, 0xe0, 0x38, 0x08, 0x03, 0xc0, 0x38, 0x08, 0x0f, 0xe0, 0x18,
  0x00, 0x0f, 0xf0, 0x38, 0x04, 0x10, 0x10, 0x38, 0x03, 0x9f, 0x70, 0xf0, 0x00, 0xe0, 0x03, 0xc0,
  0x00, 0x3f, 0xff, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00
};

String urlencode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  char code2;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encodedString += '+';
    } else {
      encodedString += c;
    }
  }
  return encodedString;
}
int sendNotification(String title, String message) {
  HTTPClient http;
  String url = urlencode("https://abidingtech.com/misc/send.php?title=" + title + "&body=" + message + "&topic=" + String(chipId) + "&key=AAAAhJoaac0:APA91bGIwYsLQ3zW5w4_e8UEUfZ9VIkYnPNyAAJi7OcxYlYMsE1HxEQMsPMcFpdpJm_Diz_wcUJil-fKsWgGyKc5ahcDUTmaX7DelRBtVbQ_a94r_HZ7PtByTgsprkk3ziQsNhVFybVj");
  Serial.println(url.c_str());
  http.begin(url);
  //  http.addHeader("Content-Type","application/json");
  Serial.print("Sending notification ! ==> ");

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    Serial.println("OK");
  } else {
    Serial.println("Failed " + String(httpCode));
  }
  http.end();
  return httpCode;
}

int getAlertValue() {
  HTTPClient http;
  String url = urlencode("https://pick-by-click-default-rtdb.firebaseio.com/Devices/" + String(chipId) + "/alert.json");
  Serial.println(url.c_str());
  http.begin(url);
  //  http.addHeader("Content-Type","application/json");
  Serial.print("get Alert ! ==> ");
  int payload = -1;
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    payload = http.getString().toInt();
    Serial.println("Alert-> " + String(payload));
  } else {
    Serial.println("Failed " + String(httpCode));
  }
  http.end();
  return payload;
}
void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  //  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  //  Serial.println(timeinfo.tm_mday);
  //  Serial.println(timeinfo.tm_wday);
  //  Serial.println(timeinfo.tm_mon);
  //  Serial.println(timeinfo.tm_year);
  //  Serial.println(timeinfo.tm_yday);
  char timeBuffer[12];
  char dateBuffer[11];

  int year = 1900 + timeinfo.tm_year;
  int month = timeinfo.tm_mon + 1;
  int day = timeinfo.tm_mday;

  int hour = timeinfo.tm_hour;
  int mint = timeinfo.tm_min;
  int sec = timeinfo.tm_sec;
  String meridien = "AM";
  if (hour > 12) {
    hour = hour - 12;
    meridien = "PM";
  }
  sprintf(timeBuffer, "%d:%02d:%02d %s", hour, mint, sec, meridien);
  sprintf(dateBuffer, "%02d-%02d-%02d", day, month, year);
  //  String stringTime = String(timeinfo.tm_hour)+":"+String(timeinfo.tm_min)+":"+String(timeinfo.tm_sec);
  String values[] = { String(dateBuffer), String(timeBuffer) };
  writeText(values);
}
void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BTN_PIN, INPUT_PULLUP);
  Serial.begin(115200);


  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
  Wire.begin(SDA, SCL);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }
  beep(1000);


  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(300);
  bool res = wm.autoConnect("Pick-By-Click", "11223344");


  if (!res) {

    writeText("AP Mode: 192.168.4.1");
    Serial.println(" Start AP mode");
    // ESP.restart();
  } else {
    Serial.println(" CONNECTED");
    writeText(" Connected");
    beep(500, 2);
    lastAlertTime = millis();


    //init and get the time
    // configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    // printLocalTime();

    for (int i = 0; i < 17; i = i + 8) {
      chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
    Serial.println(chipId);
    String path = "/Devices/" + String(chipId);
    preAlert = getAlertValue();
    sendNotification("Online", String(chipId) + " is Online now");

    oled.clearDisplay();
  }

  // writeText("Connecting to " + String(ssid));
  // Serial.printf("Connecting to %s ", ssid);
  // WiFi.begin(ssid, password);
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  //   chirp();
  // }




  //disconnect WiFi as it's no longer needed
  //  WiFi.disconnect(true);
  //  WiFi.mode(WIFI_OFF);

  //  displayHeader();
  //  oled.drawBitmap(0, 0, wifi2_icon16x16, 16, 16, 1);
  //  oled.drawBitmap(112, 0, bat2_icon16x16, 16, 16, 1);
  //  oled.drawBitmap(48, 0, mylogob, 32, 15, 1);

  //  oled.setCursor(42, 5);
  //  oled.setFont(&FreeSerif9pt7b);
  //  oled.setTextSize(1);
  //  oled.setTextColor(SSD1306_WHITE);
  //  oled.print("EZAS");


  //  oled.setCursor(0, 56);
  //  oled.setFont(&TomThumb);
  //  oled.setTextSize(1); // Draw 2X-scale text
  //  oled.setTextColor(SSD1306_WHITE);
  //  oled.print("Abiding");

  //  oled.display();


  //writeText("Init", "Connecting to wifi");
  //delay(2000);
  //
  //writeText("connect", "Abiding Tech");
  //delay(2000);
}
int count = 0;
void loop() {
  wm.process();
  // read the state of the switch/button:

  if (WiFi.status() == WL_CONNECTED) {
    currentStateBtn1 = digitalRead(BUTTON_PIN);
    currentStateBtn2 = digitalRead(BTN_PIN);

    // If the switch/button changed, due to noise or pressing:
    if (currentStateBtn1 != lastFlickerableStateBtn1) {  // reset the debouncing timer
      lastDebounceTime = millis();
      // save the the last flickerable state
      lastFlickerableStateBtn1 = currentStateBtn1;
    }
    if (currentStateBtn2 != lastFlickerableStateBtn2) {  // reset the debouncing timer
      lastDebounceTime = millis();
      // save the the last flickerable state
      lastFlickerableStateBtn2 = currentStateBtn2;
    }

    //for 1
    if ((millis() - lastDebounceTime) > DEBOUNCE_TIME) {

      if (lastSteadyStateBtn1 == HIGH && currentStateBtn1 == LOW) {
        Serial.println(" pressed");
        writeText("Set Delay: " + String(++count * 5));
      }

      else if (lastSteadyStateBtn1 == LOW && currentStateBtn1 == HIGH)
        Serial.println("released");
      lastSteadyStateBtn1 = currentStateBtn1;
    }

    //for 2
    if ((millis() - lastDebounceTime) > DEBOUNCE_TIME) {

      if (lastSteadyStateBtn2 == HIGH && currentStateBtn2 == LOW) {
        Serial.println("0");
        writeText("Sending Notification...");
        sendNotification("Pickup", String(count * 5) + " mint badd laine aa jana. mehrbani...");
        writeText("Notification Sent");
        delay(3000);
        writeText(" Connected");
        count = 0;
      }

      else if (lastSteadyStateBtn2 == LOW && currentStateBtn2 == HIGH)
        Serial.println("1");

      // save the the last steady state
      lastSteadyStateBtn2 = currentStateBtn2;

      if ((millis() - lastAlertTime) > 7000) {
        int alertValue = getAlertValue();
        if (alertValue > 0 && alertValue != preAlert) {
          writeText("Alert....");
          beep(1000, 10);
          preAlert = alertValue;
          writeText("Connected");
        }
        lastAlertTime = millis();
      }
    }
  }
}


void setHeader() {
  //  oled.clearDisplay();
  oled.drawBitmap(0, 0, wifi2_icon16x16, 16, 16, 1);
  oled.drawBitmap(112, 0, bat2_icon16x16, 16, 16, 1);
  oled.drawBitmap(48, 0, mylogob, 32, 15, 1);
  delay(10);
}
void setTextStyle() {
  oled.setFont(&Org_01);
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
}
void writeText(String text) {
  oled.clearDisplay();
  setHeader();
  oled.setCursor(0, 30);
  setTextStyle();
  oled.println(text);
  oled.display();
}
void writeText(String text[]) {
  //  writeText(text[0]);
  oled.clearDisplay();
  setHeader();
  oled.setCursor(0, 30);
  setTextStyle();
  for (int i = 0; i < sizeof(text); i++) {
    oled.println(text[i]);
  }

  oled.display();

  //  oled.setTextSize(2); // Draw 2X-scale text
  //  oled.setTextColor(SSD1306_WHITE);
  //  oled.setCursor(0, 0);
  //  oled.println(title);
  //  oled.display();
  //  oled.setTextSize(1);
  //  oled.print(text);
  //  oled.display();
}

void testscrolltext(void) {
  oled.clearDisplay();

  oled.setTextSize(2);  // Draw 2X-scale text
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(10, 0);
  oled.println(F("EZAS"));
  oled.display();  // Show initial text
  delay(100);

  // Scroll in various directions, pausing in-between:
  oled.startscrollright(0x00, 0x0F);
  delay(2000);
  oled.stopscroll();
  delay(1000);
  oled.startscrollleft(0x00, 0x0F);
  delay(2000);
  oled.stopscroll();
  delay(1000);
  oled.startscrolldiagright(0x00, 0x07);
  delay(2000);
  oled.startscrolldiagleft(0x00, 0x07);
  delay(2000);
  oled.stopscroll();
  delay(1000);
}

void alert() {
  digitalWrite(BUZZER, HIGH);
  delay(1000);
  for (int i = 0; i < 10; i++) {
    // digitalWrite(led, HIGH);
    digitalWrite(BUZZER, HIGH);
    delay(500);
    // digitalWrite(led, LOW);
    digitalWrite(BUZZER, LOW);
    delay(300);
  }
}
void chirp() {
  beep(30);
}
void beep(int milli) {
  digitalWrite(BUZZER, HIGH);
  delay(milli);
  digitalWrite(BUZZER, LOW);
  delay(milli);
}
void beep(int milli, int count) {
  for (int i = 0; i < count; i++) {
    beep(milli);
  }
}
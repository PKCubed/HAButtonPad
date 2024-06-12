#include <string.h>

#include <LiquidCrystal.h>

#include <FastLED.h>
#define NUM_LEDS 6
#define DATA_PIN 16 // ws2812b LED data pin
CRGB leds[NUM_LEDS];

#include <WiFi.h>

#include <PubSubClient.h>
#define mqtt_server "10.0.2.5"
WiFiClient espClient;
PubSubClient client(espClient);

const char* mqttS = "hapanel1/s";

const char* ssid = "PKNet";
const char* password = "HP w2207h";

const char* mqttuser = "mqtt";
const char* mqttpw = "12345678";

int bank = 0;
int lastBank = 0;

const char* bankNames[8] = {
  "Houselights   ",
  "Valances      ",
  "Stage Lights  ",
  "ChristmasLghts",
  "              ",
  "              ",
  "              ",
  "              "
  };

// Pins
const int rs = 13, en = 12, d4 = 14, d5 = 27, d6 = 26, d7 = 25, contrast = 18; // Char LCD
const int b1 = 4, b2 = 2, b3 =15, b4 = 5, b5 = 17, b6 = 32; // Buttons

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

int switches[8][4] = {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}};
int buttonStates[] = {0,0,0,0,0,0};
int buttonLastStates[] = {0,0,0,0,0,0};
int buttonTimes[] = {0,0,0,0,0,0};
const int debounceTime = 100; // ms

void mqttconnect() {
  lcd.clear();
  lcd.print("MQTT Connecting");
  // Loop until we're reconnected
  int counter = 0;
  while (!client.connected()) {
    if (counter == 5) {
      Serial.println("Restarting...");
      lcd.clear();
      lcd.print("Failed connection");
      lcd.setCursor(0,2);
      lcd.print("Restarting...");
      delay(1500);
      
      ESP.restart(); // Restart everything
    }
    counter += 1;
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect

    if (client.connect("HAButtonPanel1", mqttuser, mqttpw)) {
      Serial.println("connected");
      lcd.clear();
      lcd.print("MQTT Connected");
    } else {
      lcd.clear();
      lcd.print("Failed connection");
      lcd.setCursor(0,2);
      lcd.print("Try again in 2s");
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 2 seconds");
      delay(2000);
    }
  }
  for (int i=0;i<=32;i++) {
    client.subscribe((char*)strncat(mqttS,i));
  }
  showbank();
  lastBank = bank;
}

void mqttcallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.println((strncat(topic[10], topic[11])-'0')/4);
  Serial.println((strncat(topic[10], topic[11])-'0')%4);
  if (topic[11]) {
    switches[(strncat(topic[10], topic[11])-'0')/4][(strncat(topic[10], topic[11])-'0')%4] = (char)payload[0] - '0';
  } else if (topic[10]) {
    switches[(topic[10]-'0')/4][(topic[10]-'0')%4] = (char)payload[0] - '0';
  }
}

void showbank() {
  lcd.clear();
  lcd.print(bank+1);
  lcd.print(".");
  lcd.print(bankNames[bank]);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("HA Button Panel 1.0");

  pinMode(b1, INPUT_PULLUP);
  pinMode(b2, INPUT_PULLUP);
  pinMode(b3, INPUT_PULLUP);
  pinMode(b4, INPUT_PULLUP);
  pinMode(b5, INPUT_PULLUP);
  pinMode(b6, INPUT_PULLUP);

  pinMode(contrast, OUTPUT);
  analogWrite(contrast, 130);
  lcd.begin(16, 2);

  lcd.print("HA Button Panel");
  lcd.setCursor(0,2);
  lcd.print("v1.0");

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);  // GRB ordering is assumed
  leds[0] = CRGB(255,255,255);
  FastLED.show();
  delay(100);
  leds[1] = CRGB(255,255,255);
  FastLED.show();
  delay(100);
  leds[2] = CRGB(255,255,255);
  FastLED.show();
  delay(100);
  leds[3] = CRGB(255,255,255);
  FastLED.show();
  delay(100);
  leds[4] = CRGB(255,255,255);
  FastLED.show();
  delay(100);
  leds[5] = CRGB(255,255,255);
  FastLED.show();
  delay(1400);

  leds[0] = CRGB(0,0,0);
  leds[1] = CRGB(0,0,0);
  leds[2] = CRGB(0,0,0);
  leds[3] = CRGB(0,0,0);
  leds[4] = CRGB(0,0,0);
  leds[5] = CRGB(0,0,0);
  FastLED.show();

  Serial.println("Connecting to WiFi...");

  lcd.clear();
  lcd.print("Connecting WiFi");
  lcd.setCursor(0,2);
  lcd.print("SSID: ");
  lcd.print(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(10);
    Serial.print(".");
  }

  lcd.clear();
  lcd.print("Connect Success");
  lcd.setCursor(0,2);
  lcd.print(WiFi.localIP());

  delay(1000);

  client.setServer(mqtt_server, 1883);
  mqttconnect();
  client.setCallback(mqttcallback);

  delay(500);
  showbank();

  leds[0] = CRGB(50,20,0);
  leds[1] = CRGB(50,20,0);
  leds[2] = CRGB(20,0,0);
  leds[3] = CRGB(20,0,0);
  leds[4] = CRGB(20,0,0);
  leds[5] = CRGB(20,0,0);
  FastLED.show();
}

void loop() {
  if (!client.connected()) {
    mqttconnect();
  }
  client.loop();

  if (bank != lastBank) {
    showbank();
    lastBank = bank;
  }

  // put your main code here, to run repeatedly:
  if (switches[bank][0]) {
    leds[5] = CRGB(0, 255, 0);
  } else {
    leds[5] = CRGB(20,0,0);
  }
  if (switches[bank][1]) {
    leds[4] = CRGB(0, 255, 0);
  } else {
    leds[4] = CRGB(20,0,0);
  }
  if (switches[bank][2]) {
    leds[3] = CRGB(0, 255, 0);
  } else {
    leds[3] = CRGB(20,0,0);
  }
  if (switches[bank][3]) {
    leds[2] = CRGB(0, 255, 0);
  } else {
    leds[2] = CRGB(20,0,0);
  }
  FastLED.show();



  if (!digitalRead(b1)) {
    buttonStates[0] = 1;
    buttonTimes[0] = millis();
  } else if (millis() >= buttonTimes[0] + debounceTime) {
    buttonStates[0] = 0;
  }
  if (!digitalRead(b2)) {
    buttonStates[1] = 1;
    buttonTimes[1] = millis();
  } else if (millis() >= buttonTimes[1] + debounceTime) {
    buttonStates[1] = 0;
  }
  if (!digitalRead(b3)) {
    buttonStates[2] = 1;
    buttonTimes[2] = millis();
  } else if (millis() >= buttonTimes[2] + debounceTime) {
    buttonStates[2] = 0;
  }
  if (!digitalRead(b4)) {
    buttonStates[3] = 1;
    buttonTimes[3] = millis();
  } else if (millis() >= buttonTimes[3] + debounceTime) {
    buttonStates[3] = 0;
  }
  if (!digitalRead(b5)) {
    buttonStates[4] = 1;
    buttonTimes[4] = millis();
  } else if (millis() >= buttonTimes[4] + debounceTime) {
    buttonStates[4] = 0;
  }
  if (!digitalRead(b6)) {
    buttonStates[5] = 1;
    buttonTimes[5] = millis();
  } else if (millis() >= buttonTimes[5] + debounceTime) {
    buttonStates[5] = 0;
  }



  if (buttonStates[0] && !buttonLastStates[0]) { // Detect rising edge
    buttonLastStates[0] = 1;
    switches[bank][0] = !switches[bank][0];
    if (switches[bank][0]) {
      client.publish(strncat(mqttS, (bank*4+0)), "1", true);
    } else {
      client.publish(strncat(mqttS, (bank*4+0)), "0", true);
    }
  } if (!buttonStates[0] && buttonLastStates[0]) { // Detect falling edge
    buttonLastStates[0] = 0;
  }
  if (buttonStates[1] && !buttonLastStates[1]) { // Detect rising edge
    buttonLastStates[1] = 1;
    switches[bank][1] = !switches[bank][1];
    if (switches[bank][1]) {
      client.publish(strncat(mqttS, (bank*4+1)), "1", true);
    } else {
      client.publish(strncat(mqttS, (bank*4+1)), "0", true);
    }
  } if (!buttonStates[1] && buttonLastStates[1]) { // Detect falling edge
    buttonLastStates[1] = 0;
  }
  if (buttonStates[2] && !buttonLastStates[2]) { // Detect rising edge
    buttonLastStates[2] = 1;
    switches[bank][2] = !switches[bank][2];
    if (switches[bank][2]) {
      client.publish(strncat(mqttS, (bank*4+2)), "1", true);
    } else {
      client.publish(strncat(mqttS, (bank*4+2)), "0", true);
    }
  } if (!buttonStates[2] && buttonLastStates[2]) { // Detect falling edge
    buttonLastStates[2] = 0;
  }
  if (buttonStates[3] && !buttonLastStates[3]) { // Detect rising edge
    buttonLastStates[3] = 1;
    switches[bank][3] = !switches[bank][3];
    if (switches[bank][3]) {
      client.publish(strncat(mqttS, (bank*4+3)), "1", true);
    } else {
      client.publish(strncat(mqttS, (bank*4+3)), "0", true);
    }
  } if (!buttonStates[3] && buttonLastStates[3]) { // Detect falling edge
    buttonLastStates[3] = 0;
  }
  if (buttonStates[4] && !buttonLastStates[4]) { // Detect rising edge
    Serial.println("Button 5 Pressed");
    buttonLastStates[4] = 1;
    bank--;
    if (bank <= 0) {
      bank = 0;
    }
    Serial.println(bank);
  } if (!buttonStates[4] && buttonLastStates[4]) { // Detect falling edge
    buttonLastStates[4] = 0;
  }
  if (buttonStates[5] && !buttonLastStates[5]) { // Detect rising edge
    Serial.println("Button 6 Pressed");
    buttonLastStates[5] = 1;
    bank++;
    if (bank >= 7) {
      bank = 7;
    }
    Serial.println(bank);
  } if (!buttonStates[5] && buttonLastStates[5]) { // Detect falling edge
    buttonLastStates[5] = 0;
  }
}

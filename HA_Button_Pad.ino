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

String mqttSwitch = "hapanel1/switch";
String mqttScene = "hapanel1/scene";

const char* ssid = "PKNet";
const char* password = "HP w2207h";

const char* mqttuser = "mqtt";
const char* mqttpw = "12345678";

const char* mqttdevicename = "HAButtonPanel0";

int bank = 0;
int lastBank = 0;

const char* bankNames[8] = {
  "Houselights   ",
  "Valances      ",
  "Stage Lights  ",
  "ChristmasLghts",
  "Audio Systems ",
  "Video Systems ",
  "Scenes A      ", // Radio Buttons
  "Scenes B      "  // Radio Buttons
  };

// Pins
const int rs = 13, en = 12, d4 = 14, d5 = 27, d6 = 26, d7 = 25, contrast = 18; // Char LCD
const int b1 = 4, b2 = 2, b3 =15, b4 = 5, b5 = 17, b6 = 32; // Buttons

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

int switches[8][4] = {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}};
int scenes[1] = {0}; // 0-7
int buttonStates[] = {0,0,0,0,0,0};
int buttonLastStates[] = {0,0,0,0,0,0};
int buttonTimes[] = {0,0,0,0,0,0};
const int debounceTime = 100; // ms

/*
char charconcat(char char1, char char2) {
  char output[2];
  strcpy(output, char1);
  strcat(output, char2);
  return output;
}

char inttochar(int integer) {
  char output[2];
  itoa(integer, output, 10);
  return output; 
}

char mqttsconcat(char chars) {
  char output[12];
  strcpy(output, mqttSwitch);
  strcat(output, chars);
  return output;
}
*/

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

    if (client.connect(mqttdevicename, mqttuser, mqttpw)) {
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
  for (int i=0;i<=24;i++) {
    client.subscribe((mqttSwitch + String(i)).c_str());
  }
  client.subscribe((mqttScene + String(0)).c_str());
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
  if (String(topic).startsWith(mqttSwitch)) {
    switches[String(topic).substring(mqttSwitch.length()).toInt()/4][String(topic).substring(mqttSwitch.length()).toInt()%4] = (char)payload[0] - '0';
  } if (String(topic).startsWith(mqttScene)) {
    Serial.println("Scene received");
    scenes[0] = (char)payload[0] - '0';
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
  Serial.println("HA Button Panel");

  pinMode(b1, INPUT_PULLUP);
  pinMode(b2, INPUT_PULLUP);
  pinMode(b3, INPUT_PULLUP);
  pinMode(b4, INPUT_PULLUP);
  pinMode(b5, INPUT_PULLUP);
  pinMode(b6, INPUT_PULLUP);

  pinMode(contrast, OUTPUT);
  analogWrite(contrast, 130);
  lcd.begin(16, 2);

  lcd.print(mqttdevicename);
  lcd.setCursor(0,2);
  lcd.print("v1.1");

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
  if (bank <= 5) {
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
  } else if (bank == 6) {
    if (scenes[0] == 0) {
      leds[5] = CRGB(0, 30, 255);
    } else {
      leds[5] = CRGB(0,0,10);
    }
    if (scenes[0] == 1) {
      leds[4] = CRGB(0, 30, 255);
    } else {
      leds[4] = CRGB(0,0,10);
    }
    if (scenes[0] == 2) {
      leds[3] = CRGB(0, 30, 255);
    } else {
      leds[3] = CRGB(0,0,10);
    }
    if (scenes[0] == 3) {
      leds[2] = CRGB(0, 30, 255);
    } else {
      leds[2] = CRGB(0,0,10);
    }
  } else if (bank == 7) {
    if (scenes[0] == 4) {
      leds[5] = CRGB(0, 30, 255);
    } else {
      leds[5] = CRGB(0,0,10);
    }
    if (scenes[0] == 5) {
      leds[4] = CRGB(0, 30, 255);
    } else {
      leds[4] = CRGB(0,0,10);
    }
    if (scenes[0] == 6) {
      leds[3] = CRGB(0, 30, 255);
    } else {
      leds[3] = CRGB(0,0,10);
    }
    if (scenes[0] == 7) {
      leds[2] = CRGB(0, 30, 255);
    } else {
      leds[2] = CRGB(0,0,10);
    }
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
    if (bank <= 5) {
      switches[bank][0] = !switches[bank][0];
      if (switches[bank][0]) {
        client.publish((mqttSwitch+String(bank*4+0)).c_str(), "1", true);
      } else {
        client.publish((mqttSwitch+String(bank*4+0)).c_str(), "0", true);
      }
    } else  {
      scenes[0] = (bank-6)*4;
      client.publish(("hapanel1/scene0"), String(scenes[0]).c_str(), true);
      Serial.print(scenes[0]);
    }
  } if (!buttonStates[0] && buttonLastStates[0]) { // Detect falling edge
    buttonLastStates[0] = 0;
  }
  if (buttonStates[1] && !buttonLastStates[1]) { // Detect rising edge
    buttonLastStates[1] = 1;
    if (bank <= 5) {
      switches[bank][1] = !switches[bank][1];
      if (switches[bank][1]) {
        client.publish((mqttSwitch+String(bank*4+1)).c_str(), "1", true);
      } else {
        client.publish((mqttSwitch+String(bank*4+1)).c_str(), "0", true);
      }
    } else {
      scenes[0] = (bank-6)*4+1;
      client.publish(("hapanel1/scene0"), String(scenes[0]).c_str(), true);
      Serial.print(scenes[0]);
    }
  } if (!buttonStates[1] && buttonLastStates[1]) { // Detect falling edge
    buttonLastStates[1] = 0;
  }
  if (buttonStates[2] && !buttonLastStates[2]) { // Detect rising edge
    buttonLastStates[2] = 1;
    if (bank <= 5) {
      switches[bank][2] = !switches[bank][2];
      if (switches[bank][2]) {
        client.publish((mqttSwitch+String(bank*4+2)).c_str(), "1", true);
      } else {
        client.publish((mqttSwitch+String(bank*4+2)).c_str(), "0", true);
      }
    } else {
      scenes[0] = (bank-6)*4+2;
      client.publish(("hapanel1/scene0"), String(scenes[0]).c_str(), true);
      Serial.print(scenes[0]);
    }
  } if (!buttonStates[2] && buttonLastStates[2]) { // Detect falling edge
    buttonLastStates[2] = 0;
  }
  if (buttonStates[3] && !buttonLastStates[3]) { // Detect rising edge
    buttonLastStates[3] = 1;
    if (bank <= 5) {
      switches[bank][3] = !switches[bank][3];
      if (switches[bank][3]) {
        client.publish((mqttSwitch+String(bank*4+3)).c_str(), "1", true);
      } else {
        client.publish((mqttSwitch+String(bank*4+3)).c_str(), "0", true);
      }
    } else {
      scenes[0] = (bank-6)*4+3;
      client.publish(("hapanel1/scene0"), String(scenes[0]).c_str(), true);
      Serial.print(scenes[0]);
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

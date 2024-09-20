// Initiallization and Configuration
  const char* version = "v1.0";
  // WT32-ETH01 Specific Ethernet Libraries
    #define DEBUG_ETHERNET_WEBSERVER_PORT       Serial
    #define _ETHERNET_WEBSERVER_LOGLEVEL_       3
    #include <WebServer_WT32_ETH01.h>
    WebServer server(80); // Set the web server port to 80

  // I2C
    #include "Wire.h" // The I2C Library
    int i2c_sda = 32;
    int i2c_scl = 33;
    // PCF8574 for Button Input
      #include <Adafruit_PCF8574.h> // Include the 8574 chip library to read input from the buttons. This same chip is used to output data to the character display with the LiquidCrystal_I2C.h library.
      Adafruit_PCF8574 buttoni2c;
    // PCF8574 for Character LCD
      #include <LiquidCrystal_I2C.h> // For communication with the 16x2 char display via the 8574 chip
      LiquidCrystal_I2C lcd(0x27, 16, 2); // Initialize the LCD

  // ws2812b
    #include <FastLED.h> // For outputing data to the ws2812b LEDs
    #define NUM_LEDS 6
    #define DATA_PIN 4 // ws2812b LED data pin
    CRGB leds[NUM_LEDS]; // Create an array with each led object

  // WiFi
    #include <WiFi.h> // for WiFi

    char ssid[] = "PKNet"; // WiFi SSID
    char password[] = "HP w2207h"; // WiFi Password
  // MQTT
    #include <PubSubClient.h> // For MQTT
    #define mqtt_server "10.0.2.5" // Define the MQTT broker ip address here (this will later be configured with the web ui and be stored in memory)
    WiFiClient mqttWiFiClient; // Create the Wifi Client object
    WiFiClient mqttEthClient; // Create the Ethernet Client object
    PubSubClient client(mqttEthClient); // By default, set MQTT to use the Ethernet client. We can switch over later if Ethernet is disconnected.

    String mqttSwitch = "buttonpanels/switch"; // this is the MQTT address of each switch. We append a 0-∞ at the end, ex. "buttonpanels/switch5"
    String mqttScene = "buttonpanels/scene"; // this is the MQTT address of each scene selection. We append a 0-∞ at the end, ex. "buttonpanels/switch5"

    char mqttuser[] = "mqtt"; // MQTT Username
    char mqttpw[] = "12345678"; // MQTT Password
    const char* mqttdevicename = "HAButtonPanel"; // Device name. This is not just for MQTT, but we use this as the overall device name.

  // Buttons
    const int debounceTime = 100; // The duration in milliseconds that a button will virtually stay pressed after it has been physically pressed. This removes any bounces on the rising edge. 
    // Pins
      const int b1 = 5, b2 = 4, b3 = 3, b4 = 2, b5 = 0, b6 = 1; // Buttons (Left --> Top)   These are the pins of the physical buttons on the 8574 chip.

  // Configuration
    const char* bankNames[8] = { // These are the names of the banks that show up on the first row of the LCD screen.
      "Houselights   ",
      "Valances      ",
      "Stage Lights  ",
      "ChristmasLghts",
      "Audio Systems ",
      "Video Systems ",
      "Scenes A      ", // Radio Buttons
      "Scenes B      "  // Radio Buttons
      };
  
  // Web Pages
    // Root Site Definitions
      // I have to break these up so we can put variables in between. There has to be a cleaner solution. Maybe we can stuff this stuff into an array?
      const String rootSite1 =
          F("<html>\
        <head>\
        <title>");

      String rootSite2 = mqttdevicename;
      const String rootSite3 = F("</title>\
        <style>\
        body { background-color: #000000; font-family: Arial, Helvetica, Sans-Serif; Color: #3344ff; }\
        </style>\
        </head>\
        <body>\
        </form>\
        <h1>Settings</h1><br>\
        <form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/\">\
        <p>MQTT Device Name</p>\
        <input type=\"text\" name=\"mqttdevicename\" value=\"");

      String rootSite4 = mqttdevicename;
      const String rootSite5 = F("\"><br>\
      <input type=\"submit\" value=\"Submit\">\
      </form>\
      </body>\
      </html>");
      // initiallization of blank variables
        int bank = 0; // The bank is what set of buttons/switches we are looking at. We have 4 physical buttons, and we can cycle through the banks to control many different switches with those 4 buttons. For example, bank 0 shows switches 0-3 on the buttons. bank 1 shows switches 4-7 on the buttons.
        int lastBank = 0; // to detect changes in banks.
        int switches[8][4] = {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}}; // These are the states of the switches. Eventually we need to be able to set this up to be an infinite number of switches configured in the web interface.
        int scenes[1] = {0}; // 0-7    This is the value of each scene selection. right now, we just have one scene selection. Eventually, we need to be able to set this up to be an infinite number of scene selections configured in the web interface.
        int buttonStates[] = {0,0,0,0,0,0}; // these are the states of the physical buttons. there are 6 buttons, 4 for the switches, and 2 to select banks. They also have other functions.
        int buttonLastStates[] = {0,0,0,0,0,0}; // To detect rising and falling edges of the button presses
        int buttonTimes[] = {0,0,0,0,0,0}; // To allow for software debouncing of the buttons. We basically extend the rising edge to be as long as the debounceTime if not longer.
        IPAddress blankIP(0, 0, 0, 0); // Create a blank IP address as a "reference" to compare to in order to see if we got an IP address.

// Web Page Functions
  void handleRoot() // This function runs when someone navigates to the root page of the web interface.
  {
    if (server.method() != HTTP_POST) // If someone is sending data via POST
    {

    }
    else // If someone is just requesting via GET
    {
      String message = F("POST form was:\n");
      
      //pref.begin("settings", false); // false = read/write, true = read only

      for (uint8_t i = 0; i < server.args(); i++)
      {
        //message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
        if (server.argName(i) == "mqttdevicename") {
          const char *argCharArray = server.arg(i).c_str();
          //pref.putString("mqttdevicename", argCharArray);
        }
      }

      //pref.end();

      server.send(200, F("text/plain"), mqttdevicename);
    }
    server.send(200, F("text/html"), rootSite1+rootSite2+rootSite3+rootSite4+rootSite5);
  }

  void handleNotFound() // When someone navigates to a page that does not exist.
  {
    String message = F("File Not Found\n\n");
    
    message += F("URI: ");
    message += server.uri();
    message += F("\nMethod: ");
    message += (server.method() == HTTP_GET) ? F("GET") : F("POST");
    message += F("\nArguments: ");
    message += server.args();
    message += F("\n");
    
    for (uint8_t i = 0; i < server.args(); i++)
    {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    
    server.send(404, F("text/plain"), message);
  }

// Functions
  void mqttconnect() { // This function connects mqtt
    lcd.clear();
    lcd.print("MQTT Connecting");
    // Loop until we're reconnected
    int counter = 0;
    while (!client.connected()) {
      if (counter == 5) {
        Serial.println("Restarting...");
        lcd.clear();
        lcd.print("Failed connection");
        lcd.setCursor(0,1);
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
        lcd.setCursor(0,1);
        lcd.print("Try again in 2s");
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" trying again in 2 seconds");
        delay(2000);
      }
    }
    Serial.println("Subscribing to MQTT topics");
    for (int i=0;i<=24;i++) {
      client.subscribe((mqttSwitch + String(i)).c_str());
    }
    client.subscribe((mqttScene + String(0)).c_str());
    showbank();
    lastBank = bank;
  }
  void mqttcallback(char* topic, byte* payload, unsigned int length) { // This function runs when a topic we've subscribed to has a new message for us.
    //Serial.print("Message arrived [");
    //Serial.print(topic);
    //Serial.print("] ");
    for (int i=0;i<length;i++) {
      Serial.print((char)payload[i]);
    }
    Serial.println();
    if (String(topic).startsWith(mqttSwitch)) {
      switches[String(topic).substring(mqttSwitch.length()).toInt()/4][String(topic).substring(mqttSwitch.length()).toInt()%4] = (char)payload[0] - '0';
    } if (String(topic).startsWith(mqttScene)) {

      scenes[0] = (char)payload[0] - '0';
    }
  }

  void showbank() { // We run this function to show the current bank name on the LCD screen.
    lcd.clear();
    lcd.print(bank+1);
    lcd.print(".");
    lcd.print(bankNames[bank]);
  }

void setup() {
  Serial.begin(115200); // Start serial console at specified baud rate
  Serial.println("HA Button Panel");
  Serial.println(version);
  Serial.println("Device Name:");
  Serial.println(mqttdevicename);
  Serial.println();

  Serial.println("I2C Starting");
  Wire.begin(i2c_scl, i2c_sda); // Begin I2C
  buttoni2c.begin(0x26, &Wire);
  for (uint8_t p=0; p<6; p++) {
    buttoni2c.pinMode(p, INPUT_PULLUP);
  }

  Serial.println("Starting Ethernet");
  WT32_ETH01_onEvent(); // To be called before ETH.begin()
  ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER);

  Serial.println("Starting Character LCD");
  lcd.init();
  lcd.backlight();

  lcd.print(mqttdevicename);
  lcd.setCursor(0,1);
  lcd.print(version);

  Serial.println("Starting ws2812b LEDs");
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);  // Setup the RGB Leds ; GRB ordering is assumed
  // Flash the LEDs white one at a time, then turn LEDs off.
    leds[4] = CRGB(255,255,255);
    FastLED.show();
    delay(100);
    leds[5] = CRGB(255,255,255);
    FastLED.show();
    delay(100);
    leds[3] = CRGB(255,255,255);
    FastLED.show();
    delay(100);
    leds[2] = CRGB(255,255,255);
    FastLED.show();
    delay(100);
    leds[1] = CRGB(255,255,255);
    FastLED.show();
    delay(100);
    leds[0] = CRGB(255,255,255);
    FastLED.show();
    delay(1400);

    leds[0] = CRGB(0,0,0);
    leds[1] = CRGB(0,0,0);
    leds[2] = CRGB(0,0,0);
    leds[3] = CRGB(0,0,0);
    leds[4] = CRGB(0,0,0);
    leds[5] = CRGB(0,0,0);
    FastLED.show();

  Serial.println("Waiting for 1 second to allow Ethernet to Connect");

  lcd.clear();
  lcd.print("Ethernet Check");

  delay(1000);

  Serial.println("Checking for Ethernet Connection");

  // Check if Ethernet is Connected and we have an IP address
  if (!(ETH.localIP()==blankIP)) { // This compares the Ethernet IP with 0.0.0.0 to see if we have an IP address on the Ethernet interface. If not, ETH.localIP() will return 0.0.0.0 and we can catch that and default to WiFi.
    // If we DO have an IP Address on ETH
    Serial.println("Ethernet is connected, defaulting to Ethernet over WiFi.");
    lcd.setCursor(0,1);
    lcd.print("Connected");

    delay(1000);

    Serial.println("Connecting MQTT");
    client.setServer(mqtt_server, 1883);
    mqttconnect();

    client.setCallback(mqttcallback);

    // Configure Web Interface for Ethernet
      server.on(F("/"), handleRoot);
      server.onNotFound(handleNotFound);
      server.begin(); // Begin Ethernet Web Interface
  } else {
    // If we DO NOT have an IP address on ETH
    Serial.println("Ethernet is not connected, defaulting to WiFi over Ethernet.");

    lcd.setCursor(0,1);
    lcd.print("Disconnected");

    delay(1000);

    Serial.println("Connecting to WiFi...");

    lcd.clear();
    lcd.print("Connecting WiFi");
    lcd.setCursor(0,1);
    lcd.print("SSID: ");
    lcd.print(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(10);
      Serial.print(".");
      if (millis() > 20000) { // If we have been waiting toooooo long
        lcd.clear();
        lcd.print("WiFi Failure");
        lcd.setCursor(0,1);
        lcd.print("Rebooting in 2s");
        delay(2000);
        ESP.restart(); // Restart everything
      }
    }

    lcd.clear();
    lcd.print("Connect Success");
    lcd.setCursor(0,1);
    lcd.print(WiFi.localIP());

    delay(1000);

    Serial.println("Connecting MQTT");

    client.setClient(mqttWiFiClient);
    client.setServer(mqtt_server, 1883);
    mqttconnect();
    client.setCallback(mqttcallback);
  }

  delay(500);
  showbank();

  leds[4] = CRGB(50,20,0);
  leds[5] = CRGB(50,20,0);
  leds[3] = CRGB(20,0,0);
  leds[2] = CRGB(20,0,0);
  leds[1] = CRGB(20,0,0);
  leds[0] = CRGB(20,0,0);
  FastLED.show();
}

void loop() {
  if (!client.connected()) { // Verify that we are connected to MQTT. If not:
    Serial.println("Detected MQTT disconnection. Attempting reconnection.");
    mqttconnect(); // connect to MQTT
  }
  client.loop(); // MQTT Loop Function

  server.handleClient(); // WebServer Loop Function

  if (bank != lastBank) { // Detect changes in the bank variable
    showbank(); // If we switched banks, display that new bank.
    lastBank = bank;
  }

  // Update LEDs
    if (bank <= 5) {
      if (switches[bank][0]) {
        leds[0] = CRGB(0, 255, 0);
      } else {
        leds[0] = CRGB(20,0,0);
      }
      if (switches[bank][1]) {
        leds[1] = CRGB(0, 255, 0);
      } else {
        leds[1] = CRGB(20,0,0);
      }
      if (switches[bank][2]) {
        leds[2] = CRGB(0, 255, 0);
      } else {
        leds[2] = CRGB(20,0,0);
      }
      if (switches[bank][3]) {
        leds[3] = CRGB(0, 255, 0);
      } else {
        leds[3] = CRGB(20,0,0);
      }
    } else if (bank == 6) {
      if (scenes[0] == 0) {
        leds[0] = CRGB(0, 30, 255);
      } else {
        leds[0] = CRGB(0,0,10);
      }
      if (scenes[0] == 1) {
        leds[1] = CRGB(0, 30, 255);
      } else {
        leds[1] = CRGB(0,0,10);
      }
      if (scenes[0] == 2) {
        leds[2] = CRGB(0, 30, 255);
      } else {
        leds[2] = CRGB(0,0,10);
      }
      if (scenes[0] == 3) {
        leds[3] = CRGB(0, 30, 255);
      } else {
        leds[3] = CRGB(0,0,10);
      }
    } else if (bank == 7) {
      if (scenes[0] == 4) {
        leds[0] = CRGB(0, 30, 255);
      } else {
        leds[0] = CRGB(0,0,10);
      }
      if (scenes[0] == 5) {
        leds[1] = CRGB(0, 30, 255);
      } else {
        leds[1] = CRGB(0,0,10);
      }
      if (scenes[0] == 6) {
        leds[2] = CRGB(0, 30, 255);
      } else {
        leds[2] = CRGB(0,0,10);
      }
      if (scenes[0] == 7) {
        leds[3] = CRGB(0, 30, 255);
      } else {
        leds[3] = CRGB(0,0,10);
      }
    }
    FastLED.show();


  // Read Button Input
    if (!buttoni2c.digitalRead(b1)) {
      buttonStates[0] = 1;
      buttonTimes[0] = millis();
    } else if (millis() >= buttonTimes[0] + debounceTime) {
      buttonStates[0] = 0;
    }
    if (!buttoni2c.digitalRead(b2)) {
      buttonStates[1] = 1;
      buttonTimes[1] = millis();
    } else if (millis() >= buttonTimes[1] + debounceTime) {
      buttonStates[1] = 0;
    }
    if (!buttoni2c.digitalRead(b3)) {
      buttonStates[2] = 1;
      buttonTimes[2] = millis();
    } else if (millis() >= buttonTimes[2] + debounceTime) {
      buttonStates[2] = 0;
    }
    if (!buttoni2c.digitalRead(b4)) {
      buttonStates[3] = 1;
      buttonTimes[3] = millis();
    } else if (millis() >= buttonTimes[3] + debounceTime) {
      buttonStates[3] = 0;
    }
    if (!buttoni2c.digitalRead(b5)) {
      buttonStates[4] = 1;
      buttonTimes[4] = millis();
    } else if (millis() >= buttonTimes[4] + debounceTime) {
      buttonStates[4] = 0;
    }
    if (!buttoni2c.digitalRead(b6)) {
      buttonStates[5] = 1;
      buttonTimes[5] = millis();
    } else if (millis() >= buttonTimes[5] + debounceTime) {
      buttonStates[5] = 0;
    }


  // Process Button Input
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

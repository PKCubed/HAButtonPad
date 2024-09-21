// Initiallization and Configuration
  const char* version = "v1.1";
  // WT32-ETH01 Specific Ethernet Libraries
    #define DEBUG_ETHERNET_WEBSERVER_PORT       Serial
    #define _ETHERNET_WEBSERVER_LOGLEVEL_       3
    #include "EthWebServer.h" // This library includes WiFi.h and WebServer.h
    WebServer server(80); // Initialize the Web Server at port 80 (This is both ethernet and wifi)
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
    //#include <WiFi.h> // for WiFi
    
    //#include <WiFiClient.h>
    //#include <WebServer.h>

    String ssid; // WiFi SSID
    String password; // WiFi Password

  // MQTT
    #include <PubSubClient.h> // For MQTT
    String mqtt_server = "10.0.2.5"; // Define the MQTT broker ip address here (this will later be configured with the web ui and be stored in memory)
    WiFiClient mqttClient; // Create the Wifi Client object (This covers both ethernet and wifi)

    PubSubClient client(mqttClient); // mqtt client object

    String mqttSwitch = "buttonpanels/switch"; // this is the MQTT address of each switch. We append a 0-∞ at the end, ex. "buttonpanels/switch5"
    String mqttScene = "buttonpanels/scene"; // this is the MQTT address of each scene selection. We append a 0-∞ at the end, ex. "buttonpanels/switch5"

    String mqttuser = "mqtt"; // MQTT Username
    String mqttpw = "12345678"; // MQTT Password
    String mqttdevicename = "HAButtonPanel"; // Device name. This is not just for MQTT, but we use this as the overall device name.

  // Update
    #include <Update.h>
  // Preferences
    // This library allows us to save data to the ESP32's non volatile memory so it persists between resets and power loss.
    #include <Preferences.h>
    Preferences preferences;

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
      const String rootSite[] = {
        F("<html>\
        <head>\
        <title>"),
        F("</title>\
        <style>\
        body { background-color: #000000; font-family: Arial, Helvetica, Sans-Serif; Color: #3344ff; }\
        </style>\
        </head>\
        <body>\
        </form>\
        <h1>Settings</h1><br>\
        <form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/\">\
        <p>Device Name (Requires reboot to take effect)</p>\
        <input type=\"text\" name=\"name\" value=\""),
        F("\"><br>\
        <p>WiFi SSID</p>\
        <input type=\"text\" name=\"ssid\" value=\""),
        F("\"><br>\
        <p>WiFi Password (Leave empty for no change)</p>\
        <input type=\"password\" name=\"wifi_pw\" value=\"\"\>\
        <br>\
        <input type=\"submit\" value=\"Submit\">\
        </form>\
        <a href=\"update\">OTA Update</a>\
        <a href=\"reboot\">Reboot</a>\
        </body>\
        </html>")
      };

      // Update Page
        const char* updateSite = 
          "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
          "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
            "<input type='file' name='update'>"
                  "<input type='submit' value='Update'>"
              "</form>"
          "<div id='prg'>progress: 0%</div>"
          "<script>"
            "$('form').submit(function(e){"
            "e.preventDefault();"
            "var form = $('#upload_form')[0];"
            "var data = new FormData(form);"
            " $.ajax({"
            "url: '/update',"
            "type: 'POST',"
            "data: data,"
            "contentType: false,"
            "processData:false,"
            "xhr: function() {"
            "var xhr = new window.XMLHttpRequest();"
            "xhr.upload.addEventListener('progress', function(evt) {"
            "if (evt.lengthComputable) {"
            "var per = evt.loaded / evt.total;"
            "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
            "}"
            "}, false);"
            "return xhr;"
            "},"
            "success:function(d, s) {"
            "console.log('success!')"
          "},"
          "error: function (a, b, c) {"
          "}"
          "});"
          "});"
          "</script>";

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
    if (server.method() != HTTP_POST) // If someone is sending data via GET
    {
      //server.send(200, F("text/html"), "Hello World - GET RECEIVED");
      Serial.println("Get Request Received");
      Serial.println(mqttdevicename);
    }
    else // If someone is just requesting via POST
    {
      //String message = F("POST form was:\n");
      
      preferences.begin("habuttonpad", false); // false = read/write, true = read only

      for (uint8_t i = 0; i < server.args(); i++)
      {
        //message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
        if (server.argName(i) == "name") {
          preferences.putString("name", server.arg(i));
          Serial.println(server.arg(i));
          //mqttdevicename = server.arg(i);
        } else if (server.argName(i) == "ssid") {
          preferences.putString("wifi_ssid", server.arg(i));
          Serial.println(server.arg(i));
          ssid = server.arg(i);
        } else if (server.argName(i) == "wifi_pw") {
          if (server.arg(i) != String("")) {
            preferences.putString("wifi_pw", server.arg(i));
            Serial.println(server.arg(i));
            password = server.arg(i);
          }
        }
      }

      preferences.end();

      //server.send(200, F("text/plain"), mqttdevicename);
    }
    server.send(200, F("text/html"), rootSite[0]+mqttdevicename+rootSite[1]+mqttdevicename+rootSite[2]+ssid+rootSite[3]);
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

      client.disconnect();
      client.setClient(mqttClient);

      delay(1000);

      if (client.connect(mqttdevicename.c_str(), mqttuser.c_str(), mqttpw.c_str())) {
        Serial.println("connected");
        lcd.clear();
        lcd.print("MQTT Connected");
        break;
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

  void wificonnect() {
    Serial.println("Connecting to WiFi...");

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.begin(ssid.c_str(), password.c_str());
    if (ETH.localIP()==blankIP) { // If Ethernet is Connected -  This compares the Ethernet IP with 0.0.0.0 to see if we have an IP address on the Ethernet interface. If not, ETH.localIP() will return 0.0.0.0 and we can catch that.
      // Ethernet is NOT connected
      Serial.println("Ethernet is NOT connected, so WiFi is required now.");
      lcd.clear();
      lcd.print("Connecting WiFi");
      lcd.setCursor(0,1);
      lcd.print("SSID: ");
      lcd.print(ssid.c_str());
      while (WiFi.status() != WL_CONNECTED) {
        delay(10);
        Serial.print(".");
        if (millis() > 18000) { // If we have been waiting toooooo long
          Serial.println("WiFi Connection Timeout. Rebooting.")
          lcd.clear();
          lcd.print("WiFi Failure");
          lcd.setCursor(0,1);
          lcd.print("Rebooting in 2s");
          delay(2000);
          ESP.restart(); // Restart everything
        }
      }
      Serial.println("WiFi Connection Successful. IP Address:");
      Serial.println(WiFi.localIP());
      lcd.clear();
      lcd.print("WiFi Successful");
      lcd.setCursor(0,1);
      lcd.print(WiFi.localIP());
    } else { // If Ethernet IS connected, we don't need to worry about making sure that the WiFi connection is successful.
      Serial.println("Ethernet IS connected, so WiFi is not required, and we will not reboot upon an unsuccessful connection. Nothing will print on the screen, no delays either.");
    }
  }

void setup() {
  // Initialize Serial
    Serial.begin(115200); // Start serial console at specified baud rate
  
  // Starting Messages
    Serial.println("HA Button Panel");
    Serial.println(version);

  // Open Non-Volatile Memory and Get Settings and Data
    Serial.println("Opening Flash Memory");
    preferences.begin("habuttonpad", false); // Begin the preferences library with the "habuttonpad" namespace

    String devicenamepf = preferences.getString("name");
    Serial.println(devicenamepf);
    if (devicenamepf == String("")) {
      Serial.println("There was no device name set. Setting device name to default");
      preferences.putString("name", mqttdevicename.c_str()); // Set default device name
    } else {
      mqttdevicename = devicenamepf;
    }

    String wifi_ssidpf = preferences.getString("wifi_ssid");
    Serial.println(wifi_ssidpf);
    if (wifi_ssidpf == String("")) {
      Serial.println("There was no WiFi SSID set.");
    }
    ssid = wifi_ssidpf;

    String wifi_pwpf = preferences.getString("wifi_pw");
    Serial.println(wifi_pwpf);
    if (wifi_pwpf == String("")) {
      Serial.println("There was no WiFi Password set.");
    }
    password = wifi_pwpf;

    preferences.end();

    Serial.println("Device Name:");
    Serial.println(mqttdevicename);
    Serial.println();

  // Begin I2C
    Serial.println("I2C Starting");
    Wire.begin(i2c_scl, i2c_sda); // Begin I2C
    buttoni2c.begin(0x26, &Wire);
    for (uint8_t p=0; p<6; p++) {
      buttoni2c.pinMode(p, INPUT_PULLUP);
    }

  // Begin Character LCD
    Serial.println("Starting Character LCD");
    lcd.init();
    lcd.backlight();

    lcd.print(mqttdevicename);
    lcd.setCursor(0,1);
    lcd.print(version);

  // Begin RGB LEDs
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

  // Begin Ethernet
    lcd.clear();
    lcd.print("Starting Eth");

    Serial.println("Starting Ethernet");
    WT32_ETH01_onEvent(); // To be called before ETH.begin()
    ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER);

    delay(2000);

  // Begin WiFi
    Serial.println("Attempting WiFi Connection");

    wificonnect();

    delay(1000);

  // Connect MQTT
    Serial.println("Connecting MQTT");

    client.setServer(mqtt_server.c_str(), 1883);
    mqttconnect();
    client.setCallback(mqttcallback);

  // Configure and start Web Interface
      Serial.println("Starting Web Interface");
      server.on("/", handleRoot);
      // /update - For uploading new firmware
        server.on("/update", HTTP_GET, []() {
          server.sendHeader("Connection", "close");
          server.send(200, "text/html", updateSite);
        });
        server.on("/update", HTTP_POST, []() {
          server.sendHeader("Connection", "close");
          server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
          ESP.restart();
        }, []() {
          HTTPUpload& upload = server.upload();
          if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("Update: %s\n", upload.filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
              Update.printError(Serial);
            }
          } else if (upload.status == UPLOAD_FILE_WRITE) {
            /* flashing firmware to ESP*/
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
              Update.printError(Serial);
            }
          } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) { //true to set the size to the current progress
              Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
            } else {
              Update.printError(Serial);
            }
          }
        });
        server.on("/reboot", HTTP_GET, []() {
          Serial.println("Reboot command from WebUI");
          server.sendHeader("Connection", "close");
          server.send(200, "text/html", "Rebooting Now...");
          ESP.restart();
        });
      server.onNotFound(handleNotFound);
      server.begin(); // Begin Web Interface
      delay(500);

  // Start Main Mode
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
  // Verify Connections
    if (WiFi.status() == WL_CONNECTED) { // If WiFi is connected
      if (!client.connected()) { // If MQTT is NOT connected
        Serial.println("Detected MQTT disconnection. Attempting reconnection.");
        mqttconnect();
      }
    } else {
      if (ETH.localIP()==blankIP) { // If WIFI is NOT connected and Ethernet is NOT connected
        Serial.println("Detected WiFi disconnection, attempting reconnection.");
        wificonnect();
      }
    }

  // Loop Functions
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

/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.
  
  WiFi Control Panel
  Connection: Connect the "Actuator" board from the MYOSA kit with the "Controller" board power them up.
  Working: This example is intended to demonstrate the capabilities of WiFi (Controller) board. Here the controller board hosts to a  WebApp (viz a Control Panel for controlling the Actuator Board) on existing WiFi network. Hence, it lets users have a few controls to interact with the board from Mobile phone or any Digital Device through WebApp.

  Synopsis of MYOSA platform
  MYOSA uses an ESP32 controller with Wi-Fi and Bluetooth connectivity.
  The kit includes motion, temperature/humidity, pressure, light/proximity/gesture,
  and air-quality sensor boards, plus an OLED display and an actuator board.
  The actuator board provides relay and buzzer outputs.
  The libraries support individual sensor examples and combined BLE applications.

  NOTE
  All information, including URL references, is subject to change without prior notice.
  Please always use the latest versions of software-release for best performance.
  Unless required by applicable law or agreed to in writing, this software is distributed on an 
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied

  Modifications
  10 September, 2026 by Pegasus Automation
  (as a part of MYOSA Initiative)
  
  Contact Team MYOSA for any kind of feedback/issues pertaining to performance or any update request.
  Email: myosa.event@gmail.com
*/

/* Library Inclusion - WiFi.h is generic ESP32 library available */
#include <WiFi.h>
#include <Actuator.h>

/* Creating Object of Actuator class */
Actuator gpioExpander;

/* Enter your WiFi credentials here so that MYOSA Controller Board can connect to Internet. */
const char* ssid     = "MYOSAbyMakeSense";
const char* password = "LearnTheEasyWay";

/* Creating an Object of the WiFiServer class */
WiFiServer server(80);

/* Setup Function */
void setup()
{
  
  /* Setting up the communication */
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(100000);

  /* Initializing Actuator Board */
  for (;;)
  {
    if (gpioExpander.ping())
    {
      Serial.println("4bit IO Expander Actautor (PCA9536) is connected");
      break;
    }
    Serial.println("4bit IO Expander Actuator (PCA9536) is disconnected");
    delay(500u);
  }
  /* Set buzzer IO as output */  gpioExpander.setState(BUZZER_IO, IO_LOW);

  gpioExpander.setMode(BUZZER_IO, IO_OUTPUT);

  pinMode(2, OUTPUT);      // set the LED pin mode

  delay(1000);


  /* Connecting to WiFi Network */
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  const unsigned long started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < 30000UL) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println(WiFi.status() == WL_CONNECTED ? "WiFi connected." : "WiFi connection pending; retrying in loop.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();

}

/* Loop Function */
void loop()
{
  static unsigned long lastReconnect = 0;
  if(WiFi.status() != WL_CONNECTED) {
    if(millis() - lastReconnect >= 30000UL) { lastReconnect = millis(); WiFi.reconnect(); }
    delay(10);
    return;
  }
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  WiFiClient client = server.accept();
#else
  WiFiClient client = server.available();
#endif
  if(!client) { delay(1); return; }
  String line, request;
  line.reserve(256);
  unsigned long started = millis(), lastByte = started;
  size_t totalBytes = 0;
  bool firstLine = true, complete = false, malformed = false;
  // Bound both memory and connection time, including clients that send data very slowly.
  while(client.connected() && millis() - started < 5000UL && millis() - lastByte < 2000UL) {
    if(!client.available()) { delay(1); continue; }
    const char c = client.read();
    lastByte = millis();
    if(++totalBytes > 2048 || c == 0) { malformed = true; break; }
    if(c == '\n') {
      if(firstLine) { request = line; firstLine = false; }
      else if(line.length() == 0) { complete = true; break; }
      line = "";
    } else if(c != '\r') {
      if(line.length() >= 256) { malformed = true; break; }
      line += c;
    }
  }
  if(complete && !malformed && request.startsWith("GET /")) {
    const int end = request.indexOf(' ', 4);
    const String version = end >= 0 ? request.substring(end + 1) : String();
    if(version == "HTTP/1.1" || version == "HTTP/1.0") {
      String path = request.substring(4, end);
      const int query = path.indexOf('?');
      if(query >= 0) path = path.substring(0, query);
      if(path == "/H") digitalWrite(2, HIGH);
      else if(path == "/L") digitalWrite(2, LOW);
      else if(path == "/X") gpioExpander.setState(BUZZER_IO, IO_HIGH);
      else if(path == "/Y") gpioExpander.setState(BUZZER_IO, IO_LOW);
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html; charset=utf-8");
      client.println("Connection: close");
      client.println();
      client.println("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'><title>MYOSA Control Panel</title></head>");
      client.println("<body style='font-family:sans-serif;text-align:center'><h1>MYOSA Control Panel</h1><h2>LED</h2>");
      client.println("<form><button formaction='/H'>Turn On</button> <button formaction='/L'>Turn Off</button></form><h2>Buzzer</h2>");
      client.println("<form><button formaction='/X'>Turn On</button> <button formaction='/Y'>Turn Off</button></form></body></html>");
    } else malformed = true;
  } else malformed = true;
  if(malformed) client.print("HTTP/1.1 400 Bad Request\r\nConnection: close\r\nContent-Length: 0\r\n\r\n");
  client.stop();
}

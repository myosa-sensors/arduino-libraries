/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.

  Synopsis of MYOSA platform
  MYOSA uses an ESP32 controller with Wi-Fi and Bluetooth connectivity.
  The kit includes motion, pressure, light/proximity/gesture, VL53L0X distance
  and MAX30100 heart-rate/SpO2 boards, with an OLED for measurements.
  The libraries support individual sensor examples and combined Wi-Fi and BLE applications.

  NOTE
  All information, including URL references, is subject to change without prior notice.
  Please always use the latest versions of software-release for best performance.
  Unless required by applicable law or agreed to in writing, this software is distributed on an 
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied

  Modifications
  12 September, 2026 by Pegasus Automation
  (as a part of MYOSA Initiative)
  
  Contact Team MYOSA for any kind of feedback/issues pertaining to performance or any update request.
  Email: myosa.event@gmail.com
*/

/* Library Inclusion */
#include "myosaWeb.h"

MYOSAWeb::MYOSAWeb(MYOSA &myosa) : _myosa(myosa), _server(80, 2)
{
}

/* Keep the hotspot running even if the configured router cannot be reached. */
bool MYOSAWeb::begin(const char *apSsid, const char *apPassword,
                     const char *staSsid, const char *staPassword)
{
  if(_started)
    return true;
  if(!apSsid || !apPassword || !staSsid || !staPassword ||
     strlen(apSsid) == 0 || strlen(apSsid) > 32 ||
     strlen(apPassword) < 8 || strlen(apPassword) > 63 ||
     strlen(staSsid) > 32 || strlen(staPassword) > 64)
    return false;

  _staSsid = staSsid;
  _staPassword = staPassword;
  WiFi.persistent(false);
  if(!WiFi.mode(WIFI_AP_STA))
    return false;
  WiFi.setHostname("myosa");
  const IPAddress apAddress(192, 168, 4, 1);
  _apReady = WiFi.softAPConfig(apAddress, apAddress, IPAddress(255, 255, 255, 0)) &&
             WiFi.softAP(apSsid, apPassword);
  if(!_apReady)
    return false;
  Serial.printf("MYOSA hotspot: %s | Open http://192.168.4.1/\n", apSsid);
  if(_staSsid.length())
  {
    WiFi.setAutoReconnect(true);
    WiFi.begin(_staSsid.c_str(), _staPassword.c_str());
    Serial.printf("Connecting to the configured Wi-Fi network; hotspot remains available.\n");
  }
  else
    Serial.printf("STA credentials are empty; use the MYOSA hotspot.\n");
  _retryTime = millis();
  _mdnsReady = MDNS.begin("myosa");
  if(_mdnsReady)
    MDNS.addService("http", "tcp", 80);
  _server.begin();
  _started = bool(_server);
  _snapshot = "{\"readings\":{}}";
  _sampleTime = millis() - 500;
  return _started;
}

/* Serialize a shared snapshot; JSON null never masquerades as a zero reading. */
void MYOSAWeb::collectReadings(void)
{
  MYOSAReadings snapshot;
  _myosa.readSensors(snapshot);
  static const uint8_t counts[8][4] = {
      {3, 3, 3, 2}, {}, {2, 3, 1, 0}, {1, 1, 3, 0}, {}, {}, {1, 0, 0, 0}, {1, 1, 0, 0}};
  _snapshot = "{\"uptime\":" + String(snapshot.sampledAt) + ",\"readings\":{";
  _snapshot.reserve(1600);
  bool first = true;
  for(uint8_t group = 0; group < 8; ++group)
  {
    for(uint8_t field = 0; field < 4; ++field)
    {
      if(!counts[group][field])
        continue;
      if(!first)
        _snapshot += ',';
      first = false;
      _snapshot += "\"" + String(group) + String(field) + "\":[";
      for(uint8_t i = 0; i < counts[group][field]; ++i)
      {
        if(i)
          _snapshot += ',';
        const float value = snapshot.values[group][field][i];
        _snapshot += isfinite(value) ? String(value, 2) : String("null");
      }
      _snapshot += ']';
    }
  }
  _snapshot += "}}";
}

void MYOSAWeb::closeClient(BrowserClient &client)
{
  client.socket.stop();
  client.request = client.header = client.json = "";
  client.body = nullptr;
  client.bodySize = client.sent = 0;
  client.sending = client.sendingBody = false;
}

/* Only same-origin GET requests are served; no credentials are returned to browsers. */
void MYOSAWeb::prepareResponse(BrowserClient &client)
{
  const int end = client.request.indexOf('\r');
  const String line = client.request.substring(0, end);
  const bool get = line.startsWith("GET ");
  const int pathEnd = line.indexOf(' ', 4);
  const String path = get && pathEnd > 4 ? line.substring(4, pathEnd) : String();
  const char *status = "200 OK";
  const char *type = "text/plain; charset=utf-8";
  bool compressed = false;
  if(!get)
  {
    status = "405 Method Not Allowed";
    client.json = "Only GET is supported.";
  }
  else if(path == "/" || path == "/index.html")
  {
    type = "text/html; charset=utf-8";
    compressed = true;
    client.body = MYOSA_DASHBOARD;
    client.bodySize = sizeof(MYOSA_DASHBOARD);
  }
  else if(path == "/api/readings")
  {
    type = "application/json";
    client.json = _snapshot;
  }
  else
  {
    status = "404 Not Found";
    client.json = "Not found.";
  }
  if(!compressed)
  {
    client.body = reinterpret_cast<const uint8_t *>(client.json.c_str());
    client.bodySize = client.json.length();
  }
  client.header = String("HTTP/1.1 ") + status + "\r\nContent-Type: " + type +
                  "\r\nContent-Length: " + String(client.bodySize) +
                  "\r\nConnection: close\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\n";
  if(compressed)
    client.header += "Content-Encoding: gzip\r\n";
  if(!get)
    client.header += "Allow: GET\r\n";
  client.header += "\r\n";
  client.sending = true;
  client.sent = 0;
}

/* Bound both request parsing and transmission so a slow browser cannot stall sampling. */
void MYOSAWeb::serviceClient(BrowserClient &client)
{
  if(client.socket.fd() < 0)
    return;
  if(!client.socket.connected() || uint32_t(millis() - client.activity) > 1500)
  {
    closeClient(client);
    return;
  }
  if(!client.sending)
  {
    uint16_t budget = 256;
    while(budget-- && client.socket.available())
    {
      const int value = client.socket.read();
      if(value < 0)
        break;
      client.request += char(value);
      if(client.request.length() > 2048)
      {
        closeClient(client);
        return;
      }
      if(client.request.endsWith("\r\n\r\n"))
      {
        prepareResponse(client);
        break;
      }
    }
    if(!client.sending)
      return;
  }
  const uint8_t *data = client.sendingBody ? client.body :
                        reinterpret_cast<const uint8_t *>(client.header.c_str());
  const size_t length = client.sendingBody ? client.bodySize : client.header.length();
  const size_t remaining = length - client.sent;
  // A nonblocking socket write avoids the NetworkClient retry loop on slow receivers.
  const int written = ::send(client.socket.fd(), data + client.sent,
                             remaining > 512 ? 512 : remaining, MSG_DONTWAIT);
  if(written < 0)
  {
    if(errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
      closeClient(client);
    return;
  }
  if(written == 0)
    return;
  client.sent += written;
  client.activity = millis();
  if(client.sent == length)
  {
    if(client.sendingBody)
      closeClient(client);
    else
    {
      client.sendingBody = true;
      client.sent = 0;
    }
  }
}

void MYOSAWeb::update(void)
{
  if(!_started)
    return;
  const bool connected = WiFi.status() == WL_CONNECTED;
  if(connected && (!_stationConnected || _stationAddress != WiFi.localIP()))
  {
    _stationAddress = WiFi.localIP();
    Serial.printf("MYOSA LAN: http://%s/\n", _stationAddress.toString().c_str());
    if(_mdnsReady)
      Serial.printf("You can also try http://myosa.local/ on networks supporting mDNS.\n");
    if(_stationAddress[0] == 192 && _stationAddress[1] == 168 && _stationAddress[2] == 4)
      Serial.printf("LAN overlaps the hotspot subnet; use a different LAN subnet for reliable AP+STA access.\n");
  }
  if(!connected && _stationConnected)
    Serial.printf("LAN disconnected; MYOSA hotspot remains available.\n");
  _stationConnected = connected;
  if(!connected && _staSsid.length() && uint32_t(millis() - _retryTime) >= 30000)
  {
    _retryTime = millis();
    WiFi.reconnect();
  }
  if(uint32_t(millis() - _sampleTime) >= 500)
  {
    _sampleTime = millis();
    collectReadings();
    _myosa.update();
  }
  for(auto &client : _clients)
  {
    if(client.socket.fd() < 0)
    {
      client.socket = _server.accept();
      if(client.socket.fd() >= 0)
      {
        client.activity = millis();
        client.socket.setNoDelay(true);
      }
    }
    serviceClient(client);
    _myosa.update();
  }
}

/* Include addresses in the normal OLED cycle so a Serial Monitor is optional. */
void MYOSAWeb::printConnectionInfo(void)
{
  const String ap = _apReady ? WiFi.softAPIP().toString() : "Unavailable";
  const String sta = _stationConnected ? _stationAddress.toString() :
                     (_staSsid.length() ? "Connecting..." : "Not configured");
  Serial.printf("Wi-Fi | AP: %s | STA: %s\n", ap.c_str(), sta.c_str());
  if(!_myosa.isDisplayReady())
    return;
  _myosa.display.clearDisplay();
  _myosa.display.setFont(nullptr);
  _myosa.display.setTextSize(1);
  _myosa.display.setTextColor(WHITE);
  _myosa.display.setCursor(0, 0);
  _myosa.display.printf("MYOSA Wi-Fi\nOpen http:// + IP\n\nAP: %s\nSTA: %s", ap.c_str(), sta.c_str());
  _myosa.display.display();
  _myosa.update();
}

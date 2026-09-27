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

#ifndef MYOSA_WEB_H
#define MYOSA_WEB_H

/* Library Inclusion */
#include "myosa.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <lwip/sockets.h>
#include <errno.h>
#include "myosaDashboard.h"

/* Small, incremental HTTP server: all sensor work remains in the caller loop. */
class MYOSAWeb
{
  public:
    explicit MYOSAWeb(MYOSA &myosa);
    bool begin(const char *apSsid, const char *apPassword,
               const char *staSsid, const char *staPassword);
    void update(void);
    void printConnectionInfo(void);

  private:
    struct BrowserClient
    {
      NetworkClient socket;
      String request, header, json;
      const uint8_t *body = nullptr;
      size_t bodySize = 0, sent = 0;
      uint32_t activity = 0;
      bool sending = false, sendingBody = false;
    };

    MYOSA &_myosa;
    NetworkServer _server;
    BrowserClient _clients[2];
    String _snapshot, _staSsid, _staPassword;
    uint32_t _sampleTime = 0, _retryTime = 0;
    IPAddress _stationAddress;
    bool _started = false, _apReady = false, _stationConnected = false;
    bool _mdnsReady = false;

    void collectReadings(void);
    void serviceClient(BrowserClient &client);
    void prepareResponse(BrowserClient &client);
    void closeClient(BrowserClient &client);
};

#endif

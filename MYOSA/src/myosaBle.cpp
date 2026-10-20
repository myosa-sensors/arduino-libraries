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
  20 October, 2026 by Pegasus Automation
  (as a part of MYOSA Initiative)
  
  Contact Team MYOSA for any kind of feedback/issues pertaining to performance or any update request.
  Email: myosa.event@gmail.com
*/

/* Library Inclusion */
#include "myosaBle.h"

#define NUM_SERVICES 8
#define MAX_CHARACTERISTICS 5

static bool bleReady = false;
static BLECharacteristic *pCharacteristics[NUM_SERVICES][MAX_CHARACTERISTICS] = {};
// Logical group indices retain the existing sensor characteristic UUID suffixes.
static const uint8_t characteristicCounts[NUM_SERVICES] = {4, 0, 3, 3, 0, 0, 1, 2};
static const uint8_t parameterCounts[NUM_SERVICES][4] = {
    {3, 3, 3, 2}, {}, {2, 3, 1, 0}, {1, 1, 3, 0}, {}, {}, {1, 0, 0, 0}, {1, 1, 0, 0}};

// One outstanding notification avoids filling the Bluetooth controller queue.
static const uint32_t notificationSpacing = 40;
static const uint32_t notificationTimeout = 2000;
static esp_gatt_if_t bleInterface = ESP_GATT_IF_NONE;
static portMUX_TYPE bleMux = portMUX_INITIALIZER_UNLOCKED;

struct BleLink
{
  bool connected = false, congested = false, closing = false, blocked = false;
  uint16_t connection = 0, mtu = 23, pendingHandle = 0;
  uint32_t generation = 0, pendingAt = 0, blockedAt = 0, sentAt = 0, closeAt = 0;
  esp_bd_addr_t address = {};
};
static BleLink bleLink;

struct BleField
{
  BLECharacteristic *characteristic = nullptr;
  BLE2902 *subscription = nullptr;
  bool subscribed = false;
  uint8_t length = 0;
  char payload[96] = {};
};
static BleField bleFields[13];
static uint8_t fieldCount = 0, nextField = 0;
static uint32_t queueGeneration = 0;

/* Stack callbacks only update link state; sensor and queue work stays in loop(). */
static void handleBleEvent(esp_gatts_cb_event_t event, esp_gatt_if_t interface,
                           esp_ble_gatts_cb_param_t *param)
{
  const uint32_t now = millis();
  portENTER_CRITICAL(&bleMux);
  if(event == ESP_GATTS_REG_EVT && param->reg.status == ESP_GATT_OK)
    bleInterface = interface;
  else if(event == ESP_GATTS_CONNECT_EVT)
  {
    const uint32_t generation = bleLink.generation + 1;
    bleLink = BleLink();
    bleLink.connected = true;
    bleLink.connection = param->connect.conn_id;
    bleLink.generation = generation;
    memcpy(bleLink.address, param->connect.remote_bda, sizeof(esp_bd_addr_t));
    for(uint8_t i = 0; i < fieldCount; ++i)
      bleFields[i].subscribed = false;
  }
  else if(event == ESP_GATTS_DISCONNECT_EVT && param->disconnect.conn_id == bleLink.connection)
  {
    bleLink.connected = false;
    bleLink.pendingHandle = 0;
    ++bleLink.generation;
  }
  else if(event == ESP_GATTS_MTU_EVT && param->mtu.conn_id == bleLink.connection)
    bleLink.mtu = param->mtu.mtu;
  else if(event == ESP_GATTS_WRITE_EVT && param->write.conn_id == bleLink.connection &&
          param->write.len == 2 && !param->write.is_prep)
  {
    for(uint8_t i = 0; i < fieldCount; ++i)
      if(bleFields[i].subscription->getHandle() == param->write.handle)
        bleFields[i].subscribed = (param->write.value[0] & 1) != 0;
  }
  else if(event == ESP_GATTS_CONGEST_EVT && param->congest.conn_id == bleLink.connection)
  {
    bleLink.congested = param->congest.congested;
    if(bleLink.congested && !bleLink.blocked)
    {
      bleLink.blocked = true;
      bleLink.blockedAt = now;
    }
  }
  else if(event == ESP_GATTS_CONF_EVT && param->conf.conn_id == bleLink.connection &&
          param->conf.handle == bleLink.pendingHandle)
  {
    bleLink.pendingHandle = 0;
    if(param->conf.status == ESP_GATT_OK && !bleLink.congested)
      bleLink.blocked = false;
    else if(!bleLink.blocked)
    {
      bleLink.blocked = true;
      bleLink.blockedAt = now;
    }
  }
  portEXIT_CRITICAL(&bleMux);

  if(event == ESP_GATTS_CONNECT_EVT)
    Serial.printf("BLE client connected.\n");
  else if(event == ESP_GATTS_DISCONNECT_EVT)
    Serial.printf("BLE client disconnected (reason 0x%02X).\n", param->disconnect.reason);
  else if(event == ESP_GATTS_CONGEST_EVT && param->congest.congested)
    Serial.printf("BLE congested; pausing notifications.\n");
  else if(event == ESP_GATTS_CONF_EVT && param->conf.status != ESP_GATT_OK)
    Serial.printf("BLE send failed (status 0x%02X).\n", param->conf.status);
}

/* Drain the latest snapshot without delaying sensor sampling or accumulating old data. */
void MYOSA::serviceBle(void)
{
  const uint32_t now = millis();
  portENTER_CRITICAL(&bleMux);
  BleLink state = bleLink;
  portEXIT_CRITICAL(&bleMux);
  if(queueGeneration != state.generation)
  {
    for(uint8_t i = 0; i < fieldCount; ++i)
      bleFields[i].length = 0;
    nextField = 0;
    queueGeneration = state.generation;
  }
  if(!state.connected)
    return;

  const bool stalled = (state.pendingHandle && uint32_t(now - state.pendingAt) >= notificationTimeout) ||
                       (state.blocked && uint32_t(now - state.blockedAt) >= notificationTimeout);
  if((stalled && !state.closing) || (state.closing && uint32_t(now - state.closeAt) >= 3000))
  {
    portENTER_CRITICAL(&bleMux);
    const bool current = bleLink.connected && bleLink.generation == state.generation;
    if(current)
    {
      bleLink.closing = true;
      bleLink.closeAt = now;
    }
    portEXIT_CRITICAL(&bleMux);
    if(current)
    {
      Serial.printf("BLE transmission stalled; closing the link for reconnection.\n");
      esp_ble_gap_disconnect(state.address);
    }
    return;
  }
  if(state.closing || state.congested || state.pendingHandle ||
     uint32_t(now - state.sentAt) < notificationSpacing)
    return;

  for(uint8_t checked = 0; checked < fieldCount; ++checked)
  {
    const uint8_t index = nextField;
    nextField = (nextField + 1) % fieldCount;
    BleField &field = bleFields[index];
    if(!field.length)
      continue;
    portENTER_CRITICAL(&bleMux);
    const bool subscribed = field.subscribed;
    const bool current = bleLink.connected && bleLink.generation == state.generation;
    portEXIT_CRITICAL(&bleMux);
    if(!current)
      return;
    if(!subscribed || field.length > state.mtu - 3)
    {
      field.length = 0; // Never send a truncated multi-value reading.
      continue;
    }
    const uint16_t handle = field.characteristic->getHandle();
    portENTER_CRITICAL(&bleMux);
    const bool ready = bleLink.connected && bleLink.generation == state.generation &&
                       !bleLink.closing && !bleLink.congested && !bleLink.pendingHandle;
    if(ready)
    {
      bleLink.pendingHandle = handle;
      bleLink.pendingAt = bleLink.sentAt = now;
    }
    portEXIT_CRITICAL(&bleMux);
    if(!ready)
      return;
    const esp_err_t result = esp_ble_gatts_send_indicate(bleInterface, state.connection, handle,
        field.length, reinterpret_cast<uint8_t *>(field.payload), false);
    field.length = 0;
    if(result != ESP_OK)
    {
      portENTER_CRITICAL(&bleMux);
      if(bleLink.generation == state.generation)
      {
        bleLink.pendingHandle = 0;
        if(!bleLink.blocked)
        {
          bleLink.blocked = true;
          bleLink.blockedAt = now;
        }
      }
      portEXIT_CRITICAL(&bleMux);
      Serial.printf("BLE notification could not be queued (0x%X).\n", result);
    }
    return;
  }
}

/* Initialize BLE and all boards for existing sketches. */
bool MYOSA::begin(void)
{
  const bool radioReady = beginBle();
  return beginSensors() && radioReady;
}

/* Start the original notification services without changing UUIDs or field order. */
bool MYOSA::beginBle(void)
{
  if(bleReady)
    return true;
  BLEDevice::setCustomGattsHandler(handleBleEvent);
  BLEDevice::init("MYOSA_1");
  // Allow complete multi-value payloads when the BLE client negotiates its packet size.
  BLEDevice::setMTU(128);
  BLEServer *pServer = BLEDevice::createServer();
  if(!pServer)
    return false;
  // The stack restarts advertising after its disconnected peer has been removed.
  pServer->advertiseOnDisconnect(true);

  // Keep existing sensor UUIDs; the optical values share the service ending in b5.
  BLEService *pService = nullptr;
  for(int i = 0; i < NUM_SERVICES; ++i)
  {
    const uint8_t count = characteristicCounts[i];
    if(count == 0)
      continue;
    if(i != 7)
    {
      char serviceUUID[37];
      const int serviceId = i == 6 ? 5 : i;
      sprintf(serviceUUID, "4fafc201-1fb5-459e-8fcc-c5c9c33191b%d", serviceId);
      Serial.println(serviceUUID);
      const uint32_t numHandles = 1 + 3 * (i == 6 ? count + characteristicCounts[7] : count);
      pService = pServer->createService(BLEUUID(serviceUUID), numHandles);
      if(!pService)
        return false;
    }
    for(int j = 0; j < count; ++j)
    {
      char characteristicUUID[37];
      sprintf(characteristicUUID, "beb5483e-36e1-4688-b7f5-ea07361b2b%d%d", i, j);
      Serial.println(characteristicUUID);
      pCharacteristics[i][j] =
          pService->createCharacteristic(characteristicUUID, BLECharacteristic::PROPERTY_NOTIFY);
      if(!pCharacteristics[i][j])
        return false;
      BLE2902 *subscription = new BLE2902();
      pCharacteristics[i][j]->addDescriptor(subscription);
      bleFields[fieldCount].characteristic = pCharacteristics[i][j];
      bleFields[fieldCount].subscription = subscription;
      ++fieldCount;
    }
    // Start the shared optical service only after all of its values are registered.
    if(i != 6 && !pService->start())
      return false;
  }

  pServer->getAdvertising()->start();
  Serial.println("BLE server started!");

  _serviceBle = serviceBle;
  bleReady = true;
  return true;
}

/* Queue one fresh snapshot; update() spaces notifications across the next second. */
void MYOSA::sendBleData(void)
{
  if(!bleReady)
    return;
  serviceBle();
  portENTER_CRITICAL(&bleMux);
  BleLink state = bleLink;
  portEXIT_CRITICAL(&bleMux);
  if(!state.connected || state.closing)
    return;
  MYOSAReadings snapshot;
  readSensors(snapshot);
  portENTER_CRITICAL(&bleMux);
  const bool current = bleLink.connected && bleLink.generation == state.generation;
  portEXIT_CRITICAL(&bleMux);
  if(!current)
    return;
  queueGeneration = state.generation;
  uint8_t fieldIndex = 0;
  const float (&readings)[8][4][3] = snapshot.values;
  for(uint8_t service = 0; service < NUM_SERVICES; ++service)
  {
    for(uint8_t characteristic = 0; characteristic < characteristicCounts[service];
        ++characteristic)
    {
      BleField &field = bleFields[fieldIndex++];
      field.length = 0;
      const uint8_t count = parameterCounts[service][characteristic];
      const float *v = readings[service][characteristic];
      bool valid = true;
      for(uint8_t i = 0; i < count; ++i)
        valid = isfinite(v[i]) && valid;
      if(!valid || !pCharacteristics[service][characteristic])
        continue;
      char payload[96];
      int size;
      const bool integers = service == 6 || (service == 3 && characteristic != 1);
      if(integers && count == 1)
        size = snprintf(payload, sizeof(payload), "%.0f", v[0]);
      else if(integers)
        size = snprintf(payload, sizeof(payload), "%.0f, %.0f, %.0f", v[0], v[1], v[2]);
      else if(count == 1)
        size = snprintf(payload, sizeof(payload), "%.2f", v[0]);
      else if(count == 2)
        size = snprintf(payload, sizeof(payload), "%.2f, %.2f", v[0], v[1]);
      else
        size = snprintf(payload, sizeof(payload), "%.2f, %.2f, %.2f", v[0], v[1], v[2]);
      if(size < 0 || size >= (int)sizeof(payload))
        continue;
      memcpy(field.payload, payload, size);
      field.length = size;
      serviceHeartRateAndSpO2();
    }
  }

}


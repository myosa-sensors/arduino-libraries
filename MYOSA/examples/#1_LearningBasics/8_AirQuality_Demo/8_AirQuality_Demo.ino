/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.
  
  Air Quality Demo
  Connection: Connect the "Air Quality" board from the MYOSA kit with the "Controller" board and power them up.
  Working: Controller board prints (on Serial Monitor) the data of Total Volatile Organic Compounds (TVOCs) and equivalent carbon dioxide (eCO2) every second.

  Synopsis of Air Quality
  The MYOSA air-quality board uses the CCS811 to report eCO2 (ppm) and TVOC (ppb).
  The default I2C address is 0x5B; select 0x5A when the hardware address pin requires it.
  Read algorithm results when data is ready; hasReading() reports valid cached data.

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

/* Library Inclusion */
#include <AirQuality.h>

/* Creating Object of Air Quality Class */
AirQuality Aq(CCS811_I2C_ADDRESS1,refResitance);

/* Setup Function */
void setup() {
	
  /* Setting up communication */
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(100000);
  
  /* Setting up the Air Quality Board. */
  for(;;)
  {
    if(Aq.begin() == SENSOR_SUCCESS)
    {
      Serial.println("Air Quality sensor CCS811 is connected...");
      break;
    }
    Serial.println("Air Quality sensor ccs811 is disconnected...");
    delay(500u);
  }
  
  Serial.println("\nDevice Specifications");
  Serial.print("DEVICE ID       : 0x");
  Serial.println(Aq.getHwId(),HEX);
  Serial.print("HW VERSION      : ");
  Serial.println(Aq.getHwVersion());
  Serial.print("FW BOOT VERSION : ");
  Serial.println(Aq.getFwBootVersion());
  Serial.print("FW APP VERSION  : ");
  Serial.println(Aq.getFwAppVersion());
  Serial.println();
}

/* Loop function */
void loop() {
	
  /* Loop function continously prints data from the sensor every 1 second */
  if(Aq.ping())
  {
    /* Check if data is ready or not */
    if(Aq.isDataAvailable())
    {
      if(Aq.readAlgorithmResults() == SENSOR_SUCCESS)
      {
        Aq.getCO2();
        Aq.getTVOC();
        Serial.println();
      }
    }
  }
  delay(1010);
}
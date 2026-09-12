/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.

  BarometricPressure Demo
  Connection: Connect the "Barometric Pressure" board from the MYOSA kit with the "Controller" board and power them up.
  Working: Controller board prints (on Serial Monitor) the data of Temperature, Pressure and Altitude every second.  
  
  Synopsis of Barometric Pressure Board
  The MYOSA pressure board uses the BMP180 sensor at I2C address 0x77.
  Factory coefficients compensate the temperature and pressure measurements.
  Pressure is available in kPa, mmHg and mbar, with altitude estimates in metres.
  Floating measurements return NAN if a transaction or compensation calculation fails.

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
#include <BarometricPressure.h>

/* Creating Object of BarometricPressure Class */
BarometricPressure Pr(ULTRA_HIGH_RESOLUTION);
float altitude;

/* Setup Function */
void setup() {

  /* Setting up communication */
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(100000);
  
  /* Setting up the BarometricPressure Board. */
  for(;;)
  {
    if(Pr.begin() == true)
    {
      Serial.println("Barometric Pressure Sensor is connected");
      break;
    }
    Serial.println("Barometric Pressure Sensor is disconnected");
    delay(500u);
  }
}

/* Loop Function */
void loop() {
  
  /* Loop function continuously gets data and print at every 1 second */
  if(Pr.ping())
  {
    Pr.getTempC();
    Pr.getTempF();
    Pr.getPressurePascal();
    Pr.getPressureHg();
    Pr.getPressureBar();
    altitude = Pr.getAltitude(SEA_LEVEL_AVG_PRESSURE);
    Pr.getSeaLevelPressure(altitude);
    Serial.println();
  }
  delay(1000u);
}
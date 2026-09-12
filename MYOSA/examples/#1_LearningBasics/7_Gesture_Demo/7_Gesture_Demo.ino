/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.

  Gesture Demo
  Connection: Connect the "Light Proximity and Gesture" board from the MYOSA kit with the "Controller" board and power them up.
  Working: Controller board prints (on Serial Monitor) the gesture detected continuously.
  
  Synopsis of Light Proximity and Gesture Board
  The MYOSA light and gesture board uses the APDS9960 sensor at I2C address 0x39.
  Ambient light is reported as raw counts, RGB as percentages, and proximity as raw counts.
  Gesture sensing reports direction, near/far motion and timeout status.
  Successful configuration settings are restored when the board reconnects.

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
#include <LightProximityAndGesture.h>

/* Creating Object of LightProximityAndGesture Class */
LightProximityAndGesture Lpg;

/* Setup Function */
void setup() {

  /* Setting up communication */
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(100000);
  
  /* Setting up the LightProximityAndGesture Board. */
  for(;;)
  {
    if(Lpg.begin())
    {
      Serial.println("Proximity, Ambient Light, RGB & Gesture sensor is connected...");
      break;
    }
    Serial.println("Proximity, Ambient Light, RGB & Gesture sensor is disconnected...");
    delay(500u);
  }
  Serial.println("APDS9960 initialization completed");

  /* Start running the gesture sensor engine */
  if( Lpg.enableGestureSensor(DISABLE) )
  {
    Serial.println("Gesture sensor is now running");
  }
  else
  {
    Serial.println("Something went wrong during gesture sensor init!");
  }

  /* Wait for initialization and calibration to finish */
  delay(500u);
}

/* Loop Function */
void loop() {

  /* Loop function continuously reads gesture data and print */
  if(Lpg.ping())
  {
    Lpg.getGesture();
  }
}
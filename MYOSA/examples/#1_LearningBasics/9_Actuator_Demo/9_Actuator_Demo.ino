/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.
  
  Actuator Demo
  Connection: Connect the "Actuator" board from the MYOSA kit with the "Controller" board and power them up.
  Working: Turns Buzzer and AC switching ckt ON for 1 second and then turns it OFF.

  Synopsis of Actuator Board
  The MYOSA actuator board uses the PCA9536 four-bit I2C GPIO expander at 0x41.
  IO0 controls the AC switching output; IO1 controls the buzzer.
  IO2 and IO3 are available for user configuration.
  Output latches are set before changing pin direction to avoid startup pulses.

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
#include <Actuator.h>

/* Creating Object of Actuator Class */
Actuator gpioExpander;

/* Setup Function */
void setup() {

  /* Setting up communication */
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(100000);
  
  /* Setting up the Actuator Board. */
  for(;;)
  {
    if(gpioExpander.ping())
    {
      Serial.println("4bit IO Expander Actautor (PCA9536) is connected");
      break;
    }
    Serial.println("4bit IO Expander Actuator (PCA9536) is disconnected");
    delay(500u);
  }

  /* Set AC SWITCH IO as output */  gpioExpander.setState(AC_SWITCH_IO, IO_LOW);

  gpioExpander.setMode(AC_SWITCH_IO, IO_OUTPUT);

  /* Set BUZZER IO as output */  gpioExpander.setState(BUZZER_IO, IO_LOW);

  gpioExpander.setMode(BUZZER_IO, IO_OUTPUT);
  delay(2000);
  
  /* Turn-on AC SWITCH for one second */
  gpioExpander.setState(AC_SWITCH_IO, IO_HIGH);
  delay(1000);
  gpioExpander.setState(AC_SWITCH_IO, IO_LOW);
  delay(1000);
  
  /* Turn-on BUZZER for one second */
  gpioExpander.setState(BUZZER_IO, IO_HIGH);
  delay(1000);
  gpioExpander.setState(BUZZER_IO, IO_LOW);
  delay(1000);
}

/* Loop Function */
void loop() {

  /* Loop function does nothing */
  delay(1000u);
}
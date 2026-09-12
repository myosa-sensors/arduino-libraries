/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.
  
  Blink Demo
  Connection: Connect the "Controller" board from the MYOSA kit power it up.
  Working: Blue status LED on the Controller board starts blinking for every 1 second.

  Synopsis of "Controller" board
  MYOSA Platform consists of a centralized motherboard also known as Controller board. It is board designed of the ESP-Wroom32 module.
  ESP32 is a low-power system on a chip microcontrollers with integrated Wi-Fi and Bluetooth.
  It also has lot many GPIO pins along with famous communication protocols like i2c, uart, spi, etc. which can be used for interfacing with sensors and other modules.

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

/* Setup Function */
void setup() {

  /* Setting up Status LED pin as OUTPUT pin */
  pinMode(2, OUTPUT);
}

/* Loop Function */
void loop() {

  /* Loop function continously blinks the status LED on the Controller Board */
  digitalWrite(2,HIGH);
  delay(1000);
  digitalWrite(2,LOW);
  delay(1000);
}

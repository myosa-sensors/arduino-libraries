/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.
  
  Accelerometer and Gyroscope Demo
  Connection: Connect the "Accelerometer and Gyroscope" board from the MYOSA kit with the "Controller" board and power them up.
  Working: Controller board prints (on Serial Monitor) the (RAW) data of 3-axis Accelerometer & Gyroscope values every 5 seconds.

  Synopsis of Accelerometer and Gyroscope
  MYOSA Platform consists of an Accelerometer and Gyroscope Board. It is equipped with GY521/MPU6050 IC.
  MPU6050 provides a general X/Y/Z direction (3-axis) accelerometer and gyroscope.
  I2C Address of the board = 0x69.

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
#include <AccelAndGyro.h>

/* Creating Object of AccelAndGyro Class */
AccelAndGyro Ag;

/* Setup Function */
void setup() {
	
	/* Setting up communication */
	Serial.begin(115200);
    Wire.begin();
    Wire.setClock(100000);
	
	/* Setting up the AccelAndGyro Board. */
	for(;;)
	{
		if(Ag.begin() == true)
		{
			Serial.println("Accelerometer and Gyroscope Sensor is connected");
			break;
		}
		Serial.println("Accelerometer and Gyroscope Sensor is disconnected");
		delay(500u);
	}
}

/* Loop Function */
void loop() {
	
	/* Loop function continously prints RAW data from the sensor every 5 seconds */
	if(Ag.ping())
	{
		Ag.getAccelX();
		Ag.getAccelY();
		Ag.getAccelZ();
		Ag.getGyroX();
		Ag.getGyroY();
		Ag.getGyroZ();
		Ag.getTempC();
		Ag.getTempF();
		Ag.getTiltX();
		Ag.getTiltY();
		Ag.getTiltZ();
		Ag.getMotionStatus();
		Serial.println();
	}
	delay(5000u);
}

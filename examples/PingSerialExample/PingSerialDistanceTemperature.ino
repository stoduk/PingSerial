/*
 * PingSerialDistanceTemperature.ino - Example Arduino script for using PingSerial library with US-100 ultrasonic distance meter for distance and temperature
 * Created by Anthony Toole, 17 March 2016
 * Released under MIT License, see LICENSE file.
 * 
 * This example script is a basic example of how to use PingSerial library to talk to a US-100 distance module.
 * In this example the US-100 is connected to the hardware serial port Serial1, and the hardware serial port Serial is used for debugging.
 * Both distance and temperature are requested, and it also allows debugging information to be sent to Arduino IDE's Serial Monitor.
 */

#include <PingSerial.h>
#include <SoftwareSerial.h> // Arduino issue: a library can't include other libraries, the sketch has to do this. Avoid by not using Arduino!

// Here US-100 is connected to Serial1 on Leonardo, so we have debugging on a Serial port (which the Arduino IDE can talk to directly)
PingSerial us100(Serial1, 650, 1200);  // Valid measurements are 650-1200mm

bool ping_enabled = TRUE;
unsigned int pingSpeed = 100; // How frequently are we going to send out a ping (in milliseconds). 50ms would be 20 times a second.
unsigned long pingTimer = 0;     // Holds the next ping time.

bool temp_enabled = TRUE;
unsigned int tempSpeed = 3000;
unsigned long tempTimer = 0;

void setup() {
  // put your setup code here, to run once:
    Serial.begin(115200); // Serial over USB, talk to serial monitor
    
    while(!Serial) {  
    }

    us100.begin();
    Serial.println("Booting..");
}

void loop() {
  byte data_available;
  byte msg;

  /*
   * Note: none of this code is blocking (no calls to delay() for example)
   * so your Arduino can do other things while measurements are being made. 
   * Quite useful for any real world examples!
   */
  if (Serial.available()) {
      msg = Serial.read();
      if (msg == 'd') {
          us100.display_debugging();
      } else if (msg == 'D') {
          us100.display_debugging(TRUE);
      }
  }
  
  data_available = us100.data_available();

  if (data_available & DISTANCE) {
      Serial.print("Distance: ");
      Serial.println(us100.get_distance());
  }
  if (data_available & TEMPERATURE) {
      Serial.print("Temperature: ");
      Serial.println(us100.get_temperature());
  }

  if (ping_enabled && (millis() >= pingTimer)) {   // pingSpeed milliseconds since last ping, do another ping.
      pingTimer = millis() + pingSpeed;      // Set the next ping time.
      us100.request_distance();
      //Serial.println("Distance requested");
  }

  if (temp_enabled && (millis() >= tempTimer)) {
      tempTimer = millis() + tempSpeed;
      us100.request_temperature();
      //Serial.println("Temperature requested");
  }

  // Do other work here..  Don't block unnecessarily (eg. through call to delay()).
}

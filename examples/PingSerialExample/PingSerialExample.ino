/*
 * PingSerialExample.ino - Example Arduino script for using PingSerial library with US-100 ultrasonic distance meter
 * Created by Anthony Toole, 17 March 2016
 * Released under MIT License, see LICENSE file.
 */
#include <PingSerial.h>
#include <SoftwareSerial.h>

/*
 * This example script is a basic example of how to use PingSerial library to talk to a US-100 distance module.
 * In this example the US-100 is connected to the hardware serial port Serial, and a SoftwareSerial port is used for debugging (if required).
 */

// Here US-100 is connected to Serial, so we have debugging on a SoftwareSerial port (eg. connected to Bluetooth module or TTL-USB adaptor).
PingSerial us100(Serial, 650, 1200);  // Valid measurements are 650-1200mm
SoftwareSerial SerialDbg(4, 5);       // SoftwareSerial for debugging from this script (*not* 

bool ping_enabled = FALSE;
unsigned int pingSpeed = 100; // How frequently are we going to send out a ping (in milliseconds). 50ms would be 20 times a second.
unsigned long pingTimer = 0;     // Holds the next ping time.

void setup() {
  us100.begin();
  SerialDbg.begin(9600);
}

void loop() {
  byte data_available;
  unsigned int current_height = 0;

  data_available = us100.data_available();

  if (data_available & DISTANCE) {
      current_height = us100.get_distance();
      SerialDbg.print("Distance: ");
      SerialDbg.println(current_height);
  }
  if (data_available & TEMPERATURE) {
      SerialDbg.print("Temperature: ");
      SerialDbg.println(us100.get_temperature());
  }
  
  // Notice how there's no delays in this sketch to allow you to do other processing in-line while doing distance pings.
  if (ping_enabled && (millis() >= pingTimer)) {   // pingSpeed milliseconds since last ping, do another ping.
      pingTimer = millis() + pingSpeed;      // Set the next ping time.
      us100.request_distance();
  }
}

# PingSerial
Arduino library for using serial enabled ultrasonic distance modules (eg. US-100)

## Background
There are two common types of cheap ultrasonic distance modules listed below, which are both very similar from a hardware perspective.  They both have an ultrasonic transmitter and a matching receiver, with a small amount of logic on board to talk to the connected microcontroller (such as an Arduino).  They both claim similar specifications (ranging distance from 20-4500mm, with resolution of a few mm, and a measuring angle of 15 degrees)

This library assumes that, unlike the rather simplistic first-attempt examples of how to use these devices, the devices will be used in a complex environment where the controller cannot block for any significant time and where accuracy and precision of measurements matter (ie. both the right measurement is returned, and that measurement has small variation if repeated).

### Trigger/Echo models
The cheapest and most common devices are the family of devices that use trigger/echo pins to make measurements.  HC-SR04 is a common example sold on ebay, though there are a large number of devices out there.  Sometimes a different model number just indicates a different design, manufacturer, or Chinese clone, while sometimes there are actual differences in the behaviour.

The devices operate by the microcontroller signaling to the device to perform a measurement by toggling the trigger line briefly (10s of microseconds), and then waiting for the echo pin to be toggled to indicate the ultrasonic pulse has been returned.  Measuring the time the echo takes allows us to infer the distance to the nearest object.

The problem here is twofold:
1. Far too much sample code does this timing in a blocking way (either using pulseIn(), which is a blocking API, or using the NewPing library in a blocking way), which is not useful for real world devices..
2. The NewPing library (which is very good, and supports a wide range of devices!) can be used in a timer interrupt driven way, which avoids the blocking issues.  However, the accuracy and precision is limited by how quickly the change is pin state will be noticed.  Measurements tend to be slightly high, and vary noticeably on repeated measurements, both due to the variable delay in noticing the echo pin state change affecting the perceived echo timing.

It may be possible to use these devices with pin-change interrupts, but that would not necessarily fix all the issues.

Though these are the cheapest devices by far (£1.39 including postage from China, as of July 2016) I cannot recommend using them, given the relative cost and far superior performance of the serial models.

This library does not handle these devices - the NewPing library already exists for that and is very good.

### Serial models
The less common and more expensive devices are the family of devices that use a TTL serial communication to make measurements.  US-100 is a common example sold on ebay.

These cost fractionally more than the Trigger/Echo models (£2.32 including postage from China, as of July 2016), but I cannot imagine a usage scenario where this difference in cost will be an issue, given the far superior performance of the US-100 and similar models.

There are two main benefits to these models:
 1. communication is entirely via TTL serial.  A measurement is requested and will be returned once it has been made.  This gives far greater accuracy and repeatability, as the device itself is timing the ultrasonic pulses and is not succeptible to delays as an external microcontroller would be.
 1. temperature measurement is possible on-board, allowing for tmperature compensation of the speed of the ultrasonic pulses.  It is unclear how significant this is.

These devices are not without foibles however!  This library aims to hide most of these from the user.  The most obvious weaknesses are:
 * the device returned a value without telling you what measurement it is returning (distance or temperature).  Nothing is stopping you requesting both types of measurements at once, so the library effectively blocks this so we always know what measurement is expected next.  Any pending operation will be requested as soon as possible after the last operation is returned.
 * if one operation is requested too soon after the last operation completed, then the device will ignore the new request.  The library simply ensures the device has a moment between operations to make it happy.
 * there are multiple timeout modes!  You may get no response at all or you will get a large distance reported (various fixed values above 10m, which is outside the range of the device).

Note: US-100 in particular can operate in either Trigger/Echo mode or Serial mode.  Thought if you've got the device I think you'd be mad to use it in the Trigger/Echo mode for the reasons already given!

## Installation
Download the zip file (link below) then install as you would any other library (instructions below).

## Examples
Two examples are provided:
 1. PingSerialDistance: Only requests distance measurements, and uses the hardware serial port on a non-Leonardo device (which requires a SoftwareSerial if your sketch needs to output debugging information over a serial port)
 1. PingSerialDistanceTemperature: Requests both distance and temperature measurements, while using both Leonardo's hardware serial port (Serial1) for the US-100 plus the serial-over-USB Serial port for debugging information

## Caveats
 1. To use a SoftwareSerial to talk to the US-100 requires manually modifying the PingSerial.h file to add ```#define PS_INCLUDE_SOFTWARESERIAL``` before the ```#ifdef PS_INCLUDE_SOFTWARESERIAL``` check.  If anyone has better ideas on how to do this, I'm all ears.
 
## References
 * Download link: https://github.com/stoduk/PingSerial/archive/master.zip
 * How to install libraries (see section on installing from zip file): https://www.arduino.cc/en/Guide/Libraries.  
 * NewPing library - very good for Trigger/Echo devices: http://playground.arduino.cc/Code/NewPing
 * US-100 on Banana Robotics (has some background details): https://www.bananarobotics.com/shop/US-100-Ultrasonic-Distance-Sensor-Module
 * Comparison of US-100 and HC-SR04 devices (also see comments): https://arduibots.wordpress.com/2013/10/14/comparison-of-ultrasonic-sensors-us-100-and-hc-sr04/ and https://arduibots.wordpress.com/2014/10/12/us-100-ultrasonic-sensor-in-serial-mode/

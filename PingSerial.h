#ifndef __PINGSERIAL_H__
#define __PINGSERIAL_H__

/*
 * PingSerial.h - Library for interacting with Serial enabled ultrasonic distance modules (currently just US-100).
 * Created by Anthony Toole, 17 March 2016
 * Released under MIT License, see LICENSE file.
 */
#define __STDC_LIMIT_MACROS // need UINT16_MAX
#include <stdint.h>

#ifdef PS_INCLUDE_SOFTWARESERIAL
#include <SoftwareSerial.h>
#endif

#if defined (ARDUINO) && ARDUINO >= 100
  #include <Arduino.h>
#else
  #include <WProgram.h>
  #include <pins_arduino.h>
#endif

#if defined (__AVR__)
  #include <avr/io.h>
  #include <avr/interrupt.h>
#endif

/*
 * Returned by data_available() to indicate which values have been requested previously and are now available
 */
#define DISTANCE    0x1
#define TEMPERATURE 0x2

/*
 * Yeah, just because.
 */
#define TRUE (1 == 1)
#define FALSE (!TRUE)

/*
 * If you want debug enabled, define PS_ENABLE_DEBUG in your sketch
 * If you can't use Serial (eg. on Leonardo, where you want Serial1) then
 * you've got to edit this library header file to change this.
 *
 * If anyone has neater ideas, answers on a postcard, please.
 */
//#define PS_ENABLE_DEBUG
#ifdef PS_ENABLE_DEBUG
#define DBG_PRINT(args) {Serial.print(args);}
#define DBG_PRINTLN(args) {Serial.println(args);}
#else
#define DBG_PRINT(args) {}
#define DBG_PRINTLN(args) {}
#endif

class PingSerial {
  public:
      /*
       * If the sensor is connected to a hardware serial port then pass that to the constructor, else
       * pass in the pin details and a SoftwareSerial port will be created.
       *
       * Max distance sensor can handle is 5m=5000mm, so a uint16 is plenty (max 65,536mm).
       */
      PingSerial(HardwareSerial& serialport, uint16_t min_mm_distance = 0, uint16_t max_mm_distance = 500);
#ifdef PS_INCLUDE_SOFTWARESERIAL
      PingSerial(SoftwareSerial& serialport, uint16_t min_mm_distance = 0, uint16_t max_mm_distance = 500);
      PingSerial(byte rx_pin, byte tx_pin, uint16_t min_mm_distance = 0, uint16_t max_mm_distance = 500);
#endif

      // begin() must be called from setup loop, primarily to open and set the baud rate of the serial conncetion
      void begin(void);

      // Reads data from serial connection to US-100.
      // Returns value with DISTANCE or TEMPERATURE flags set indicating which values are available
      // (not necessarily just received, the flags are cleared only when the values are retrieved). 
      byte data_available(void);

      // Get the stored values, if data_available() indicates a value is available.  Returns zero on error (which could be a valid temperature).
      uint16_t get_distance(void);
      int get_temperature(void);

      // Send request to US-100 for distance or temperature.  This may not be sent immediately, but will be queued if required.
      // (it won't be sent if another request is pending, nor if a request has only just been received as US-100 seems to like a brief pause)
      void request_distance(void);
      void request_temperature(void);

      // Prints some useful internal state, see DBG_PRINT for details on how to enable this.
      void display_debugging(bool clear=FALSE);

  private:
      void Init(uint16_t min_mm_distance, uint16_t max_mm_distance);

      // Wrappers to call underlying function on hardware or software serial object
      int read(uint8_t count); // adds 'count' argument; if not 1 then all read bytes will be discarded
      size_t write(uint8_t b);

      // One of these will be valid.
      HardwareSerial *_hw_serial = NULL;
#ifdef PS_INCLUDE_SOFTWARESERIAL
      SoftwareSerial *_sw_serial = NULL;
#else
      HardwareSerial *_sw_serial = NULL;
#endif

      // Internal state, mostly for debugging purposes (see display_debugging())
      unsigned long _op_started = 0;
      uint16_t      _max_op_duration_ms = 0;
      uint16_t      _op_timeout_ms = 0;
      uint16_t      _timeout_count = 0;

      // Stored values
      uint16_t _distance = 0; // Distance is always positive, max (255 * 256 + 255) = 65535
      int _temperature = 0; // Temperature returned is (byte) - 45 (so ranges from -45 to 256-45)

      // If values are out of range then distance returned will be 0 or UINT16_MAX respectively 
      uint16_t _min_mm_distance = 0;
      uint16_t _max_mm_distance = UINT16_MAX;

      // Indicates if a request has been sent to US-100, and neither response received or timed out
      bool _distance_pending = FALSE;
      bool _temperature_pending = FALSE;

      // Indicates if a value has been received from US-100.  Cleared once read.
      bool _distance_avail = FALSE;
      bool _temperature_avail = FALSE;

      // Indicates if a request was made while an existing operation is in progress
      bool _distance_request_queued = FALSE;
      bool _temperature_request_queued = FALSE;
};
#endif __PINGSERIAL_H__

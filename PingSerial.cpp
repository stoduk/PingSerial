/*
 * PingSerial.cpp - Library for interacting with Serial enabled ultrasonic distance modules (currently just US-100).
 * Created by Anthony Toole, 17 March 2016
 * Released under MIT License, see LICENSE file.
 */

#include "PingSerial.h"

#define OPERATION_PENDING (_distance_pending || _temperature_pending)

void
PingSerial::Init (uint16_t min_mm_distance,
                  uint16_t max_mm_distance)
{
    /*
     * From testing with a US-100:
     * - around ~11ms overhead for any measurement to be returned
     *   [3ms of that will be sending/receiving 3 chars @ 9600 baud]
     * - sound takes 5.7us per mm round trip
     * The above combined should give us an upper bound on how long an
     * operation can take, but it seems we have two long-distance-timeout
     * modes - one where no value is returned (ie. genuine timeout), and one
     * where after 91ms a bogus large number is returned (11090 or 11110mm).
     *
     * Given a max range of 3.5m, this gives 11ms + 20ms = 31ms.
     *
     * Set the timeout to be the max of these two, ie. 91ms plus a bit.
     * Why US-100 takes so long to timeout I don't know..
     */
    _op_timeout_ms = 99;

    // If we've got garbage, at least make sure min < max
    if (_min_mm_distance >= max_mm_distance) {
        DBG_PRINT("Screwy min/max distance passed: ");
        DBG_PRINT(min_mm_distance);
        DBG_PRINT("/");
        DBG_PRINTLN(max_mm_distance);
    }
    _min_mm_distance = min(min_mm_distance, max_mm_distance);
    _max_mm_distance = max(max_mm_distance, min_mm_distance);
}

PingSerial::PingSerial (HardwareSerial& serialport,
                        uint16_t    min_mm_distance,
                        uint16_t    max_mm_distance)
{
    _hw_serial = &serialport;
    Init(min_mm_distance, max_mm_distance);
}

#ifdef PS_INCLUDE_SOFTWARESERIAL
PingSerial::PingSerial (SoftwareSerial& serialport,
                        uint16_t    min_mm_distance,
                        uint16_t    max_mm_distance)
{
    _sw_serial = &serialport;
    Init(min_mm_distance, max_mm_distance);
}
PingSerial::PingSerial (byte         rx_pin,
                        byte         tx_pin,
                        uint16_t min_mm_distance,
                        uint16_t max_mm_distance)
{
    _sw_serial = new SoftwareSerial(rx_pin, tx_pin);
    Init(min_mm_distance, max_mm_distance);
}
#endif

int
PingSerial::read (uint8_t count=1)
{
    // A read of more than one byte implicitly discards
    // the values.
    uint8_t i;

    if (_hw_serial) {
        if (count == 1) {
            return _hw_serial->read();
        } else {
            for (i = 0; i < count; i++) {
                _hw_serial->read();
            }    
        }
    } else {
        if (count == 1) {
            return _sw_serial->read();
        } else {
            for (i = 0; i < count; i++) {
                _sw_serial->read();
            }
        }
    }

    return 0; // Multi-byte read, return anything, it will be ignored.
}

size_t
PingSerial::write (uint8_t b)
{
    if (_hw_serial) {
        return _hw_serial->write(b);
    } else {
        return _sw_serial->write(b);
    }
}

void
PingSerial::begin(void)
{
    if (_hw_serial) {
        _hw_serial->begin(9600);
    } else {
        _sw_serial->begin(9600);
    }
    DBG_PRINTLN("Library beginning");
}

byte
PingSerial::data_available (void)
{
    byte retval = 0;
    byte available;

    // If serial data available, read it in and validate it
    if (_hw_serial) {
        available = _hw_serial->available();
    } else {
        available = _sw_serial->available();
    }

    /*
     * If a message is queued then we'd ideally send it immediately after receiving
     * the last response - but US-100 seems to regularly drop messages it receives
     * too quickly..  So handle it on the next loop around, assuming no other operation
     * has been triggered and we haven't got serial data.
     * (we could inline sleep below, as even printing something to serial is delay enough,
     * but assume it is better to avoid any possible blocking at the cost of a tiny delay)
     */
    if (!available && !OPERATION_PENDING) {
        if (_distance_request_queued) {
            _distance_request_queued = FALSE;
            request_distance();
        } else if (_temperature_request_queued) {
            _temperature_request_queued = FALSE;
            request_temperature();
        }
    }

    if (available) {
        if (_distance_pending) {
            if (available < 2) {
                // Only got one byte, wait
            } else if (available > 2) {
                // Something is screwy - garbage from US-100, noise on serial port, or a bug here
                DBG_PRINT("Distance available too large: ");
                DBG_PRINTLN(available);
                (void) read(available); // read and discard all available
                _distance_pending = FALSE;
                request_distance();
            } else {
                // Got valid data
                _max_op_duration_ms = max(millis() - _op_started, _max_op_duration_ms);
                _distance = (read() * 256) + read();
                _distance_pending = FALSE;
                _distance_avail = TRUE;
                if (_distance > _max_mm_distance) {
                    _distance = UINT16_MAX;
                } else if (_distance < _min_mm_distance) {
                    _distance = 0;
                }
            }
        } else if (_temperature_pending) {
            if (available > 1) {
                // As above, something screwy again, clear it out and try again
                DBG_PRINT("Temperature available too large: ");
                DBG_PRINTLN(available);
                (void) read(available); // read and discard all available
                _temperature_pending = FALSE;
                request_temperature();
            } else {
                // Got valid data
                _temperature = read() - 45; // 45 is magic number for US-100
                _temperature_pending = FALSE;
                _temperature_avail = TRUE;
            }
        }
    }

    if (OPERATION_PENDING && (millis() > _op_started + _op_timeout_ms)) {
        // Operation timed out - discard all pending serial data, and kick it off again

        // Increment counter but make sure if we've looped we set it to 1
        // (we've lost info, but better than thinking we've had no timeouts)
        _timeout_count++;
        _timeout_count = max(_timeout_count, 1); 

        if (_hw_serial) {
            available = _hw_serial->available();
        } else {
            available = _sw_serial->available();
        }
        (void) read(available);

        DBG_PRINT("Operation timed out (");
        DBG_PRINT(available);
        DBG_PRINT(" rx) - retry ");
        if (_distance_pending) {
            DBG_PRINTLN("distance");
            _distance_pending = FALSE;
            request_distance();
        } else {
            DBG_PRINTLN("temperature");
            _temperature_pending = FALSE;
            request_temperature();
        }
    }

    // If we just found data or already had it, report that
    if (_distance_avail) {
        retval = retval | DISTANCE;
    }
    if (_temperature_avail) {
        retval = retval | TEMPERATURE;
    }

    return (retval);
}

uint16_t
PingSerial::get_distance (void)
{
    if (_distance_avail) {
        _distance_avail = FALSE;
        return(_distance);
    } else {
        return(0);
    }
}

int
PingSerial::get_temperature (void)
{
    if (_temperature_avail) {
        _temperature_avail = FALSE;
        return(_temperature);
    } else {
        return(0);
    }
}

void
PingSerial:: request_distance (void)
{
    if (OPERATION_PENDING) {
        _distance_request_queued = TRUE; // could already be set, doesn't matter if we overwrite.
    } else {
        write(0x55);
        _distance_pending = TRUE;
        _op_started = millis();
    }
}

void
PingSerial::request_temperature (void)
{
    if (OPERATION_PENDING) {
        _temperature_request_queued = TRUE; // could already be set, doesn't matter if we overwrite.
    } else {
        write(0x50);
        _temperature_pending = TRUE;
        _op_started = millis();
    }
}

void
PingSerial::display_debugging (bool clear)
{
    DBG_PRINT("Max op duration: ");
    DBG_PRINT(_max_op_duration_ms);
    DBG_PRINTLN("ms");
    DBG_PRINT("Timeouts hit: ");
    DBG_PRINTLN(_timeout_count);
    if (clear) {
        DBG_PRINTLN("Debug values cleared");
        _max_op_duration_ms = 0;
        _timeout_count = 0;
    }
}

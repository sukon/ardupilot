/*
  LP5009 I2C driver
*/
/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* LED driver for LP5009 */

#include "LP5009.h"

#if AP_NOTIFY_LP5009_ENABLED

#include <AP_HAL/AP_HAL.h>

extern const AP_HAL::HAL& hal;

#define LP5009_LED_BRIGHT  255    // full brightness
#define LP5009_LED_MEDIUM  170    // medium brightness
#define LP5009_LED_DIM     85     // dim
#define LP5009_LED_OFF     0      // off

enum class Register {
    DEVICE_CONFIG0   = 0x00,
    DEVICE_CONFIG1   = 0x01,
    LED_CONFIG0      = 0x02,
    BANK_BRIGHTNESS  = 0x03,
    BANK_A_COLOR     = 0x04,
    BANK_B_COLOR     = 0x05,
    BANK_C_COLOR     = 0x06,
    LED0_BRIGHTNESS  = 0x07,
    LED1_BRIGHTNESS  = 0x08,
    LED2_BRIGHTNESS  = 0x09,
    OUT0_COLOR       = 0x0B,
    OUT1_COLOR       = 0x0C,
    OUT2_COLOR       = 0x0D,
    RESET            = 0x17,
};

// DEVICE_CONFIG1 reset value per datasheet: Log_Scale_EN=1, Power_Save_EN=1,
// Auto_Incr_EN=1, PWM_Dithering_EN=1, Max_Current_Option=0, LED_Global_Off=0
#define LP5009_DEVICE_CONFIG1_RESET_VALUE  0x3C

// DEVICE_CONFIG1 value we run with: as above but with Log_Scale_EN cleared
// so PWM output is linear, matching every other RGBLed backend
#define LP5009_DEVICE_CONFIG1_RUN_VALUE    0x1C

LP5009::LP5009(uint8_t bus, uint8_t addr)
    : RGBLed(LP5009_LED_OFF, LP5009_LED_BRIGHT, LP5009_LED_MEDIUM, LP5009_LED_DIM)
    , _bus(bus)
    , _addr(addr)
{
}

bool LP5009::init(void)
{
    _dev = hal.i2c_mgr->get_device_ptr(_bus, _addr);
    if (!_dev) {
        return false;
    }

    if (!configure_dev()) {
        delete _dev;
        _dev = nullptr;
        return false;
    }

    _dev->register_periodic_callback(20000, FUNCTOR_BIND_MEMBER(&LP5009::_timer, void));

    return true;
}

bool LP5009::configure_dev()
{
    WITH_SEMAPHORE(_dev->get_semaphore());

    _dev->set_retries(10);

    // reset the device and probe to see if this device looks like an LP5009:
    if (!_dev->write_register((uint8_t)Register::RESET, 0xff)) {
        return false;
    }

    // reset delay; unsure if this is really required:
    hal.scheduler->delay_microseconds(100);

    // check DEVICE_CONFIG1 has its reset value:
    uint8_t value;
    if (!_dev->read_registers((uint8_t)Register::DEVICE_CONFIG1, &value, 1)) {
        return false;
    }
    if (value != LP5009_DEVICE_CONFIG1_RESET_VALUE) {
        return false;
    }

    // chip enable:
    if (!_dev->write_register((uint8_t)Register::DEVICE_CONFIG0, 0b1000000)) {
        return false;
    }

    // start-up delay:
    hal.scheduler->delay_microseconds(500);

    // linear (non-log-scale) dimming, auto-increment left enabled:
    if (!_dev->write_register((uint8_t)Register::DEVICE_CONFIG1, LP5009_DEVICE_CONFIG1_RUN_VALUE)) {
        return false;
    }

    // independent (non-bank) control for LED0/1/2:
    if (!_dev->write_register((uint8_t)Register::LED_CONFIG0, 0b00000000)) {
        return false;
    }

    _dev->set_retries(1);

    return true;
}

// set_rgb - set color as a combination of red, green and blue values
bool LP5009::hw_set_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    rgb[0] = red;
    rgb[1] = green;
    rgb[2] = blue;
    _need_update = true;
    return true;
}

void LP5009::_timer(void)
{
    if (!_need_update) {
        return;
    }
    _need_update = false;

    for (uint8_t i=0; i<ARRAY_SIZE(rgb); i++) {
        const uint8_t new_colour = rgb[i];
        const uint8_t last_sent = last_sent_rgb[i];
        if (new_colour == last_sent) {
            continue;
        }
        // take advantage of the linear layout of the registers.  OUT0_COLOR
        // (red), OUT1_COLOR (green) and OUT2_COLOR (blue) are contiguous:
        _dev->write_register((uint8_t)Register::OUT0_COLOR + i, new_colour);

        last_sent_rgb[i] = new_colour;
    }
}

#endif  // AP_NOTIFY_LP5009_ENABLED

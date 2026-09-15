#include "SIM_config.h"

#if AP_SIM_LP5009_ENABLED

#include "SIM_LP5009.h"

using namespace SITL;

#include <stdio.h>

void LP5009::init()
{
    add_register("DEVICE_CONFIG0", LP5009DevReg::DEVICE_CONFIG0, I2CRegisters::RegMode::RDWR);
    add_register("DEVICE_CONFIG1", LP5009DevReg::DEVICE_CONFIG1, I2CRegisters::RegMode::RDWR);
    add_register("LED_CONFIG0", LP5009DevReg::LED_CONFIG0, I2CRegisters::RegMode::RDWR);

    add_register("LED0_BRIGHTNESS", LP5009DevReg::LED0_BRIGHTNESS, I2CRegisters::RegMode::RDWR);
    add_register("LED1_BRIGHTNESS", LP5009DevReg::LED1_BRIGHTNESS, I2CRegisters::RegMode::RDWR);
    add_register("LED2_BRIGHTNESS", LP5009DevReg::LED2_BRIGHTNESS, I2CRegisters::RegMode::RDWR);

    add_register("OUT0_COLOR", LP5009DevReg::OUT0_COLOR, I2CRegisters::RegMode::RDWR);
    add_register("OUT1_COLOR", LP5009DevReg::OUT1_COLOR, I2CRegisters::RegMode::RDWR);
    add_register("OUT2_COLOR", LP5009DevReg::OUT2_COLOR, I2CRegisters::RegMode::RDWR);

    add_register("RESET", LP5009DevReg::RESET, I2CRegisters::RegMode::RDWR);

    reset_registers();

    // create objects for each of the channels to handle the dynamics
    // of updating each.  The channel gets references to the relevant
    // configuration bytes.  Note that this assumes LED_CONFIG0 stays
    // at its reset value (independent, non-bank, control).
    r = NEW_NOTHROW LEDChannel(
        byte[(uint8_t)LP5009DevReg::OUT0_COLOR]
        );
    g = NEW_NOTHROW LEDChannel(
        byte[(uint8_t)LP5009DevReg::OUT1_COLOR]
        );
    b = NEW_NOTHROW LEDChannel(
        byte[(uint8_t)LP5009DevReg::OUT2_COLOR]
        );

    rgbled.init();
}

void LP5009::reset_registers()
{
    set_register(LP5009DevReg::DEVICE_CONFIG0, (uint8_t)0x00);
    set_register(LP5009DevReg::DEVICE_CONFIG1, (uint8_t)0x3C);  // datasheet reset value
    set_register(LP5009DevReg::LED_CONFIG0, (uint8_t)0x00);
    set_register(LP5009DevReg::LED0_BRIGHTNESS, (uint8_t)0xFF);
    set_register(LP5009DevReg::LED1_BRIGHTNESS, (uint8_t)0xFF);
    set_register(LP5009DevReg::LED2_BRIGHTNESS, (uint8_t)0xFF);
    set_register(LP5009DevReg::OUT0_COLOR, (uint8_t)0x00);
    set_register(LP5009DevReg::OUT1_COLOR, (uint8_t)0x00);
    set_register(LP5009DevReg::OUT2_COLOR, (uint8_t)0x00);
    set_register(LP5009DevReg::RESET, (uint8_t)0);
}

void LP5009::update(const class Aircraft &aircraft)
{
    {    // check the state of the reset register
        // 0xff in this register resets the device:
        if (get_register(LP5009DevReg::RESET) == 0xFF) {
            reset_registers();
        }
    }

    {    // check the state of the DEVICE_CONFIG0 register
        union {
            struct {
                uint8_t reserved_low : 6;
                uint8_t chip_en : 1;
                uint8_t reserved_high : 1;
            } bits;
            uint8_t byte;
        } config0_reg;
        config0_reg.byte = get_register(LP5009DevReg::DEVICE_CONFIG0);

        if (!config0_reg.bits.chip_en) {
            // do nothing, maintain outputs; not sure what actual behaviour is
            return;
        }
    }

    {    // check the state of the LED_CONFIG0 register
        union {
            struct {
                uint8_t led0_bank_en : 1;
                uint8_t led1_bank_en : 1;
                uint8_t led2_bank_en : 1;
            } bits;
            uint8_t byte;
        } led_config0_reg;
        led_config0_reg.byte = get_register(LP5009DevReg::LED_CONFIG0);
        // we don't get fancy with the device.  We expect this to be
        // at its reset value (independent control of LED0):
        if (led_config0_reg.bits.led0_bank_en) {
            fprintf(stderr, "Waiting for LED0 to be set to independent (non-bank) control\n");
            return;
        }
    }

    // update each channel.
    r->update();
    g->update();
    b->update();

    rgbled.set_colours(
        r->current_value(),
        g->current_value(),
        b->current_value()
        );
}

void LP5009::LEDChannel::update()
{
    // nothing to update here ATM; the output value is just the input value
}

#endif

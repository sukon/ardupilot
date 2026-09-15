#pragma once

/*

  ./Tools/autotest/sim_vehicle.py -v ArduCopter --gdb --debug --rgbled

  param set NTF_LED_TYPES 1048576  # enable LP5009 (External)
  reboot

  param set NTF_LED_OVERRIDE 1
  led 255 0 0   # red
  led 0 255 0   # green
  led 0 0 255   # blue

 */


#include "SIM_config.h"

#if AP_SIM_LP5009_ENABLED

#include "SIM_I2CDevice.h"
#include "SIM_RGBLED.h"

namespace SITL {

class LP5009DevReg : public I2CRegEnum {
public:
    static constexpr uint8_t DEVICE_CONFIG0   = 0x00;
    static constexpr uint8_t DEVICE_CONFIG1   = 0x01;
    static constexpr uint8_t LED_CONFIG0      = 0x02;

    static constexpr uint8_t BANK_BRIGHTNESS  = 0x03;
    static constexpr uint8_t BANK_A_COLOR     = 0x04;
    static constexpr uint8_t BANK_B_COLOR     = 0x05;
    static constexpr uint8_t BANK_C_COLOR     = 0x06;

    static constexpr uint8_t LED0_BRIGHTNESS  = 0x07;
    static constexpr uint8_t LED1_BRIGHTNESS  = 0x08;
    static constexpr uint8_t LED2_BRIGHTNESS  = 0x09;

    static constexpr uint8_t OUT0_COLOR       = 0x0B;
    static constexpr uint8_t OUT1_COLOR       = 0x0C;
    static constexpr uint8_t OUT2_COLOR       = 0x0D;
    static constexpr uint8_t OUT3_COLOR       = 0x0E;
    static constexpr uint8_t OUT4_COLOR       = 0x0F;
    static constexpr uint8_t OUT5_COLOR       = 0x10;
    static constexpr uint8_t OUT6_COLOR       = 0x11;
    static constexpr uint8_t OUT7_COLOR       = 0x12;
    static constexpr uint8_t OUT8_COLOR       = 0x13;

    static constexpr uint8_t RESET            = 0x17;
};

class LP5009 : public I2CDevice, protected I2CRegisters_8Bit
{
public:

    void init() override;

    void update(const class Aircraft &aircraft) override;

    int rdwr(I2C::i2c_rdwr_ioctl_data *&data) override {
        return I2CRegisters_8Bit::rdwr(data);
    }

private:

    // nested class to hold calculations for a single channel:
    class LEDChannel {
    public:
        LEDChannel(uint8_t &_direct_pwm_value) :
            direct_pwm_value{_direct_pwm_value}
            { }

        void update();

        // returns a value 0-255 for LED brightness:
        uint8_t current_value() const { return direct_pwm_value; }

    private:
        uint8_t &direct_pwm_value;
    };

    // OUT0/1/2 = LED0 group = red/green/blue for v1
    LEDChannel *r;
    LEDChannel *g;
    LEDChannel *b;

    void reset_registers();

    SIM_RGBLED rgbled{"LP5009"};
};

} // namespace SITL

#endif  // AP_SIM_LP5009_ENABLED

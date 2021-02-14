#include <PlatformDetermination.h>
#include "ESP32TouchKeysAbstraction.h"

void esp32TouchKeyInterruptHandler(void* touchAbsAsVoid) {
    TaskManager::markInterrupted(0);
}

bool ESP32TouchKeysAbstraction::runLoop() {
    return allOk;
}

void ESP32TouchKeysAbstraction::attachInterrupt(pinid_t pin, RawIntHandler interruptHandler, uint8_t mode) {
    if(interruptEnabled || !allOk) return; // on this abstraction interrupts are on or off
    serdebugF2("Enabling interrupt for ", pin);
    touch_pad_isr_register(esp32TouchKeyInterruptHandler, this);
    touch_pad_intr_enable();
    interruptEnabled = true;
}

uint8_t ESP32TouchKeysAbstraction::readValue(pinid_t pin) {
    if(!allOk) return 0;
    if(!startedUp) {
        startedUp = true;
        touch_pad_filter_start(DEFAULT_TOUCHKEY_FILTER_FREQ);
    }

    uint16_t val;
    touch_pad_read_filtered((touch_pad_t)pin, &val);
    return val <= pinThreshold;
}

void ESP32TouchKeysAbstraction::pinDirection(pinid_t pin, uint8_t mode) {
    if(!allOk) return;
    serdebugF2("Pin Direction ", pin);
    touch_pad_config((touch_pad_t) pin, pinThreshold);
    touch_pad_set_trigger_mode(TOUCH_TRIGGER_BELOW);
}

ESP32TouchKeysAbstraction::ESP32TouchKeysAbstraction(int defThreshold, touch_high_volt_t highVoltage,
                                                     touch_low_volt_t lowVoltage, touch_volt_atten_t attenuation) {
    allOk = touch_pad_init() == ESP_OK;
    serdebugF2("touch_pad_init ", allOk);
    touch_pad_set_voltage(highVoltage, lowVoltage, attenuation);
    serdebugF2("touch_pad set voltage ", allOk);
    interruptEnabled = false;
    startedUp = false;
    pinThreshold = defThreshold;
    enabledPinMask = 0;
}

#include "peripheral_power.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"


void peripheral_power_create_config(peripheral_power_config_t *config, uint8_t gate_pin, bool start_enabled) {
    if (!config) return;

    config->gate_pin = gate_pin;
    config->start_enabled = start_enabled;
}

void peripheral_power_init(peripheral_power_t *power, const peripheral_power_config_t *config) {
    if (!power || !config) return;
    if (power->initialized) return;

    power->config = *config;
    gpio_init(config->gate_pin);
    gpio_set_dir(config->gate_pin, GPIO_OUT);
    power->initialized = true;

    if (config->start_enabled) {
        peripheral_power_enable(power);
    } else {
        peripheral_power_disable(power);
    }
}

bool peripheral_power_enable(peripheral_power_t *power) {
    if (!power->initialized) return false;
    if (power->power_enabled) return true; 

    gpio_put(power->config.gate_pin, 0); // P-channel MOSFET ON (gate low)
    power->power_enabled = true;
    return true;
}

bool peripheral_power_disable(peripheral_power_t *power) {
    if (!power->initialized) return false;
    if (!power->power_enabled) return true; // Already disabled

    gpio_put(power->config.gate_pin, 1); // P-channel MOSFET OFF (gate high)
    power->power_enabled = false;
    return true;
}
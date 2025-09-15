#include "peripheral_power.h"
#include "hardware/gpio.h"

static bool power_enabled = false;
static bool initialized = false;

void peripheral_power_init(bool start_enabled) {
    if (initialized) return;

    gpio_init(MOSFET_GATE_PIN);
    gpio_set_dir(MOSFET_GATE_PIN, GPIO_OUT);
    initialized = true;

    if (start_enabled) {
        peripheral_power_enable();
    } else {
        peripheral_power_disable();
    }
}

int peripheral_power_enable(void) {
    if (!initialized) return -1;
    if (power_enabled) return 0; // Already enabled

    gpio_put(MOSFET_GATE_PIN, 0); // P-channel MOSFET ON (gate low)
    power_enabled = true;
    return 1;
}

int peripheral_power_disable(void) {
    if (!initialized) return -1;
    if (!power_enabled) return 0; // Already disabled

    gpio_put(MOSFET_GATE_PIN, 1); // P-channel MOSFET OFF (gate high)
    power_enabled = false;
    return 1;
}

bool peripheral_power_is_enabled(void) {
    return power_enabled;
}
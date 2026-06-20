#include "driver/gpio.h"

void hal_gpio_write(int pin, int val) { gpio_set_level(pin, val); }

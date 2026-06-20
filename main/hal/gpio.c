#include "driver/gpio.h"
#include "hal/gpio_types.h"

void hal_gpio_write(int pin, int value) {
  gpio_set_direction(pin, GPIO_MODE_OUTPUT); // enable by default
  gpio_set_level(pin, value);
}

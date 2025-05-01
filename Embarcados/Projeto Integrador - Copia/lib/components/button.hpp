#pragma once

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"

struct Button_cfg {
    void (*rise_callback)(void);
    void (*fall_callback)(void);
    gpio_num_t pin;
    uint32_t debounce_time;
    int32_t last_bt_state;
};

void add_button(Button_cfg cfg);

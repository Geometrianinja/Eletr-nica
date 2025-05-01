#include "button.hpp"
#include <Arduino.h>

static void bt_dbounce(void* arg) {
    Button_cfg* cfg = static_cast<Button_cfg*>(arg); // Cast the argument to Button_cfg type
     
    if(cfg->last_bt_state == 0) 
    {
        if(cfg->rise_callback != NULL) {
            cfg->rise_callback(); // Call the rise callback function if it is not null
        }
    } else {
        if(cfg->fall_callback != NULL) {
            cfg->fall_callback(); // Call the fall callback function if it is not null
        }
    }

    vTaskDelay(cfg->debounce_time / portTICK_PERIOD_MS); // Debounce delay
    cfg->last_bt_state = gpio_get_level(cfg->pin); // Read the button state

    gpio_intr_enable(cfg->pin); // Enable the interrupt for the button
    vTaskDelete(NULL);
}

static void bt_isr_handler(void*arg)
{
    Button_cfg* cfg = static_cast<Button_cfg*>(arg); // Cast the argument to Button_cfg type
    gpio_intr_disable(cfg->pin); // Disable the interrupt for the button
    xTaskCreate(bt_dbounce, "bt_dbounce", 2048, cfg, 0, NULL); // Create a new task to handle the button press
}


void add_button(Button_cfg cfg) {
    Button_cfg* cfg_ptr = new Button_cfg(cfg); // Store the button configuration in a global pointer
    gpio_config_t conf{
        1UL << cfg.pin, // Set the GPIO pin number
        GPIO_MODE_INPUT, // Set the GPIO pin as input
        GPIO_PULLUP_DISABLE, // Enable pull-up resistor
        GPIO_PULLDOWN_ENABLE, // Disable pull-down resistor
        GPIO_INTR_ANYEDGE
    };
    gpio_config(&conf); // Configure the GPIO pin for the button
    gpio_install_isr_service(0);
    gpio_isr_handler_add(cfg.pin, bt_isr_handler, cfg_ptr); // Install ISR handler for the button
}
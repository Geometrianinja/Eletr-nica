#include <Arduino.h>
#include "mpu.hpp"
#include "button.hpp"
#include "ble.hpp"
#include "BluetoothSerial.h"

SemaphoreHandle_t txMutex;
BluetoothSerial SerialBT;
bool isConnected = false;

void mpu_callback(float yaw, float pitch, float roll) {
    if (xSemaphoreTake(txMutex, portMAX_DELAY) == pdTRUE && isConnected) {
        char identifier = 'G'; // Identifier for MPU data
        float data[3] = {yaw, pitch, roll};
        SerialBT.write(identifier);
        SerialBT.write((uint8_t*)data, sizeof(data));
        xSemaphoreGive(txMutex);
    }
}

static void bluetooth_send_byte(char byte) {
    if (xSemaphoreTake(txMutex, portMAX_DELAY) == pdTRUE && isConnected) {
        SerialBT.write(byte);
        xSemaphoreGive(txMutex);
    }
}

static void wait_for_question_mark() {
    while (true) {
        if (SerialBT.available()) {
            char c = SerialBT.read();
            if (c == '?') {
                SerialBT.print("ESP32");
                isConnected = true;
                break;
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

static const gpio_num_t motor_pin = GPIO_NUM_5;
static void bt_listener(void *arg) {
    static unsigned long last_ping_time = millis();
    while (true) {
        unsigned long current_time = millis();
        if (current_time - last_ping_time >= 2300) {
            if (xSemaphoreTake(txMutex, portMAX_DELAY) == pdTRUE) {
                isConnected = false;
                wait_for_question_mark();
                xSemaphoreGive(txMutex);
                last_ping_time = current_time; // Reset ping time
            }
        }

        if (SerialBT.available()) {
            char c = SerialBT.read();
            if (c == 0) {
                digitalWrite(motor_pin, HIGH); // Turn off motor
            } else if (c == 1) {
                digitalWrite(motor_pin, LOW); // Turn on motor
            } else if (c == 2) {
                last_ping_time = current_time; // Reset ping time
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}


void setup() {
    Serial.begin(115200);
    pinMode(motor_pin, OUTPUT);
    pinMode(GPIO_NUM_2, OUTPUT); // Button pin
    digitalWrite(motor_pin, HIGH); // Turn off motor

    txMutex = xSemaphoreCreateMutex();
    SerialBT.begin("ESP_CONTROLLER");

    wait_for_question_mark();
    xTaskCreate(bt_listener, "Bluetooth Listener", 2048, NULL, 1, NULL);
    
    Wire.begin();
    MPU_cfg mpu_cfg;
    mpu_cfg.data_ready_callback = mpu_callback;
    if (add_mpu(mpu_cfg) == 0) {
        Serial.println("MPU6050 initialized successfully.");
    } else {
        Serial.println("Failed to initialize MPU6050.");
    }

    Button_cfg button_back_cfg;
    button_back_cfg.rise_callback = []() { bluetooth_send_byte('B'); };
    button_back_cfg.fall_callback = []() { bluetooth_send_byte('b'); };
    button_back_cfg.pin = GPIO_NUM_18;
    button_back_cfg.debounce_time = 20; 
    add_button(button_back_cfg);

    Button_cfg button_select_cfg;
    button_select_cfg.rise_callback = []() { bluetooth_send_byte('S');  };
    button_select_cfg.fall_callback = []() { bluetooth_send_byte('s'); };
    button_select_cfg.pin = GPIO_NUM_4; 
    button_select_cfg.debounce_time = 20; 
    add_button(button_select_cfg);

}

void loop() {
    //digitalWrite(GPIO_NUM_2, LOW); // Turn off the LED
    //vTaskDelay(1000 / portTICK_PERIOD_MS); // Main loop delay
    //digitalWrite(GPIO_NUM_2, HIGH); // Turn on the LED
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Main loop delay
}
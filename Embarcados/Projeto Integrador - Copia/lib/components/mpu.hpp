#pragma once
#include <Wire.h>
#include <I2Cdev.h>
#include <MPU6050_6Axis_MotionApps612.h>

struct MPU_cfg {
    void (*data_ready_callback)(float yaw, float pitch, float roll);
    //gpio_num_t interrupt_pin;
    MPU6050 mpu;
};

uint8_t add_mpu(MPU_cfg cfg);



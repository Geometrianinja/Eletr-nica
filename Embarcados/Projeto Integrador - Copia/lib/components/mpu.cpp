#include <Arduino.h>
#include <Wire.h>
#include <I2Cdev.h>
#include <MPU6050_6Axis_MotionApps612.h>
#include "mpu.hpp"



static void mpuTask(void *pvParameters) {
    MPU_cfg* cfg = static_cast<MPU_cfg*>(pvParameters);
    MPU6050& mpu = cfg->mpu;
    uint8_t fifoBuffer[64]; // FIFO storage buffer
    Quaternion q; // [w, x, y, z] quaternion container
    VectorFloat gravity; // [x, y, z] gravity vector
    float ypr[3]; // [yaw, pitch, roll] yaw/pitch/roll container and gravity vector

    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t interval = pdMS_TO_TICKS(6); // Convert 6ms to ticks

    while(1) {
        vTaskDelayUntil(&lastWakeTime, interval);

        static unsigned long lastPrintTime = millis();
        static int callCount = 0;
        callCount++;
        unsigned long currentTime = millis();
        if (currentTime - lastPrintTime >= 1000) {
            Serial.print("Task frequency: ");
            Serial.print(callCount);
            Serial.println(" Hz");
            callCount = 0;
            lastPrintTime = currentTime;
        }

        if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
            mpu.dmpGetQuaternion(&q, fifoBuffer);

            mpu.dmpGetGravity(&gravity, &q);
            mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
        

            float yaw   = ypr[0] * 180 / M_PI;
            float pitch = ypr[1] * 180 / M_PI;
            float roll  = ypr[2] * 180 / M_PI;

            if (cfg->data_ready_callback != NULL) {
                cfg->data_ready_callback(yaw, pitch, roll); // Call the data ready callback function
            }
        }
        
    }
}


uint8_t add_mpu(MPU_cfg cfg) {
    MPU_cfg* cfg_ptr = new MPU_cfg(cfg);
    MPU6050& mpu = cfg_ptr->mpu;
    uint8_t devStatus;
    Wire.setClock(400000);
    mpu.initialize();

    if (!mpu.testConnection()) {
        Serial.println("MPU6050 connection failed");
        return 10;
    }

    devStatus = mpu.dmpInitialize();

    if (devStatus == 0) {
        //mpu.CalibrateAccel(6);
        //mpu.CalibrateGyro(6);
        //1006.00000,       1433.00000,     1266.00000,      92.00000,       39.00000,       0.00000
        mpu.setXAccelOffset(1006);
        mpu.setYAccelOffset(1433);
        mpu.setZAccelOffset(1266);
        mpu.setXGyroOffset(92);
        mpu.setYGyroOffset(39);
        mpu.setZGyroOffset(0);

        mpu.PrintActiveOffsets();
        mpu.setDMPEnabled(true);

        xTaskCreate(mpuTask, "MPU6050_task", 30000, cfg_ptr, 5, NULL);
    } 
    return devStatus;
}

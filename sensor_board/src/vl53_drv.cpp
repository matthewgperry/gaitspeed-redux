// sensor_board/src/vl53_drv.cpp
//
// VL53L4CX time-of-flight sensor driver wrapper.
// Initialises the sensor over I2C (XSHUT + I2C address),
// attaches the GPIO1 falling-edge interrupt for measurement-ready
// events, and exposes start/stop/get-data/clear-interrupt
// operations consumed by vVL53_ISR_Task and vVL53_Poll_Task.

#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <task.h>
#include <Wire.h>
#include <vl53l4cx_class.h>
#include "vl53_drv.h"
#include "sensor_board_config.h"

static VL53L4CX sensor(&Wire, VL53_XSHUT_PIN);

void vl53_init() {
    Wire.begin();
    sensor.begin();
    sensor.VL53L4CX_Off();
    sensor.InitSensor(VL53_I2C_ADDR);
    attachInterrupt(digitalPinToInterrupt(VL53_INT_PIN), vl53_gpio_isr, FALLING);
}

void vl53_reset() {
    sensor.VL53L4CX_Off();
    vTaskDelay(pdMS_TO_TICKS(10));   // must be called outside xVL53Mutex
    sensor.InitSensor(VL53_I2C_ADDR);
    sensor.VL53L4CX_StartMeasurement();
}

void vl53_start_ranging() {
    sensor.VL53L4CX_StartMeasurement();
}

void vl53_stop_ranging() {
    sensor.VL53L4CX_StopMeasurement();
}

void vl53_get_data(VL53L4CX_MultiRangingData_t *data) {
    sensor.VL53L4CX_GetMultiRangingData(data);
}

void vl53_clear_interrupt() {
    sensor.VL53L4CX_ClearInterruptAndStartMeasurement();
}

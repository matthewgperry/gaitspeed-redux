#pragma once
// sensor_board/include/vl53_drv.h
#include <vl53l4cx_class.h>

void vl53_init();
void vl53_reset();
void vl53_start_ranging();
void vl53_stop_ranging();
void vl53_get_data(VL53L4CX_MultiRangingData_t *data);
void vl53_clear_interrupt();

// ISR callback — attached to VL53_INT_PIN via attachInterrupt()
extern "C" void vl53_gpio_isr();


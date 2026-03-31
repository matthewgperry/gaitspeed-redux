#pragma once
// can_drv.h

#include "can_protocol.h"

void can_init(uint8_t node_id);
void can_transmit(const CanFrame_t *frame);
// can_rx_callback() is defined in main.cpp; FDCAN ISR calls it.

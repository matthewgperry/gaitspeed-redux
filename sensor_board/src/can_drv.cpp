// sensor_board/src/can_drv.cpp
//
// STM32C092 FDCAN driver (classic frame mode, max 8-byte payload).
// Configures FDCAN1 at 500 kbps (48 MHz APB, prescaler=6,
// BS1=12 TQ, BS2=3 TQ), sets up a pass-all extended-ID filter
// into RX FIFO 0, and routes the interrupt to can_rx_callback()
// which enqueues frames for vCAN_RX_Task.

#include <Arduino.h>
#include <stm32c0xx_hal.h>
#include "can_drv.h"
#include "sensor_board_config.h"

// STM32C092 uses FDCAN (not classic bxCAN).  Classic frame format selected
// so maximum payload stays 8 bytes, matching the shared CAN protocol.
//
// Nominal bit timing for 500 kbps @ 48 MHz APB (FDCAN kernel clock):
//   48 MHz / (6 prescaler × (1 + 12 + 3) TQ) = 500 kbps
static FDCAN_HandleTypeDef hfdcan;

extern "C" void can_rx_callback(CanFrame_t *frame);   // defined in main.cpp

// HAL MSP init — configure PB8 (RX, AF8) and PB1 (TX, AF3), enable clock
extern "C" void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef *hfdcan_arg) {
    (void)hfdcan_arg;
    __HAL_RCC_FDCAN1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {};
    gpio.Mode  = GPIO_MODE_AF_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;

    // PB8 = FDCAN1_RX (AF8)
    gpio.Pin       = GPIO_PIN_8;
    gpio.Alternate = GPIO_AF8_FDCAN1;
    HAL_GPIO_Init(GPIOB, &gpio);

    // PB1 = FDCAN1_TX (AF3)
    gpio.Pin       = GPIO_PIN_1;
    gpio.Alternate = GPIO_AF3_FDCAN1;
    HAL_GPIO_Init(GPIOB, &gpio);

    HAL_NVIC_SetPriority(FDCAN1_IT0_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);
}

void can_init(uint8_t node_id) {
    (void)node_id;

    hfdcan.Instance                  = FDCAN1;
    hfdcan.Init.ClockDivider         = FDCAN_CLOCK_DIV1;
    hfdcan.Init.FrameFormat          = FDCAN_FRAME_CLASSIC;
    hfdcan.Init.Mode                 = FDCAN_MODE_NORMAL;
    hfdcan.Init.AutoRetransmission   = ENABLE;
    hfdcan.Init.TransmitPause        = DISABLE;
    hfdcan.Init.ProtocolException    = DISABLE;
    hfdcan.Init.NominalPrescaler     = 6;
    hfdcan.Init.NominalSyncJumpWidth = 3;
    hfdcan.Init.NominalTimeSeg1      = 12;
    hfdcan.Init.NominalTimeSeg2      = 3;
    // Data-phase fields unused in classic mode; set to match nominal timing.
    hfdcan.Init.DataPrescaler        = 6;
    hfdcan.Init.DataSyncJumpWidth    = 3;
    hfdcan.Init.DataTimeSeg1         = 12;
    hfdcan.Init.DataTimeSeg2         = 3;
    hfdcan.Init.StdFiltersNbr        = 0;
    hfdcan.Init.ExtFiltersNbr        = 1;
    hfdcan.Init.TxFifoQueueMode      = FDCAN_TX_FIFO_OPERATION;

    HAL_FDCAN_Init(&hfdcan);

    // Accept all extended frames into RX FIFO 0 (mask = 0 → don't-care all bits)
    FDCAN_FilterTypeDef f = {};
    f.IdType       = FDCAN_EXTENDED_ID;
    f.FilterIndex  = 0;
    f.FilterType   = FDCAN_FILTER_MASK;
    f.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    f.FilterID1    = 0x00000000;
    f.FilterID2    = 0x00000000;
    HAL_FDCAN_ConfigFilter(&hfdcan, &f);

    HAL_FDCAN_Start(&hfdcan);
    HAL_FDCAN_ActivateNotification(&hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
}

void can_transmit(const CanFrame_t *frame) {
    FDCAN_TxHeaderTypeDef hdr = {};
    hdr.Identifier          = frame->id;
    hdr.IdType              = FDCAN_EXTENDED_ID;
    hdr.TxFrameType         = FDCAN_DATA_FRAME;
    hdr.DataLength          = (uint32_t)(frame->dlc > 8 ? 8 : frame->dlc) << 16;
    hdr.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    hdr.BitRateSwitch       = FDCAN_BRS_OFF;
    hdr.FDFormat            = FDCAN_CLASSIC_CAN;
    hdr.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
    hdr.MessageMarker       = 0;

    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan, &hdr, const_cast<uint8_t *>(frame->data));
}

// Called by HAL when a frame arrives in RX FIFO 0
extern "C" void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan_arg, uint32_t RxFifo0ITs) {
    (void)RxFifo0ITs;
    FDCAN_RxHeaderTypeDef hdr;
    CanFrame_t frame = {};
    if (HAL_FDCAN_GetRxMessage(hfdcan_arg, FDCAN_RX_FIFO0, &hdr, frame.data) == HAL_OK) {
        frame.id  = hdr.Identifier;
        frame.dlc = static_cast<uint8_t>((hdr.DataLength >> 16) & 0x0F);
        can_rx_callback(&frame);
    }
}

// ISR vector
extern "C" void FDCAN1_IT0_IRQHandler(void) {
    HAL_FDCAN_IRQHandler(&hfdcan);
}

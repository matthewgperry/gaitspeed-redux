// display_board/src/can_drv.cpp
//
// STM32F103 bxCAN driver (CAN 2.0A/B, classic frames only).
// Configures CAN1 at 500 kbps (72 MHz APB1, prescaler=9,
// BS1=12 TQ, BS2=3 TQ), sets up a pass-all extended-ID filter
// into FIFO0, and routes the RX interrupt to can_rx_callback()
// which enqueues frames for vCAN_RX_Task.

#include <Arduino.h>
#include <stm32f1xx_hal.h>
#include "can_drv.h"
#include "display_board_config.h"

// STM32F103 bxCAN — classic CAN only, max 8 bytes per frame.
// Bit timing for 500 kbps @ 72 MHz APB1:
//   72 MHz / (9 prescaler * (1 + 12 + 3) TQ) = 500 kbps
static CAN_HandleTypeDef hcan;

extern "C" void can_rx_callback(CanFrame_t *frame);   // defined in main.cpp

// HAL MSP init — configure PA11/PA12 alternate function and clock
extern "C" void HAL_CAN_MspInit(CAN_HandleTypeDef *hcan_arg) {
    (void)hcan_arg;
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {};
    // PA12 = CAN_TX (alternate function push-pull)
    gpio.Pin   = GPIO_PIN_12;
    gpio.Mode  = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    // PA11 = CAN_RX (input floating)
    gpio.Pin  = GPIO_PIN_11;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
}

void can_init(uint8_t node_id) {
    (void)node_id;

    hcan.Instance                  = CAN1;
    hcan.Init.Prescaler            = 9;
    hcan.Init.Mode                 = CAN_MODE_NORMAL;
    hcan.Init.SyncJumpWidth        = CAN_SJW_1TQ;
    hcan.Init.TimeSeg1             = CAN_BS1_12TQ;
    hcan.Init.TimeSeg2             = CAN_BS2_3TQ;
    hcan.Init.TimeTriggeredMode    = DISABLE;
    hcan.Init.AutoBusOff           = ENABLE;
    hcan.Init.AutoWakeUp           = DISABLE;
    hcan.Init.AutoRetransmission   = ENABLE;
    hcan.Init.ReceiveFifoLocked    = DISABLE;
    hcan.Init.TransmitFifoPriority = DISABLE;

    HAL_CAN_Init(&hcan);

    // Accept all extended frames into FIFO0
    CAN_FilterTypeDef f = {};
    f.FilterBank           = 0;
    f.FilterMode           = CAN_FILTERMODE_IDMASK;
    f.FilterScale          = CAN_FILTERSCALE_32BIT;
    f.FilterIdHigh         = 0x0000;
    f.FilterIdLow          = 0x0000;
    f.FilterMaskIdHigh     = 0x0000;
    f.FilterMaskIdLow      = 0x0000;
    f.FilterFIFOAssignment = CAN_RX_FIFO0;
    f.FilterActivation     = ENABLE;
    HAL_CAN_ConfigFilter(&hcan, &f);

    HAL_CAN_Start(&hcan);
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
}

void can_transmit(const CanFrame_t *frame) {
    CAN_TxHeaderTypeDef hdr = {};
    hdr.ExtId = frame->id;
    hdr.IDE   = CAN_ID_EXT;
    hdr.RTR   = CAN_RTR_DATA;
    hdr.DLC   = frame->dlc > 8 ? 8 : frame->dlc;   // bxCAN max 8 bytes

    uint32_t mailbox;
    HAL_CAN_AddTxMessage(&hcan, &hdr, const_cast<uint8_t *>(frame->data), &mailbox);
}

// Called by HAL when a frame arrives in FIFO0
extern "C" void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan_arg) {
    CAN_RxHeaderTypeDef hdr;
    CanFrame_t frame = {};
    if (HAL_CAN_GetRxMessage(hcan_arg, CAN_RX_FIFO0, &hdr, frame.data) == HAL_OK) {
        frame.id  = hdr.ExtId;
        frame.dlc = static_cast<uint8_t>(hdr.DLC);
        can_rx_callback(&frame);
    }
}

// ISR vector
extern "C" void USB_LP_CAN1_RX0_IRQHandler(void) {
    HAL_CAN_IRQHandler(&hcan);
}

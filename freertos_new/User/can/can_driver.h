// can_driver.h
#ifndef __CAN_DRIVER_H
#define __CAN_DRIVER_H

#include "stm32f4xx.h"
#include "stm32f4xx_can.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "misc.h"

// 定义CAN设备和相关参数
#define CAN_DEVICE          CAN1
#define CAN_RCC_CLK         RCC_APB1Periph_CAN1
#define CAN_RX_GPIO_PORT    GPIOB
#define CAN_RX_GPIO_CLK     RCC_AHB1Periph_GPIOB
#define CAN_RX_PIN          GPIO_Pin_12
#define CAN_RX_SOURCE       GPIO_PinSource12
#define CAN_RX_AF           GPIO_AF_CAN1
#define CAN_TX_GPIO_PORT    GPIOB
#define CAN_TX_GPIO_CLK     RCC_AHB1Periph_GPIOB
#define CAN_TX_PIN          GPIO_Pin_13
#define CAN_TX_SOURCE       GPIO_PinSource13
#define CAN_TX_AF           GPIO_AF_CAN1
#define CAN_IRQn            CAN1_RX0_IRQn
#define CAN_IRQHandler      CAN1_RX0_IRQHandler

// 函数声明
void CANx_Init(void);
void CANx_SendStandardFrame(uint32_t id, uint8_t *data, uint8_t len);
void CANx_ReceiveStandardFrame(void);

#endif /* __CAN_DRIVER_H */


// can_driver.c
#include "can_driver.h"
#include "stdio.h" // 用于printf

__IO uint32_t g_usart_tx_count = 0; // 串口发送计数器

/**
 * @brief  初始化CAN外设
 * @note   配置GPIO, CAN时钟, 波特率, 回环模式, 中断等
 */
void CANx_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    CAN_InitTypeDef CAN_InitStructure;
    CAN_FilterInitTypeDef CAN_FilterInitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    // 1. 使能相关时钟
    RCC_AHB1PeriphClockCmd(CAN_TX_GPIO_CLK | CAN_RX_GPIO_CLK, ENABLE);
    RCC_APB1PeriphClockCmd(CAN_RCC_CLK, ENABLE);

    // 2. 配置TX和RX引脚 (PB12, PB13)
    GPIO_PinAFConfig(CAN_TX_GPIO_PORT, CAN_TX_SOURCE, CAN_TX_AF);
    GPIO_PinAFConfig(CAN_RX_GPIO_PORT, CAN_RX_SOURCE, CAN_RX_AF);

    GPIO_InitStructure.GPIO_Pin = CAN_TX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(CAN_TX_GPIO_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = CAN_RX_PIN;
    GPIO_Init(CAN_RX_GPIO_PORT, &GPIO_InitStructure);

    // 3. 配置CAN参数
    CAN_InitStructure.CAN_TTCM = DISABLE; // 时间触发通信模式
    CAN_InitStructure.CAN_ABOM = DISABLE; // 自动离线管理
    CAN_InitStructure.CAN_AWUM = DISABLE; // 自动唤醒模式
    CAN_InitStructure.CAN_NART = DISABLE; // 禁用报文自动传送
    CAN_InitStructure.CAN_RFLM = DISABLE; // 锁定模式
    CAN_InitStructure.CAN_TXFP = DISABLE; // 发送优先级由报文标识符决定
    CAN_InitStructure.CAN_Mode = CAN_Mode_LoopBack; // 关键：设置为回环模式

    // 波特率配置: 假设APB1 = 42MHz, 目标波特率为1Mbps
    // 波特率 = APB1 / (PRESCALER * (BS1 + BS2 + 1))
    // 42MHz / (4 * (5 + 4 + 1)) = 42MHz / (4 * 10) = 1Mbps
    CAN_InitStructure.CAN_SJW = CAN_SJW_1tq; // 重新同步跳跃宽度
    CAN_InitStructure.CAN_BS1 = CAN_BS1_5tq; // 时间段1
    CAN_InitStructure.CAN_BS2 = CAN_BS2_4tq; // 时间段2
    CAN_InitStructure.CAN_Prescaler = 4;     // 分频系数

    CAN_Init(CAN_DEVICE, &CAN_InitStructure);

    // 4. 配置过滤器，允许所有标准帧通过
    CAN_FilterInitStructure.CAN_FilterNumber = 0; // 过滤器编号
    CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask; // 屏蔽位模式
    CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit; // 32位宽
    CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000; // ID高16位
    CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;  // ID低16位
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000; // 掩码高16位
    CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;  // 掩码低16位
    CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_FIFO0; // 映射到FIFO0
    CAN_FilterInitStructure.CAN_FilterActivation = ENABLE; // 激活过滤器
    CAN_FilterInit(&CAN_FilterInitStructure);

    // 5. 配置NVIC中断
    NVIC_InitStructure.NVIC_IRQChannel = CAN_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01; // 中断优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // 6. 使能FIFO0接收中断
    // *** 修正：使用V1.8.0库的正确宏定义 ***
    CAN_ITConfig(CAN_DEVICE, CAN_IT_FMP0, ENABLE);
}

/**
 * @brief  发送标准帧
 * @param  id: 报文ID (0-0x7FF for standard frame)
 * @param  data: 要发送的数据指针
 * @param  len: 数据长度 (0-8)
 */
void CANx_SendStandardFrame(uint32_t id, uint8_t *data, uint8_t len) {
    CanTxMsg TxMessage;

    // 配置发送报文结构体
    TxMessage.StdId = id;                    // 设置标准ID
    TxMessage.ExtId = 0;                     // 扩展ID设为0 (因为使用标准帧)
    TxMessage.RTR = CAN_RTR_DATA;            // 数据帧
    TxMessage.IDE = CAN_ID_STD;              // 标准标识符
    TxMessage.DLC = (len > 8) ? 8 : len;     // 数据长度，最大8字节

    // 拷贝数据
    for(int i = 0; i < TxMessage.DLC; i++) {
        TxMessage.Data[i] = data[i];
    }

    // 请求发送
    CAN_Transmit(CAN_DEVICE, &TxMessage);
}

/**
 * @brief  CAN接收中断服务程序
 * @note   当FIFO0收到报文时，此函数会被调用
 */
void CAN_IRQHandler(void) {
    CanRxMsg RxMessage;

    // *** 修正：使用V1.8.0库的正确宏定义 ***
    if (CAN_GetITStatus(CAN_DEVICE, CAN_IT_FMP0) != RESET) {
        // *** 修正：使用V1.8.0库的正确宏定义 ***
        CAN_ClearITPendingBit(CAN_DEVICE, CAN_IT_FMP0);

        // 从FIFO0读取报文
        CAN_Receive(CAN_DEVICE, CAN_FIFO0, &RxMessage);

        // 简单处理接收到的数据：打印到串口
        printf("CAN Received - ID: 0x%03X, Data: ", RxMessage.StdId);
        for(int i = 0; i < RxMessage.DLC; i++) {
            printf("%02X ", RxMessage.Data[i]);
        }
        printf("\n");
    }
}

/**
 * @brief  一个简单的测试函数，用于调用发送函数
 * @note   可以放在主循环中定期调用
 */
void CANx_ReceiveStandardFrame(void) {
    // 准备要发送的数据
    uint8_t test_data[] = {0xDE, 0xAD, 0xBE, 0xEF, g_usart_tx_count >> 24, g_usart_tx_count >> 16, g_usart_tx_count >> 8, g_usart_tx_count};
    g_usart_tx_count++;

    // 发送标准帧，ID为0x123
    CANx_SendStandardFrame(0x123, test_data, sizeof(test_data));
}


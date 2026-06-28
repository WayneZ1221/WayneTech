#ifndef __I2C_EEPROM_H
#define	__I2C_EEPROM_H

#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "mpu6050.h"

// 定义 SCL 和 SDA 引脚
#define I2C_SCL_PIN       GPIO_Pin_8
#define I2C_SCL_GPIO_PORT GPIOB
#define I2C_SDA_PIN       GPIO_Pin_9
#define I2C_SDA_GPIO_PORT GPIOB

// I2C 地址
#define EEPROM_ADDR       0x68 << 1 // 左移一位，因为 FreeRTOS 通常用最低位区分读写
// AT24C02 设备地址
#define AT24C02_ADDR          0xD0 // 写地址 (0x68 << 1)
#define AT24C02_ADDR_W        0xD0
#define AT24C02_ADDR_R        0xD1

extern QueueHandle_t xQueueUartCommand; // 声明队列句柄，供中断函数使用

// 定义一个命令结构体，用于在任务间传递解析后的命令
typedef enum {
    CMD_TYPE_WRITE,
    CMD_TYPE_READ,
    CMD_TYPE_INVALID
} CommandType_t;

typedef struct {
    CommandType_t type;
    uint16_t addr;
    uint8_t data;
} Command_t;



// 函数声明
void I2C_Soft_Init(void);
uint8_t I2C_Start(void);
void I2C_Stop(void);
void I2C_Ack(void);
void I2C_NAck(void);
uint8_t I2C_WaitAck(void);
void I2C_SendByte(uint8_t txd);
uint8_t I2C_ReadByte(uint8_t ack);
uint8_t AT24C02_WriteOneByte(uint16_t WriteAddr, uint8_t DataToWrite);
uint8_t AT24C02_ReadOneByte(uint16_t ReadAddr);
uint8_t AT24C02_WriteLenByte(uint16_t WriteAddr, uint8_t *pBuffer, uint16_t NumToWrite);
uint8_t AT24C02_ReadLenByte(uint16_t ReadAddr, uint8_t *pBuffer, uint16_t NumToRead);
void Mpu6050_Initial(void);
void Mpu6050_ReadLenByte(uint16_t ReadAddr,uint8_t len,uint8_t* buffer);


#endif /* __I2C_EEPROM_H */




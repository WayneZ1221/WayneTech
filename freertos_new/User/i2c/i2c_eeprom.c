/**
  ******************************************************************************
  * @file    bsp_led.c
  * @author  fire
  * @version V1.0
  * @date    2015-xx-xx
  * @brief   led应用函数接口
  ******************************************************************************
  * @attention
  *
  * 实验平台:野火  STM32 F407 开发板  
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */


#include "i2c_eeprom.h"
#include "delay.h"

// 在 i2c_soft.c 文件顶部添加一个宏来控制调试打印
#define DEBUG_I2C 0 // 设置为 1 启用调试，设置为 0 禁用

#if DEBUG_I2C
    #include <stdio.h> // 临时引入，以便编译通过
    #define I2C_DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
    #define I2C_DEBUG_PRINT(...)
#endif


// SCL 和 SDA 引脚的宏定义，方便快速操作
// 使用标准库API替代直接寄存器操作
#define SCL_H         GPIO_SetBits(I2C_SCL_GPIO_PORT, I2C_SCL_PIN)
#define SCL_L         GPIO_ResetBits(I2C_SCL_GPIO_PORT, I2C_SCL_PIN)
#define SDA_H         GPIO_SetBits(I2C_SDA_GPIO_PORT, I2C_SDA_PIN)
#define SDA_L         GPIO_ResetBits(I2C_SDA_GPIO_PORT, I2C_SDA_PIN)
// 读取SDA状态依然可以直接读取寄存器
#define READ_SDA     (I2C_SDA_GPIO_PORT->IDR & I2C_SDA_PIN)

/**
  * @brief  初始化模拟 I2C 的 GPIO 引脚
  * @param  None
  * @retval None
  */
void I2C_Soft_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // 使能 GPIO 时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    // 配置 SCL 和 SDA 为开漏输出
    GPIO_InitStructure.GPIO_Pin = I2C_SCL_PIN | I2C_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;      // 输出模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;     // 开漏输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; // 高速
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;       // 上拉
    GPIO_Init(I2C_SCL_GPIO_PORT, &GPIO_InitStructure);

    // 空闲状态，SCL 和 SDA 都拉高
    SCL_H;
    SDA_H;
}
// I2C 延时函数，用于控制400K速率
#define I2C_DELAY_US() delay_us(1) 

/**
 * @brief  产生I2C起始信号
 */
uint8_t I2C_Start(void)
{
    SDA_H;
    SCL_H;
    I2C_DELAY_US();
    if (!READ_SDA) return 1; // SDA线为低电平则总线忙，退出
    SDA_L;
    I2C_DELAY_US();
    SCL_L;   // 钳住I2C总线，准备发送或接收数据
    return 0;
}

/**
 * @brief  产生I2C停止信号
 */
void I2C_Stop(void)
{
    SCL_L;
    SDA_L;
    I2C_DELAY_US();
    SCL_H;
    I2C_DELAY_US();
    SDA_H;
    I2C_DELAY_US();
}

/**
 * @brief  发送ACK信号
 */
void I2C_Ack(void)
{
    SCL_L;
    SDA_L; // 将SDA拉低，表示ACK
    I2C_DELAY_US();
    SCL_H;
    I2C_DELAY_US();
    SCL_L;
}

/**
 * @brief  发送NACK信号
 */
void I2C_NAck(void)
{
    SCL_L;
    SDA_H; // 释放SDA，使其为高，表示NACK
    I2C_DELAY_US();
    SCL_H;
    I2C_DELAY_US();
    SCL_L;
}

/**
 * @brief  等待应答信号到来
 * @retval 1，接收应答失败; 0，接收应答成功
 */
uint8_t I2C_WaitAck(void)
{
    uint8_t ucErrTime = 0;
    SCL_L; // 先拉低时钟，让SDA准备好
    SDA_H; // 释放SDA总线，让从机可以拉低它
    I2C_DELAY_US();
    SCL_H; // 拉高时钟，准备采样
    I2C_DELAY_US();
    while(READ_SDA) // 检查SDA是否被从机拉低 (ACK信号)
    {
        ucErrTime++;
        if(ucErrTime > 250) // 等待超时
        {
            I2C_Stop();
            return 1;
        }
    }
    SCL_L; // 时钟拉低，为下次传输做准备
    return 0;
}

/**
 * @brief  I2C发送一个字节
 * @param  txd：待发送数据
 */
void I2C_SendByte(uint8_t txd)
{
    uint8_t t;
    SCL_L; // 拉低时钟开始数据传输
    for(t = 0; t < 8; t++)
    {
        if((txd & 0x80) >> 7)  SDA_H; // 如果最高位是1，则拉高SDA
        else SDA_L;             // 如果最高位是0，则拉低SDA
        txd <<= 1; // 数据左移一位
        I2C_DELAY_US();
        SCL_H; // 拉高时钟，从机将在时钟上升沿采样SDA
        I2C_DELAY_US();
        SCL_L; // 拉低时钟，为下一个bit做准备
        I2C_DELAY_US();
    }
    // 发送完8位后，释放SDA总线，让从机可以拉低它发送ACK
    SDA_H;
}

/**
 * @brief  I2C读取一个字节
 * @param  ack：是否发送ACK信号 (1: ACK, 0: NACK)
 * @retval 读取到的字节
 */
uint8_t I2C_ReadByte(uint8_t ack)
{
    uint8_t i, receive = 0;
    SDA_H; // 释放SDA总线，让从机可以驱动它
    for(i = 0; i < 8; i++ )
    {
        SCL_L;
        I2C_DELAY_US();
        SCL_H; // 拉高时钟，从机将在时钟上升沿更新SDA上的数据
        receive <<= 1; // 先左移一位，为接收新bit腾位置
        if(READ_SDA) receive++; // 读取SDA上的电平状态
        I2C_DELAY_US();
    }
    if (!ack)
        I2C_NAck(); // 读完最后一个字节后，发送NACK
    else
        I2C_Ack();  // 读完非最后一个字节后，发送ACK继续读
    return receive;
}

/**
 * @brief  在AT24C02指定地址写入一个字节
 * @param  WriteAddr：写入数据的目的地址
 * @param  DataToWrite：要写入的数据
 * @retval 0: 成功；1: 失败
 */
uint8_t AT24C02_WriteOneByte(uint16_t WriteAddr, uint8_t DataToWrite)
{
    if(I2C_Start()) return 1;
    I2C_SendByte(AT24C02_ADDR_W);   // 发送写命令
    if(I2C_WaitAck()) {
        I2C_Stop();
        return 1;
    }
    if(WriteAddr > 255) // AT24C02是256字节，地址高字节只可能是0
    {
        I2C_Stop();
        return 1;
    }
    I2C_SendByte(WriteAddr % 256); // 发送低字节地址
    I2C_WaitAck();
    I2C_SendByte(DataToWrite); // 发送字节数据
    I2C_WaitAck();
    I2C_Stop();
    delay_ms(5); // 等待内部写周期完成
    return 0;
}

/**
 * @brief  在AT24C02指定地址读出一个字节
 * @param  ReadAddr：要读出数据的地址
 * @retval 读取到的数据
 */
uint8_t AT24C02_ReadOneByte(uint16_t ReadAddr)
{
    uint8_t temp = 0;
    I2C_Start();
    I2C_SendByte(AT24C02_ADDR_W); // 发送写命令
    I2C_WaitAck();
    if(ReadAddr > 255) // AT24C02是256字节
    {
        I2C_Stop();
        return 0xFF;
    }
    I2C_SendByte(ReadAddr % 256); // 发送低字节地址
    I2C_WaitAck();
    I2C_Start(); // 重启信号
    I2C_SendByte(AT24C02_ADDR_R); // 进入接收模式
    I2C_WaitAck();
    temp = I2C_ReadByte(0); // 读数据, 发送nACK (最后一个字节)
    I2C_Stop();
    return temp;
}

/**
 * @brief  在Mpu6050指定地址起读出多个字节
 * @param  ReadAddr：要读出数据的地址
 * @retval 读取到的数据
 */
void Mpu6050_ReadLenByte(uint16_t ReadAddr,uint8_t len,uint8_t* buffer)
{
    uint8_t temp = 0;
    I2C_Start();
    I2C_SendByte(AT24C02_ADDR_W); // 发送写命令
    I2C_WaitAck();
//    if(ReadAddr > 255) // AT24C02是256字节
//    {
//        I2C_Stop();
//        return 0xFF;
//    }
    I2C_SendByte(ReadAddr % 256); // 发送低字节地址
    I2C_WaitAck();
    I2C_Start(); // 重启信号
    I2C_SendByte(AT24C02_ADDR_R); // 进入接收模式
    for(temp=0;temp<len;temp++)
    {
		if((len-temp)>1)
		{
			I2C_WaitAck();
		    buffer[temp] = I2C_ReadByte(1); // 读数据, 发送ACK 
		}
		else if((len-temp)==1)
		{
			I2C_WaitAck();
		    buffer[temp] = I2C_ReadByte(0); // 读数据, 发送nACK (最后一个字节)
		}
    }
    I2C_Stop();
//    return temp;
}



/**
 * @brief  在AT24C02指定地址连续写入多个字节
 * @param  WriteAddr：写入数据的目的地址
 * @param  pBuffer：数据指针
 * @param  NumToWrite：要写的字节数
 * @retval 0: 成功；1: 失败
 */
uint8_t AT24C02_WriteLenByte(uint16_t WriteAddr, uint8_t *pBuffer, uint16_t NumToWrite)
{
    for(uint16_t i=0; i<NumToWrite; i++)
    {
        if(AT24C02_WriteOneByte(WriteAddr + i, *(pBuffer + i)))
            return 1; // 写入失败
    }
    return 0;
}

/**
 * @brief  在AT24C02指定地址连续读出多个字节
 * @param  ReadAddr：要读出数据的首地址
 * @param  pBuffer：数据指针
 * @param  NumToRead：要读的字节数
 * @retval 0: 成功；1: 失败
 */
uint8_t AT24C02_ReadLenByte(uint16_t ReadAddr, uint8_t *pBuffer, uint16_t NumToRead)
{
    for(uint16_t i=0; i<NumToRead; i++)
    {
        pBuffer[i] = AT24C02_ReadOneByte(ReadAddr + i);
    }
    return 0;
}


void Mpu6050_Initial(void)
{
	AT24C02_WriteOneByte(MPU6050_RA_PWR_MGMT_1, 0x00);

	AT24C02_WriteOneByte(MPU6050_RA_SMPLRT_DIV , 0x07);

	AT24C02_WriteOneByte(MPU6050_RA_CONFIG , 0x06);

	AT24C02_WriteOneByte(MPU6050_RA_ACCEL_CONFIG , 0x01);

	AT24C02_WriteOneByte(MPU6050_RA_GYRO_CONFIG, 0x18);

}



// delay.c
#include "delay.h"

static uint8_t  fac_us = 0;	// us延时倍乘数
static uint16_t fac_ms = 0;	// ms延时倍乘数

/**
  * @brief  初始化延迟函数
  * @note   此函数必须在 SysTick_Config() 之后调用，因为 fac_us 需要 SystemCoreClock 的值
  * @param  None
  * @retval None
  */
void delay_init(void)
{
    // SystemCoreClock / 1000000UL 代表1us需要的时钟周期数
    // 例如 SystemCoreClock = 168000000, fac_us = 168
    fac_us = SystemCoreClock / 1000000UL;
    // SystemCoreClock / 1000UL 代表1ms需要的时钟周期数
    // 例如 SystemCoreClock = 168000000, fac_ms = 168000
    fac_ms = (uint16_t)fac_us * 1000;
}

/**
  * @brief  微秒级延时
  * @param  nus: 要延时的微秒数
  * @retval None
  */
void delay_us(uint32_t nus)
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 0;
    uint32_t reload = SysTick->LOAD;	// LOAD的值
    ticks = nus * fac_us; 			// 需要的节拍数
    told = SysTick->VAL;        		// 刚进入时的计数器值
    while(1)
    {
        tnow = SysTick->VAL;
        if(tnow != told)
        {
            if(tnow < told) tcnt += told - tnow;	// 正常情况，减法即可
            else tcnt += reload - tnow + told;	// 发生溢出的情况
            told = tnow;
            if(tcnt >= ticks) break;	// 时间超过/等于要延迟的时间,则退出.
        }
    }
}

/**
  * @brief  毫秒级延时
  * @param  nms: 要延时的毫秒数
  * @retval None
  */
void delay_ms(uint32_t nms)
{
    uint32_t i;
    for(i = 0; i < nms; i++)
    {
        delay_us(1000); // 调用微秒延时函数
    }
}
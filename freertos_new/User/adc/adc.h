#ifndef _ADC_H
#define	_ADC_H

#include "stm32f4xx.h"

#define RHEOSTAT_NOFCHANEL      3

/*=====================¨ª¡§¦Ì¨¤1 IO======================*/
// PB0 ¨ª¡§1y¦Ì¡Â?¡À?¨®¦Ì????¡Â
// ADC IOo¨º?¡§¨°?
#define RHEOSTAT_ADC_GPIO_PORT1    GPIOB
#define RHEOSTAT_ADC_GPIO_PIN1     GPIO_Pin_0
#define RHEOSTAT_ADC_GPIO_CLK1     RCC_AHB1Periph_GPIOB
#define RHEOSTAT_ADC_CHANNEL1      ADC_Channel_8
/*=====================¨ª¡§¦Ì¨¤2 IO ======================*/
// PB1 ¨ª¡§1y¦Ì¡Â?¡À?¨®1a??¦Ì?¡Á¨¨
// ADC IOo¨º?¡§¨°?
#define RHEOSTAT_ADC_GPIO_PORT2    GPIOB
#define RHEOSTAT_ADC_GPIO_PIN2     GPIO_Pin_1
#define RHEOSTAT_ADC_GPIO_CLK2     RCC_AHB1Periph_GPIOB
#define RHEOSTAT_ADC_CHANNEL2      ADC_Channel_9
/*=====================¨ª¡§¦Ì¨¤3 IO ======================*/
// PA6 D¨¹??¡ê??¨¦¨®???¡ã????¨®3V3?¨°??GND¨¤¡ä¨º¦Ì?¨¦
// ADC IOo¨º?¡§¨°?
#define RHEOSTAT_ADC_GPIO_PORT3    GPIOA
#define RHEOSTAT_ADC_GPIO_PIN3     GPIO_Pin_6
#define RHEOSTAT_ADC_GPIO_CLK3     RCC_AHB1Periph_GPIOA
#define RHEOSTAT_ADC_CHANNEL3     ADC_Channel_6

// ADC D¨°o?o¨º?¡§¨°?
#define RHEOSTAT_ADC              ADC1
#define RHEOSTAT_ADC_CLK          RCC_APB2Periph_ADC1
// ADC DR??¡ä??¡Âo¨º?¡§¨°?¡ê?ADC¡Áa??o¨®¦Ì?¨ºy¡Á??¦Ì?¨°¡ä?¡¤??¨²?a¨¤?
#define RHEOSTAT_ADC_DR_ADDR    ((u32)ADC1+0x4c)


// ADC DMA ¨ª¡§¦Ì¨¤o¨º?¡§¨°?¡ê??a¨¤??¨°??¨º1¨®?DMA¡ä?¨º?
#define RHEOSTAT_ADC_DMA_CLK      RCC_AHB1Periph_DMA2
#define RHEOSTAT_ADC_DMA_CHANNEL  DMA_Channel_0
#define RHEOSTAT_ADC_DMA_STREAM   DMA2_Stream0



void Rheostat_Init(void);

#endif /* __BSP_ADC_H */




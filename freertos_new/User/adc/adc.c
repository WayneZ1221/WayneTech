#include "adc.h"

__IO uint16_t ADC_ConvertedValue[RHEOSTAT_NOFCHANEL]={0};

static void Rheostat_ADC_GPIO_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;	
	/*=====================¨ª¡§¦Ì¨¤1======================*/	
	// ¨º1?¨¹ GPIO ¨º¡À?¨®
	RCC_AHB1PeriphClockCmd(RHEOSTAT_ADC_GPIO_CLK1,ENABLE);		
	// ???? IO
  GPIO_InitStructure.GPIO_Pin = RHEOSTAT_ADC_GPIO_PIN1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  //2?¨¦?¨¤-2???¨¤-	
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
	GPIO_Init(RHEOSTAT_ADC_GPIO_PORT1, &GPIO_InitStructure);

	/*=====================¨ª¡§¦Ì¨¤2======================*/
	// ¨º1?¨¹ GPIO ¨º¡À?¨®
	RCC_AHB1PeriphClockCmd(RHEOSTAT_ADC_GPIO_CLK2,ENABLE);		
	// ???? IO
  GPIO_InitStructure.GPIO_Pin = RHEOSTAT_ADC_GPIO_PIN2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  //2?¨¦?¨¤-2???¨¤-	
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
	GPIO_Init(RHEOSTAT_ADC_GPIO_PORT2, &GPIO_InitStructure);	

	/*=====================¨ª¡§¦Ì¨¤3=======================*/
	// ¨º1?¨¹ GPIO ¨º¡À?¨®
	RCC_AHB1PeriphClockCmd(RHEOSTAT_ADC_GPIO_CLK3,ENABLE);		
	// ???? IO
  GPIO_InitStructure.GPIO_Pin = RHEOSTAT_ADC_GPIO_PIN3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  //2?¨¦?¨¤-2???¨¤-	
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
	GPIO_Init(RHEOSTAT_ADC_GPIO_PORT3, &GPIO_InitStructure);
}

static void Rheostat_ADC_Mode_Config(void)
{
	DMA_InitTypeDef DMA_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;
  ADC_CommonInitTypeDef ADC_CommonInitStructure;
	
  // ------------------DMA Init ?¨¢11¨¬?2?¨ºy 3?¨º??¡¥--------------------------
  // ADC1¨º1¨®?DMA2¡ê?¨ºy?Y¨¢¡Â0¡ê?¨ª¡§¦Ì¨¤0¡ê??a??¨º?¨º?2¨¢1¨¬?¡§?¨¤¦Ì?
  // ?a??DMA¨º¡À?¨®
  RCC_AHB1PeriphClockCmd(RHEOSTAT_ADC_DMA_CLK, ENABLE); 
	// ¨ªa¨¦¨¨?¨´?¡¤?a¡êoADC ¨ºy?Y??¡ä??¡Â¦Ì??¡¤
	DMA_InitStructure.DMA_PeripheralBaseAddr = RHEOSTAT_ADC_DR_ADDR;	
  // ¡ä?¡ä¡é?¡Â¦Ì??¡¤¡ê?¨º¦Ì?¨º¨¦??¨ª¨º?¨°????¨²2?SRAM¦Ì?¡À?¨¢?	
	DMA_InitStructure.DMA_Memory0BaseAddr = (u32)ADC_ConvertedValue;  
  // ¨ºy?Y¡ä?¨º?¡¤??¨°?a¨ªa¨¦¨¨¦Ì?¡ä?¡ä¡é?¡Â	
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;	
	// ?o3???¡ä¨®D??a¡ê???¨°?¡ä?¡ä?¨º?¦Ì?¨ºy?Y¨¢?
	DMA_InitStructure.DMA_BufferSize = RHEOSTAT_NOFCHANEL;	
	// ¨ªa¨¦¨¨??¡ä??¡Â??¨®D¨°???¡ê?¦Ì??¡¤2?¨®?¦ÌY??
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  // ¡ä?¡ä¡é?¡Â¦Ì??¡¤1¨¬?¡§
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; 
  // // ¨ªa¨¦¨¨¨ºy?Y¡ä¨®D??a¡ã?¡Á?¡ê??¡ä¨¢???¡Á??¨² 
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; 
  //	¡ä?¡ä¡é?¡Â¨ºy?Y¡ä¨®D?¨°2?a¡ã?¡Á?¡ê??¨²¨ªa¨¦¨¨¨ºy?Y¡ä¨®D??¨¤¨ª?
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;	
	// ?-?¡¤¡ä?¨º??¡ê¨º?
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  // DMA ¡ä?¨º?¨ª¡§¦Ì¨¤¨®??¨¨???a??¡ê?¦Ì¡À¨º1¨®?¨°???DMA¨ª¡§¦Ì¨¤¨º¡À¡ê?¨®??¨¨??¨¦¨¨??2?¨®¡ã?¨¬
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  // ???1DMA FIFO	¡ê?¨º1¨®??¡À¨¢??¡ê¨º?
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;  
  // FIFO ¡ä¨®D?¡ê?FIFO?¡ê¨º????1¨º¡À¡ê??a??2?¨®?????	
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;  
	// ???? DMA ¨ª¡§¦Ì¨¤¡ê?¨ª¡§¦Ì¨¤¡ä??¨²¨®¨²¨¢¡Â?D
  DMA_InitStructure.DMA_Channel = RHEOSTAT_ADC_DMA_CHANNEL; 
  //3?¨º??¡¥DMA¨¢¡Â¡ê?¨¢¡Â?¨¤¦Ì¡À¨®¨²¨°???¡ä¨®¦Ì?1¨¹¦Ì¨¤¡ê?1¨¹¦Ì¨¤¨¤???¨®Do¨¹?¨¤¨ª¡§¦Ì¨¤
	DMA_Init(RHEOSTAT_ADC_DMA_STREAM, &DMA_InitStructure);
	// ¨º1?¨¹DMA¨¢¡Â
  DMA_Cmd(RHEOSTAT_ADC_DMA_STREAM, ENABLE);
	
	// ?a??ADC¨º¡À?¨®
	RCC_APB2PeriphClockCmd(RHEOSTAT_ADC_CLK , ENABLE);
  // -------------------ADC Common ?¨¢11¨¬? 2?¨ºy 3?¨º??¡¥------------------------
	// ?¨¤¨¢¡éADC?¡ê¨º?
  ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
  // ¨º¡À?¨®?afpclk x¡¤??¦Ì	
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;
  // ???1DMA?¡À?¨®¡¤??¨º?¡ê¨º?	
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
  // 2¨¦?¨´¨º¡À??????	
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_20Cycles;  
  ADC_CommonInit(&ADC_CommonInitStructure);
	
  // -------------------ADC Init ?¨¢11¨¬? 2?¨ºy 3?¨º??¡¥--------------------------
	ADC_StructInit(&ADC_InitStructure);
  // ADC ¡¤?¡À??¨º
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
  // ¨¦¡§?¨¨?¡ê¨º?¡ê??¨¤¨ª¡§¦Ì¨¤2¨¦?¡¥D¨¨¨°a	
  ADC_InitStructure.ADC_ScanConvMode = ENABLE; 
  // ¨¢?D?¡Áa??	
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE; 
  //???1¨ªa2?¡À???¡ä£¤¡¤¡é
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
  //¨ªa2?¡ä£¤¡¤¡é¨ª¡§¦Ì¨¤¡ê?¡À?¨¤y¡Á¨®¨º1¨®?¨¨¨ª?t¡ä£¤¡¤¡é¡ê?¡ä??¦Ì??¡À??3?¦Ì?¡ä?¨¦
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
  //¨ºy?Y¨®¨°????	
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  //¡Áa??¨ª¡§¦Ì¨¤ 1??
  ADC_InitStructure.ADC_NbrOfConversion = RHEOSTAT_NOFCHANEL;                                    
  ADC_Init(RHEOSTAT_ADC, &ADC_InitStructure);
  //---------------------------------------------------------------------------
	
  // ???? ADC ¨ª¡§¦Ì¨¤¡Áa???3D¨°o¨ª2¨¦?¨´¨º¡À???¨¹?¨²
  ADC_RegularChannelConfig(RHEOSTAT_ADC, RHEOSTAT_ADC_CHANNEL1, 1, 
	                         ADC_SampleTime_3Cycles);
  ADC_RegularChannelConfig(RHEOSTAT_ADC, RHEOSTAT_ADC_CHANNEL2, 2, 
	                         ADC_SampleTime_3Cycles); 
  ADC_RegularChannelConfig(RHEOSTAT_ADC, RHEOSTAT_ADC_CHANNEL3, 3, 
	                         ADC_SampleTime_3Cycles); 

  // ¨º1?¨¹DMA???¨® after last transfer (Single-ADC mode)
  ADC_DMARequestAfterLastTransferCmd(RHEOSTAT_ADC, ENABLE);
  // ¨º1?¨¹ADC DMA
  ADC_DMACmd(RHEOSTAT_ADC, ENABLE);
	
	// ¨º1?¨¹ADC
  ADC_Cmd(RHEOSTAT_ADC, ENABLE);  
  //?a¨º?adc¡Áa??¡ê?¨¨¨ª?t¡ä£¤¡¤¡é
  ADC_SoftwareStartConv(RHEOSTAT_ADC);
}



void Rheostat_Init(void)
{
	Rheostat_ADC_GPIO_Config();
	Rheostat_ADC_Mode_Config();
}




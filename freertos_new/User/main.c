/**
  *********************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2018-xx-xx
  * @brief   FreeRTOS V9.0.0  + STM32 �жϹ���
  *********************************************************************
  * @attention
  *
  * ʵ��ƽ̨:Ұ��  STM32 ������ 
  * ��̳    :http://www.firebbs.cn
  * �Ա�    :https://fire-stm32.taobao.com
  *
  **********************************************************************
  */ 
 
/*
*************************************************************************
*                             ������ͷ�ļ�
*************************************************************************
*/ 
/* FreeRTOSͷ�ļ� */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"

/* ������Ӳ��bspͷ�ļ� */
#include "bsp_led.h"
#include "bsp_debug_usart.h"
#include "bsp_key.h"
#include "bsp_exti.h"
#include "i2c_eeprom.h"   
#include "delay.h"
#include "adc.h"
#include "can_driver.h"
#include "bsp_led_pwm.h"

/* ��׼��ͷ�ļ� */
#include <string.h>

/*�㷨ͷ�ļ�*/
#include "kalman_filter.h"

// �û��Զ�����������
uint8_t data_to_write[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0X99};

// Ҫ��ȡ�����ݳ���
#define DATA_TO_READ_LENGTH 16


/**************************** ������ ********************************/
/* 
 * ��������һ��ָ�룬����ָ��һ�����񣬵����񴴽���֮�����;�����һ��������
 * �Ժ�����Ҫ��������������Ҫͨ��������������������������������Լ�����ô
 * ����������ΪNULL��
 */
static TaskHandle_t LED_Task_Handle = NULL;/* LED������ */

static TaskHandle_t AppTaskCreate_Handle = NULL;/* ���������� */
static TaskHandle_t Key_Task_Handle = NULL;/* LED������ */
static TaskHandle_t Uart_Task_Handle = NULL;/* KEY������ */

static TaskHandle_t xTaskCommand_Handle = NULL;
static TaskHandle_t xTaskExecutor_Handle = NULL;
static TaskHandle_t xAdcTaskHandle = NULL;


/********************************** �ں˶����� *********************************/
/*
 * �ź�������Ϣ���У��¼���־�飬������ʱ����Щ�������ں˵Ķ���Ҫ��ʹ����Щ�ں�
 * ���󣬱����ȴ����������ɹ�֮��᷵��һ����Ӧ�ľ����ʵ���Ͼ���һ��ָ�룬������
 * �ǾͿ���ͨ��������������Щ�ں˶���
 *
 * �ں˶���˵���˾���һ��ȫ�ֵ����ݽṹ��ͨ����Щ���ݽṹ���ǿ���ʵ��������ͨ�ţ�
 * �������¼�ͬ���ȸ��ֹ��ܡ�������Щ���ܵ�ʵ��������ͨ��������Щ�ں˶���ĺ���
 * ����ɵ�
 * 
 */

static TimerHandle_t Adctimer_Handle =NULL;   /* adc������ʱ�����*/
static TimerHandle_t Swtmr2_Handle =NULL;   /* ������ʱ����� */
static TimerHandle_t Ledtimer_Handle=NULL; /*LED�仯��ʱ�����*/

static EventGroupHandle_t KEY_Event_Handle =NULL;/*�¼�����*/


QueueHandle_t Test_Queue =NULL;
SemaphoreHandle_t BinarySem_Handle =NULL;
QueueHandle_t xQueueUartCommand =NULL;
QueueHandle_t xAdcDataQueue =NULL;


/******************************* ȫ�ֱ������� ************************************/
/*
 * ��������дӦ�ó����ʱ�򣬿�����Ҫ�õ�һЩȫ�ֱ�����
 */
 
extern char Usart_Rx_Buf[USART_RBUFF_SIZE];
 
 // ADC��ԭʼֵ
extern __IO uint16_t ADC_ConvertedValue[RHEOSTAT_NOFCHANEL];

// ADC������ֵ	 
float ADC_ConvertedValueLocal[RHEOSTAT_NOFCHANEL]={0}; 
/******************************* �궨�� ************************************/
/*
 * ��������дӦ�ó����ʱ�򣬿�����Ҫ�õ�һЩ�궨�塣
 */
#define  QUEUE_LEN    4   /* ���еĳ��ȣ����ɰ������ٸ���Ϣ */
#define  QUEUE_SIZE   4   /* ������ÿ����Ϣ��С���ֽڣ� */
#define  KEY1_EVENT (0x01 << 0)//�����¼������λ 0 
#define  KEY2_EVENT (0x01 << 1)//�����¼������λ 1


/*
*************************************************************************
*                             ��������
*************************************************************************
*/
static void AppTaskCreate(void);/* ���ڴ������� */
static void LED_Task(void* pvParameters);/* KEY_Task����ʵ�� */

static void Key_Task(void* pvParameters);/* KEY_Task����ʵ�� */
static void Uart_Task(void* pvParameters);/* UART_Task����ʵ�� */

static void BSP_Init(void);/* ���ڳ�ʼ�����������Դ */

static void task_parse_and_execute_command(void* parameter);/*I2Cͨ������д��Ͷ�ȡeeprom����*/

static void ADC_Task(void* parameter);//ADCȡֵ����

/*****************************************************************
  * @brief  ������
  * @param  ��
  * @retval ��
  * @note   ��һ����������Ӳ����ʼ�� 
            �ڶ���������APPӦ������
            ������������FreeRTOS����ʼ���������
  ****************************************************************/
int main(void)
{	
  BaseType_t xReturn = pdPASS;/* ����һ��������Ϣ����ֵ��Ĭ��ΪpdPASS */
  
  /* ������Ӳ����ʼ�� */
  BSP_Init();
  
	printf("����һ��[Ұ��]-STM32ȫϵ�п�����-FreeRTOS�жϹ���ʵ�飡\n");
  printf("����KEY1 | KEY2�����жϣ�\n");
  printf("���ڷ������ݴ����ж�,����������!\n");
  
   /* ����AppTaskCreate���� */
  xReturn = xTaskCreate((TaskFunction_t )AppTaskCreate,  /* ������ں��� */
                        (const char*    )"AppTaskCreate",/* �������� */
                        (uint16_t       )512,  /* ����ջ��С */
                        (void*          )NULL,/* ������ں������� */
                        (UBaseType_t    )1, /* ��������ȼ� */
                        (TaskHandle_t*  )&AppTaskCreate_Handle);/* ������ƿ�ָ�� */ 
  /* ����������� */           
  if(pdPASS == xReturn)
    vTaskStartScheduler();   /* �������񣬿������� */
  else
    return -1;  

  while(1);/* ��������ִ�е����� */    
}


/***********************************************************************
  * @ ������  �� AppTaskCreate
  * @ ����˵���� Ϊ�˷�����������е����񴴽����������������������
  * @ ����    �� ��  
  * @ ����ֵ  �� ��
  **********************************************************************/
static void AppTaskCreate(void)
{
  BaseType_t xReturn = pdPASS;/* ����һ��������Ϣ����ֵ��Ĭ��ΪpdPASS */
  
  taskENTER_CRITICAL();           //�����ٽ���
  
		  /* ��������Queue */
		  Test_Queue = xQueueCreate((UBaseType_t ) QUEUE_LEN,/* ��Ϣ���еĳ��� */
	                            (UBaseType_t ) QUEUE_SIZE);/* ��Ϣ�Ĵ�С */

								
		xAdcDataQueue = xQueueCreate(4, 8);
		if(NULL != Test_Queue)
	    printf("Test_Queue��Ϣ���д����ɹ�!\n");

	  xQueueUartCommand = xQueueCreate(5, 256); // 5��Ԫ�أ�ÿ��Ԫ��256�ֽ�
		if(xQueueUartCommand == NULL)
			printf("FATAL ERROR: Queue creation failed! System halted.\r\n");

	
	  /* ���� �ź���Sem */
	  BinarySem_Handle = xSemaphoreCreateBinary();	 
	  
		if(NULL != BinarySem_Handle)
	    printf("BinarySem_Handle��ֵ�ź��������ɹ�!\n");

		/* ���� �¼��� */
		KEY_Event_Handle = xEventGroupCreate(); 
		 if (NULL != KEY_Event_Handle) 
		   printf("KEY_Event_Handle �¼������ɹ�!\r\n");
	
  /* ����LED_Task���� */
  xReturn = xTaskCreate((TaskFunction_t )LED_Task, /* ������ں��� */
                        (const char*    )"LED_Task",/* �������� */
                        (uint16_t       )512,   /* ����ջ��С */
                        (void*          )NULL,	/* ������ں������� */
                        (UBaseType_t    )2,	    /* ��������ȼ� */
                        (TaskHandle_t*  )&LED_Task_Handle);/* ������ƿ�ָ�� */
  if(pdPASS == xReturn)
    printf("����LED_Task����ɹ�!\n");

    /* ����KEY_Task���� */
  xReturn = xTaskCreate((TaskFunction_t )Key_Task, /* ������ں��� */
                        (const char*    )"Key_Task",/* �������� */
                        (uint16_t       )256,   /* ����ջ��С */
                        (void*          )NULL,	/* ������ں������� */
                        (UBaseType_t    )2,	    /* ��������ȼ� */
                        (TaskHandle_t*  )&Key_Task_Handle);/* ������ƿ�ָ�� */
  if(pdPASS == xReturn)
    printf("����Key_Task����ɹ�!\n");
  
  /* ����Uart_Task���� */
  xReturn = xTaskCreate((TaskFunction_t )Uart_Task,  /* ������ں��� */
                        (const char*    )"Uart_Task",/* �������� */
                        (uint16_t       )512,  /* ����ջ��С */
                        (void*          )NULL,/* ������ں������� */
                        (UBaseType_t    )3, /* ��������ȼ� */
                        (TaskHandle_t*  )&Uart_Task_Handle);/* ������ƿ�ָ�� */ 
  if(pdPASS == xReturn)
    printf("����Uart_Task����ɹ�!\n");

  /* ����task_parse_and_execute_command���� */
  xReturn = xTaskCreate(
		task_parse_and_execute_command,
		"Task_CmdParseExec",
		configMINIMAL_STACK_SIZE + 512, // ����ջ�ռ䣬��������Ҫ��scanf�����޴�Ŀռ�
		NULL,
		tskIDLE_PRIORITY + 1, // ���ú��ʵ����ȼ�
		&xTaskCommand_Handle
	);
  if(pdPASS == xReturn)
	 printf("����task_parse_and_execute_command����ɹ�!\r\n");

  /* ����task_parse_and_execute_command���� */
  xReturn = xTaskCreate(vTaskKalmanFilter, "KalmanTask", configMINIMAL_STACK_SIZE + 128, NULL, tskIDLE_PRIORITY + 3, &xKalmanTaskHandle);
  	if(pdPASS == xReturn)
	  printf("����KalmanTask����ɹ�!\r\n");



 //������ʱ������
 	 Adctimer_Handle=xTimerCreate((const char*		)"AutoReloadTimer",
                            (TickType_t			)1000,/* ����1000(tick) */
                            (UBaseType_t		)pdTRUE,/* ѭ��ģʽ */
                            (void*				  )1,/* �˶�ʱ����Ψһ����ID */
                            (TimerCallbackFunction_t)ADC_Task); /*��ʱ���Ļص�����*/
	 if(Adctimer_Handle != NULL)                          
	  {
	    printf("����Adctimer_Handle��ʱ���ɹ�!\r\n");

	    xTimerStart(Adctimer_Handle,0);	//������ʱ�������������ʱ���Ķ������������ȴ�
	  }                            


	// ====== LED自动变色定时器已注释（调试按键控制LED时禁用）======
	// Ledtimer_Handle=xTimerCreate((const char*		)"LedTimer",
	// 							(TickType_t 		)500,/* ����1000(tick) */
	// 							(UBaseType_t		)pdTRUE,/* ѭ��ģʽ */
	// 							(void*				  )2,/* �˶�ʱ����Ψһ����ID */
	// 							(TimerCallbackFunction_t)ColorLED); /*��ʱ���Ļص�����*/
	// if(Adctimer_Handle != NULL)
	//  {
	// 	printf("����Ledtimer_Handle��ʱ���ɹ�!\r\n");
	// 	xTimerStart(Ledtimer_Handle,0); //������ʱ�������������ʱ���Ķ������������ȴ�
	//  } 						   
	 
	 

							
  vTaskDelete(AppTaskCreate_Handle); //ɾ��AppTaskCreate����
  
  taskEXIT_CRITICAL();            //�˳��ٽ���
}

/**********************************************************************
  * @ ������  �� LED_Task
  * @ ����˵���� LED_Task
  * @ ����    ��   
  * @ ����ֵ  �� ��
  ********************************************************************/

static void LED_Task(void* pvParameters)
{
	 while (1)
  	{
  	 xEventGroupWaitBits(KEY_Event_Handle, /* �¼������� */ 
						 KEY1_EVENT|KEY2_EVENT,/* �����������Ȥ���¼� */ 
						 pdTRUE, /* �˳�ʱ����¼�λ */ 
						 pdFALSE, /* �������Ȥ�������¼� */ 
						 portMAX_DELAY);/* ָ����ʱ�¼�,һֱ�� */

		LED2_TOGGLE;
	}
}

/**********************************************************************
  * @ ������  �� KEY_Task
  * @ ����˵���� KEY_Task
  * @ ����    ��   
  * @ ����ֵ  �� ��
  ********************************************************************/
static void Key_Task(void* parameter)
{	
  BaseType_t xReturn = pdPASS;/* ����һ��������Ϣ����ֵ��Ĭ��ΪpdPASS */
  uint32_t r_queue;	/* ����һ��������Ϣ�ı��� */
  while (1)
  {
#if 0
	  /* ���ж�ȡ�����գ����ȴ�ʱ��Ϊһֱ�ȴ� */
	    xReturn = xQueueReceive( Test_Queue,    /* ��Ϣ���еľ�� */
	                             &r_queue,      /* ���͵���Ϣ���� */
	                             portMAX_DELAY); /* �ȴ�ʱ�� һֱ�� */
									
			if(pdPASS == xReturn)
			{
			
				CANx_ReceiveStandardFrame();
				printf("�����жϵ��� KEY%d !\n",r_queue);
			}
			else
			{
				printf("���ݽ��ճ���\n");
			}
			
	    LED1_TOGGLE;
#endif

		
		if ( Key_Scan(KEY1_GPIO_PORT,KEY1_PIN) == KEY_ON ) { 
		 printf ( "KEY1 ������\n" ); 
		 /* ����һ���¼� 1 */ 
		 xEventGroupSetBits(KEY_Event_Handle,KEY1_EVENT); //��һ����λ���Ժ�Ϳ����ˣ���ΪLED�Ǳ߲���������Ļ��������λ��
		 } 
		//��� KEY2 ������ 
		 if ( Key_Scan(KEY2_GPIO_PORT,KEY2_PIN) == KEY_ON ) { 
		 printf ( "KEY2 ������\n" ); 
		 /* ����һ���¼� 2 */ 
		 xEventGroupSetBits(KEY_Event_Handle,KEY2_EVENT); //��һ����λ���Ժ�Ϳ����ˣ���ΪLED�Ǳ߲���������Ļ��������λ��
		 } 
		 vTaskDelay(20); //ÿ 20ms ɨ��һ��
  	}
}

/**********************************************************************
  * @ ������  �� Uart_Task
  * @ ����˵���� Uart_Task��������
  * @ ����    ��   
  * @ ����ֵ  �� ��
  ********************************************************************/
static void Uart_Task(void* parameter)
{	
	BaseType_t xReturn = pdPASS;/* ����һ��������Ϣ����ֵ��Ĭ��ΪpdPASS */
  while (1)
  {
    //��ȡ��ֵ�ź��� xSemaphore,û��ȡ����һֱ�ȴ�
		xReturn = xSemaphoreTake(BinarySem_Handle,/* ��ֵ�ź������ */
                              portMAX_DELAY); /* �ȴ�ʱ�� */
    if(pdPASS == xReturn)
    {
      LED2_TOGGLE;
      printf("�յ�����:%s\n",Usart_Rx_Buf);
      memset(Usart_Rx_Buf,0,USART_RBUFF_SIZE);/* ���� */
    }
  }
}

/**********************************************************************
  * @ ������  �� task_parse_and_execute_command
  * @ ����˵���� I2C��д�������� ��ȡ���ڽ��յ�������дeeprom
  * @ ����    ��   
  * @ ����ֵ  �� ��
  ********************************************************************/


void task_parse_and_execute_command(void* parameter)
{
    char command_line[256];
    char command_word[10];
    char temp_str[10];
    int addr, data;
    Command_t cmd;

    // �����о���Ƿ���Ч
    if(xQueueUartCommand == NULL)
    {
        printf("ERROR: Queue not initialized in task_parse_and_execute_command!\r\n");
        while(1);
    }

    I2C_Soft_Init(); // ��ʼ��I2C

    printf("=== EEPROM Control Console ===\r\n");
    printf("Commands:\r\n");
    printf("  write addr <address> data <value> -> Write data to EEPROM\r\n");
    printf("  read addr <address>             -> Read data from EEPROM\r\n");
    printf("Example: write addr 1 data 5\r\n");
    printf("Example: read addr 1\r\n");
    printf("==================================\r\n");

    while(1) // ѭ���������˳�
    {
        // �Ӷ����н���һ�������������ַ���
        // �������Ϊ�գ��˺������Զ������������ó�CPU����������
        if(xQueueReceive(xQueueUartCommand, command_line, portMAX_DELAY) == pdTRUE)//��ȡ���ڽ��յ������û�л�ȡ��һֱ�ȴ�
        {
            printf("Received Command: %s\r\n", command_line);

            // ʹ�� sscanf ���ַ����н���������Ӱ�ȫ�ɿ�
            // ���� "write addr X data Y"
            if(sscanf(command_line, "%s %s %d %s %d", command_word, temp_str, &addr, temp_str, &data) == 5)
            {
                if(strcasecmp(command_word, "write") == 0 && strcasecmp(temp_str, "data") == 0)
                {
                    cmd.type = CMD_TYPE_WRITE;
                    cmd.addr = (uint16_t)addr;
                    cmd.data = (uint8_t)data;
                    
                    printf("Executing: Write 0x%02X to Address %d\r\n", cmd.data, cmd.addr);
                    if(AT24C02_WriteOneByte(cmd.addr, cmd.data) == 0)
                    {
                        printf("Write Success!\r\n");
                    }
                    else
                    {
                        printf("Write Failed!\r\n");
                    }
                    continue; // �����������꣬�����ȴ���һ��
                }
            }

            // ���� "read addr X"
            if(sscanf(command_line, "%s %s %d", command_word, temp_str, &addr) == 3)
            {
                if(strcasecmp(command_word, "read") == 0 && strcasecmp(temp_str, "addr") == 0)
                {
                    cmd.type = CMD_TYPE_READ;
                    cmd.addr = (uint16_t)addr;
                    
                    printf("Executing: Read from Address %d\r\n", cmd.addr);
                    uint8_t read_value = AT24C02_ReadOneByte(cmd.addr);
                    printf("Read Result: Address %d -> Data 0x%02X (%d)\r\n", cmd.addr, read_value, read_value);
                    continue; // �����������꣬�����ȴ���һ��
                }
            }

            // ������϶�ƥ�䲻�ϣ���Ϊδ֪����
            printf("Unrecognized command: %s. Type 'help' for a list of commands.\r\n", command_line);
        }
    }
    // vTaskDelete(NULL); // �Ƴ�����
}


//adc����ȡֵ
static void ADC_Task(void* parameter)
{	

  
		ADC_ConvertedValueLocal[0] =(float) ADC_ConvertedValue[0]/4096*(float)3.3;
		ADC_ConvertedValueLocal[1] =(float) ADC_ConvertedValue[1]/4096*(float)3.3;
		ADC_ConvertedValueLocal[2] =(float) ADC_ConvertedValue[2]/4096*(float)3.3;
		
	//	printf(" CH1_PA6 value = %f V ",ADC_ConvertedValueLocal[0]);
	//	printf(" CH2_PB8 value = %f V ",ADC_ConvertedValueLocal[1]);
	//	printf(" CH3_PB9 value = %f V \r\n",ADC_ConvertedValueLocal[2]);
		ADC_ConvertedValueLocal[2] *=1000;//��Ϊ����֪ͨ�Ǵ�ul��ʽ�����������������ʽ����
   		xTaskNotify(xKalmanTaskHandle, ADC_ConvertedValueLocal[2], eSetValueWithOverwrite);
}


/***********************************************************************
  * @ ������  �� BSP_Init
  * @ ����˵���� �弶�����ʼ�������а����ϵĳ�ʼ�����ɷ��������������
  * @ ����    ��   
  * @ ����ֵ  �� ��
  *********************************************************************/
static void BSP_Init(void)
{
	/*
	 * STM32�ж����ȼ�����Ϊ4����4bit��������ʾ��ռ���ȼ�����ΧΪ��0~15
	 * ���ȼ�����ֻ��Ҫ����һ�μ��ɣ��Ժ������������������Ҫ�õ��жϣ�
	 * ��ͳһ��������ȼ����飬ǧ��Ҫ�ٷ��飬�мɡ�
	 */
	NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );
	
	/* LED ��ʼ�� */
	LED_GPIO_Config();

	/* ����DMA��ʼ��	*/
	USARTx_DMA_Config();
	
	/* ���ڳ�ʼ��	*/
	Debug_USART_Config();
  
	/* ������ʼ��	*/
	Key_GPIO_Config();

	
	/* ������ʼ��	*/
	EXTI_Key_Config();

	/*adc����*/
	Rheostat_Init();	

	/*CAN�ػ�����*/
	CANx_Init();

	/*PWM�����ò�LED*/
// [注释] 	ColorLED_Config();

#ifdef __PWM_H
	/* ��ʼ���߼����ƶ�ʱ�����벶���Լ�ͨ�ö�ʱ�����PWM */
	 TIMx_Configuration();
#endif
	/*ʹ�����������*/
	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);
	RNG_Cmd(ENABLE);
}

/********************************END OF FILE****************************/

#include <string.h>
#include "W7500x_uart.h"
#include "W7500x_gpio.h"
#include "common.h"
#include "W7500x_board.h"
#include "configdata.h"
#include "uartHandler.h"
#include "seg.h"

#include <stdio.h> // for debugging

/* Private typedef -----------------------------------------------------------*/
RINGBUFF_T txring[DEVICE_UART_CNT];
RINGBUFF_T rxring[DEVICE_UART_CNT];

/* Private define ------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
extern void delay(__IO uint32_t nCount);

/* Private functions ---------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t flag_ringbuf_full[DEVICE_UART_CNT] = {RESET,};

static uint8_t rxbuff[DEVICE_UART_CNT][UART_RRB_SIZE];
static uint8_t txbuff[DEVICE_UART_CNT][UART_SRB_SIZE];

uint8_t word_len_table[] = {7, 8, 9};
uint8_t * parity_table[] = {(uint8_t *)"N", (uint8_t *)"ODD", (uint8_t *)"EVEN"};
uint8_t stop_bit_table[] = {1, 2};
uint8_t * flow_ctrl_table[] = {(uint8_t *)"NONE", (uint8_t *)"XON/XOFF", (uint8_t *)"RTS/CTS", (uint8_t *)"RTS Only"};
uint8_t * uart_if_table[] = {(uint8_t *)UART_IF_STR_RS232_TTL, (uint8_t *)UART_IF_STR_RS422_485};

static uint8_t xonoff_status[DEVICE_UART_CNT] = {UART_XON,};

// RTS Status; __USE_GPIO_HARDWARE_FLOWCONTROL__ defined
#ifdef __USE_GPIO_HARDWARE_FLOWCONTROL__
	static uint8_t rts_status[DEVICE_UART_CNT] = {UART_RTS_LOW,};
#endif

/* UART Interface selecter; RS-422 or RS-485 use only */
static uint8_t uart_if_mode[DEVICE_UART_CNT] = {UART_IF_RS422,};


/* Public functions ----------------------------------------------------------*/

/**
  * @brief  None
  * @param  None
  * @retval None
  */
void S2E_UART_IRQ_Handler(UART_TypeDef * UARTx, uint8_t channel)
{
    struct __serial_option *serial_option = (struct __serial_option *)get_DevConfig_pointer()->serial_option;
    
	uint8_t ch_tx; 
    uint8_t ch_rx; 
	
	if(UART_GetITStatus(UARTx, UART_IT_FLAG_RXI) == SET)
	{
        UART_ClearITPendingBit(UARTx, UART_IT_FLAG_RXI);
        if(RingBuffer_IsFull(&rxring[channel]) == TRUE)
		{
			ch_rx = UART_ReceiveData(UARTx);
			flag_ringbuf_full[channel] = SET;
		}
		else
		{
#ifdef __USE_GPIO_HARDWARE_FLOWCONTROL__
			if(serial_option[channel].flow_control == flow_rts_cts)
			{
				;
			}
#else
            if((serial_option[channel].flow_control == flow_rts_cts) && (RingBuffer_GetCount(&rxring[channel]) > UART_OFF_THRESHOLD)) // CTS/RTS
			{
				; // Does nothing => RTS signal inactive
			}
			else
#endif
			{
				ch_rx = UART_ReceiveData(UARTx);
#ifdef _SEG_DEBUG_
				UART_SendData(UARTx, ch_rx);	// ## UART echo; for debugging
#endif
                if(channel==0)
                {
                    if((check_modeswitch_trigger(ch_rx)) == 0) // ret: [0] data / [1] trigger code
                    {
                        if(check_serial_store_permitted(channel, ch_rx) == 1) // ret: [0] not permitted / [1] permitted
                            RingBuffer_Insert(&rxring[channel], &ch_rx);
                    }
                }
                else
                {
                    if(check_serial_store_permitted(channel, ch_rx) == 1) // ret: [0] not permitted / [1] permitted
                        RingBuffer_Insert(&rxring[channel], &ch_rx);
                }
			}
		}
		init_time_delimiter_timer(channel);
	}

	if(UART_GetITStatus(UARTx, UART_IT_FLAG_TXI) == SET) 
	{
		UART_ClearITPendingBit(UARTx, UART_IT_FLAG_TXI);
        if(RingBuffer_Pop(&txring[channel], &ch_tx) == SUCCESS)
		{
			while(UART_GetFlagStatus(UARTx, UART_FR_TXFF) == SET);
			UART_SendData(UARTx, ch_tx);
		}
        else	
		{			
            UART_ITConfig(UARTx, UART_IT_FLAG_TXI, DISABLE);
		}
	}
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
uint32_t UART_Send_RB(UART_TypeDef* UARTx, RINGBUFF_T *pRB, const void *data, int bytes)
{
	uint32_t ret;
	uint8_t *p8 = (uint8_t *) data;
	uint8_t ch;

	/* Don't let UART transmit ring buffer change in the UART IRQ handler */
	UART_ITConfig(UARTx, UART_IT_FLAG_TXI, DISABLE);

	/* Move as much data as possible into transmit ring buffer */ 
	ret = RingBuffer_InsertMult(pRB, p8 , bytes);
	
	if(RingBuffer_Pop(pRB, &ch))
	{
		while(UART_GetFlagStatus(UARTx, UART_FR_TXFF) == SET);
		UART_SendData(UARTx, ch);
	}
	
	/* Enable UART transmit interrupt */
	UART_ITConfig(UARTx, UART_IT_FLAG_TXI, ENABLE);
	
	return ret;
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
int UART_Read_RB(RINGBUFF_T *pRB, void *data, int bytes)
{
	return RingBuffer_PopMult(pRB, (uint8_t *) data, bytes);
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
void UART_Buffer_Flush(RINGBUFF_T *pRB)
{
	RingBuffer_Flush(pRB);
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
void S2E_UART_Configuration(uint8_t channel)
{
    UART_InitTypeDef UART_InitStructure;
    
    UART_TypeDef* UARTx = (channel==0)?UART0:UART1;
    IRQn_Type UARTx_IRQn = (channel==0)?UART0_IRQn:UART1_IRQn;
    
    /* Ring Buffer */
    RingBuffer_Init(&rxring[channel], rxbuff[channel], 1, UART_RRB_SIZE);
    RingBuffer_Init(&txring[channel], txbuff[channel], 1, UART_SRB_SIZE);
    /* Configure the UART */
    serial_info_init(&UART_InitStructure, channel);
    UART_Init(UARTx,&UART_InitStructure);
    /* Configure UART Interrupt Enable */
	UART_ITConfig(UARTx, (UART_IT_FLAG_TXI | UART_IT_FLAG_RXI), ENABLE);
    /* NVIC configuration */
    NVIC_ClearPendingIRQ(UARTx_IRQn);
	#if 1
	if(channel==0)
		NVIC_SetPriority(UARTx_IRQn, 1);
	else
		NVIC_SetPriority(UARTx_IRQn, 2);
	#endif
    NVIC_EnableIRQ(UARTx_IRQn);
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
void UART2_Configuration(void)
{
	/* Configure UART2: Simple UART */
	/* UART2 configured as follow: 115200-8-N-1 */
	S_UART_Init(115200);
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
void serial_info_init(UART_InitTypeDef* UART_InitStructure, uint8_t channel)
{
    struct __serial_option *serial_option = (struct __serial_option *)get_DevConfig_pointer()->serial_option;
    
	uint32_t valid_arg = 0;
	uint32_t baud_table[14] = {300, 600, 1200, 1800, 2400, 4800, 9600, 14400, 19200, 28800, 38400, 57600, 115200, 230400};

	/* Set Baud Rate */
	if(serial_option[channel].baud_rate < (sizeof(baud_table) / sizeof(baud_table[0])))
	{
		UART_InitStructure->UART_BaudRate = baud_table[serial_option[channel].baud_rate];
		valid_arg = 1;
	}
	
	if(!valid_arg)
		UART_InitStructure->UART_BaudRate = baud_table[baud_115200];

	/* Set Data Bits */
	switch(serial_option[channel].data_bits) {
		case word_len7:
			UART_InitStructure->UART_WordLength = UART_WordLength_7b;
			break;
		case word_len8:
			UART_InitStructure->UART_WordLength = UART_WordLength_8b;
			break;
		default:
			UART_InitStructure->UART_WordLength = UART_WordLength_8b;
			serial_option[channel].data_bits = word_len8;
			break;
	}

	/* Set Stop Bits */
	switch(serial_option[channel].stop_bits) {
		case stop_bit1:
			UART_InitStructure->UART_StopBits = UART_StopBits_1;
			break;
		case stop_bit2:
			UART_InitStructure->UART_StopBits = UART_StopBits_2;
			break;
		default:
			UART_InitStructure->UART_StopBits = UART_StopBits_1;
			serial_option[channel].stop_bits = stop_bit1;
			break;
	}

	/* Set Parity Bits */
	switch(serial_option[channel].parity) {
		case parity_none:
			UART_InitStructure->UART_Parity = UART_Parity_No;
			break;
		case parity_odd:
			UART_InitStructure->UART_Parity = UART_Parity_Odd;
			break;
		case parity_even:
			UART_InitStructure->UART_Parity = UART_Parity_Even;
			break;
		default:
			UART_InitStructure->UART_Parity = UART_Parity_No;
			serial_option[channel].parity = parity_none;
			break;
	}
	
	/* Flow Control */
	if(serial_option[channel].uart_interface == UART_IF_RS232_TTL)
	{
		// RS232 Hardware Flow Control
		//7		RTS		Request To Send		Output
		//8		CTS		Clear To Send		Input
		switch(serial_option[channel].flow_control) {
			case flow_none:
				UART_InitStructure->UART_HardwareFlowControl = UART_HardwareFlowControl_None;
				break;
			case flow_rts_cts:
#ifdef __USE_GPIO_HARDWARE_FLOWCONTROL__
                UART_InitStructure->UART_HardwareFlowControl = UART_HardwareFlowControl_None;
				if(!channel)
				{
					GPIO_Configuration(UART0_RTS_PORT, UART0_RTS_PIN, GPIO_Mode_OUT, UART0_RTS_PAD_AF);
					GPIO_Configuration(UART0_CTS_PORT, UART0_CTS_PIN, GPIO_Mode_IN, UART0_CTS_PAD_AF);
					set_uart_rts_pin_low(channel);
			 	}
    #if (DEVICE_UART_CNT == 2)
				else
				{
					GPIO_Configuration(UART1_RTS_PORT, UART1_RTS_PIN, GPIO_Mode_OUT, UART1_RTS_PAD_AF);
					GPIO_Configuration(UART1_CTS_PORT, UART1_CTS_PIN, GPIO_Mode_IN, UART1_CTS_PAD_AF);
					set_uart_rts_pin_low(channel);
				}
    #endif
#else
				UART_InitStructure->UART_HardwareFlowControl = UART_HardwareFlowControl_RTS_CTS;
#endif
				break;
			case flow_xon_xoff:
				UART_InitStructure->UART_HardwareFlowControl = UART_HardwareFlowControl_None;
				break;
			default:
				UART_InitStructure->UART_HardwareFlowControl = UART_HardwareFlowControl_None;
				break;
		}
	}
	else // UART_IF_RS422_485
	{
		UART_InitStructure->UART_HardwareFlowControl = UART_HardwareFlowControl_None;
		
        /* GPIO configration (RTS pin -> GPIO: 485SEL) */
		if((serial_option[channel].flow_control != flow_rtsonly) && (serial_option[channel].flow_control != flow_reverserts)) // Added by James in March 29
		{
			if(channel == 0)
				get_uart_rs485_sel(SEG_DATA_UART0);
			else
				get_uart_rs485_sel(SEG_DATA_UART1);
		}
        else
		{
			if(serial_option->flow_control == flow_rtsonly)
				uart_if_mode[channel] = UART_IF_RS485;
            else
				uart_if_mode[channel] = UART_IF_RS485_REVERSE;
		}
		
		if(channel == 0)
			uart_rs485_rs422_init(SEG_DATA_UART0);
		else
			uart_rs485_rs422_init(SEG_DATA_UART1);
	}
	
	// GPIO_Configuration(GPIOC, GPIO_Pin_14, GPIO_Mode_IN, PAD_AF1); what?
	
	/* Configure the UARTx */
	UART_InitStructure->UART_Mode = UART_Mode_Rx | UART_Mode_Tx;
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
void check_uart_flow_control(uint8_t channel, uint8_t flow_ctrl)
{
	if(flow_ctrl == flow_xon_xoff)
	{
		if((xonoff_status[channel] == UART_XON) && (RingBuffer_GetCount(&rxring[channel]) > UART_OFF_THRESHOLD))
        {
            (channel==0)?UartPutc(UART0, UART_XOFF):UartPutc(UART1, UART_XOFF);
			xonoff_status[channel] = UART_XOFF;
#ifdef _UART_DEBUG_
            printf(" >> SEND XOFF [%d / %d]\r\n", RingBuffer_GetCount(&rxring[channel]), SEG_DATA_BUF_SIZE);
#endif
		}
		else if((xonoff_status[channel] == UART_XOFF) && (RingBuffer_GetCount(&rxring[channel]) < UART_ON_THRESHOLD))
        {
            (channel==0)?UartPutc(UART0, UART_XON):UartPutc(UART1, UART_XON);
			xonoff_status[channel] = UART_XON;
#ifdef _UART_DEBUG_
            printf(" >> SEND XON [%d / %d]\r\n", RingBuffer_GetCount(&rxring[channel]), SEG_DATA_BUF_SIZE);
#endif
		}
	}
	else if(flow_ctrl == flow_rts_cts) // RTS pin control
	{      
		/* Buffer full occurred */
        if((rts_status[channel] == UART_RTS_LOW) && (RingBuffer_GetCount(&rxring[channel]) > UART_OFF_THRESHOLD))
		{
			set_uart_rts_pin_high(channel);
			rts_status[channel] = UART_RTS_HIGH;
#ifdef _UART_DEBUG_
            printf(" >> UART_RTS_HIGH [%d / %d]\r\n", RingBuffer_GetCount(&rxring[channel]), SEG_DATA_BUF_SIZE);
#endif
		}
		/* Clear the buffer full event */
        if((rts_status[channel] == UART_RTS_HIGH) && (RingBuffer_GetCount(&rxring[channel]) <= UART_OFF_THRESHOLD))
		{
			set_uart_rts_pin_low(channel);
			rts_status[channel] = UART_RTS_LOW;
#ifdef _UART_DEBUG_
			printf(" >> UART_RTS_LOW [%d / %d]\r\n", RingBuffer_GetCount(&rxring[channel]), SEG_DATA_BUF_SIZE);
#endif
		}
	}
}

uint8_t get_uart_rs485_sel(uint8_t uartNum)
{
	if(uartNum == 0) // UART0
	{
		GPIO_Configuration(UART0_RTS_PORT, UART0_RTS_PIN, GPIO_Mode_IN, UART0_RTS_PAD_AF); // UART0 RTS pin: GPIO / Input
		GPIO_SetBits(UART0_RTS_PORT, UART0_RTS_PIN); 
		
		if(GPIO_ReadInputDataBit(UART0_RTS_PORT, UART0_RTS_PIN) == UART_IF_RS422)
			uart_if_mode[uartNum] = UART_IF_RS422;
		else
			uart_if_mode[uartNum] = UART_IF_RS485;
	}
	#if (DEVICE_UART_CNT == 2)
	else
	{
		GPIO_Configuration(UART1_RTS_PORT, UART1_RTS_PIN, GPIO_Mode_IN, UART1_RTS_PAD_AF); // UART1 RTS pin: GPIO / Input
		GPIO_SetBits(UART1_RTS_PORT, UART1_RTS_PIN); 
		
		if(GPIO_ReadInputDataBit(UART1_RTS_PORT, UART1_RTS_PIN) == UART_IF_RS422) // RS-485
			uart_if_mode[uartNum] = UART_IF_RS422;
		else
			uart_if_mode[uartNum] = UART_IF_RS485;
	}
	#endif
	return uart_if_mode[uartNum];
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
void uart_rs485_rs422_init(uint8_t uartNum)
{
	if(uartNum == 0) // UART0
	{
		GPIO_Configuration(UART0_RTS_PORT, UART0_RTS_PIN, GPIO_Mode_OUT, UART0_RTS_PAD_AF); // UART0 RTS pin: GPIO / Output
		
		if(uart_if_mode[uartNum] == UART_IF_RS485)   
			GPIO_ResetBits(UART0_RTS_PORT, UART0_RTS_PIN); // UART0 RTS pin init, Set the signal low
		else 
			GPIO_SetBits(UART0_RTS_PORT, UART0_RTS_PIN); // UART0 RTS pin init, Set the signal low
	}
	#if (DEVICE_UART_CNT == 2)
	else if(uartNum == 1) // UART1
	{
		GPIO_Configuration(UART1_RTS_PORT, UART1_RTS_PIN, GPIO_Mode_OUT, UART1_RTS_PAD_AF); // UART1 RTS pin: GPIO / Output
		
		if(uart_if_mode[uartNum] == UART_IF_RS485) 
			GPIO_ResetBits(UART1_RTS_PORT, UART1_RTS_PIN); // UART1 RTS pin init, Set the signal low
		else 
			GPIO_SetBits(UART1_RTS_PORT, UART1_RTS_PIN); // UART1 RTS pin init, Set the signal low
	}
	#endif 
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
void uart_rs485_enable(uint8_t uartNum)
{
	if(uart_if_mode[uartNum] == UART_IF_RS485)
	{
		// RTS pin -> High
		if(uartNum == 0) // UART0
			GPIO_SetBits(UART0_RTS_PORT, UART0_RTS_PIN);
        #if (DEVICE_UART_CNT == 2)
		else if(uartNum == 1) // UART1
			GPIO_SetBits(UART1_RTS_PORT, UART1_RTS_PIN);
        #endif 
		delay(1);
	}
	else if(uart_if_mode[uartNum] == UART_IF_RS485_REVERSE)
	{
		// RTS pin -> High
		if(uartNum == 0) // UART0
			GPIO_ResetBits(UART0_RTS_PORT, UART0_RTS_PIN);
		#if (DEVICE_UART_CNT == 2)
		else if(uartNum == 1) // UART1
			GPIO_ResetBits(UART1_RTS_PORT, UART1_RTS_PIN);
		#endif 
		delay(1);
	}
	//UART_IF_RS422: None
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
void uart_rs485_disable(uint8_t uartNum)
{
	if(uart_if_mode[uartNum] == UART_IF_RS485)
	{
		// RTS pin -> Low
		if(uartNum == 0) // UART0
			GPIO_ResetBits(UART0_RTS_PORT, UART0_RTS_PIN);
        #if (DEVICE_UART_CNT == 2)
		else if(uartNum == 1) // UART1
			GPIO_ResetBits(UART1_RTS_PORT, UART1_RTS_PIN);
        #endif 
		delay(1);
	}
	//UART_IF_RS422: None
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
uint8_t get_uart_cts_pin(uint8_t uartNum)
{
	uint8_t cts_pin;

#ifdef __USE_GPIO_HARDWARE_FLOWCONTROL__
    #ifdef _UART_DEBUG_
	static uint8_t prev_cts_pin;
    #endif
    
	if(uartNum == 0) // UART0
		cts_pin = GPIO_ReadInputDataBit(UART0_CTS_PORT, UART0_CTS_PIN); 
    #if (DEVICE_UART_CNT == 2)
	else if(uartNum == 1) // UART1
		cts_pin = GPIO_ReadInputDataBit(UART1_CTS_PORT, UART1_CTS_PIN); 
    #endif 
    
    #ifdef _UART_DEBUG_
	if(cts_pin != prev_cts_pin)
	{
		printf(" >> UART_CTS_%s\r\n", cts_pin?"HIGH":"LOW");
		prev_cts_pin = cts_pin;
	}
    #endif
	return cts_pin;
#endif
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
void set_uart_rts_pin_high(uint8_t uartNum)
{
#ifdef __USE_GPIO_HARDWARE_FLOWCONTROL__
	if(uartNum == 0) // UART0
		GPIO_SetBits(UART0_RTS_PORT, UART0_RTS_PIN);
    #if (DEVICE_UART_CNT == 2)
	else if(uartNum == 1) // UART1
		GPIO_SetBits(UART1_RTS_PORT, UART1_RTS_PIN);
    #endif 
#endif
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
void set_uart_rts_pin_low(uint8_t uartNum)
{
#ifdef __USE_GPIO_HARDWARE_FLOWCONTROL__
	if(uartNum == 0) // UART0
		GPIO_ResetBits(UART0_RTS_PORT, UART0_RTS_PIN);
    #if (DEVICE_UART_CNT == 2)
	else if(uartNum == 1) // UART1
		GPIO_ResetBits(UART1_RTS_PORT, UART1_RTS_PIN);
    #endif
#endif
}


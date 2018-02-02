/*******************************************************************************************************************************************************
 * Copyright (c) 2016 <WIZnet Co.,Ltd.> 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*********************************************************************************************************************************************************/
/**
  ******************************************************************************
  * @file    W7500x_stdPeriph_Driver/src/W7500x_uart.c    
  * @author  IOP Team
  * @version v1.0.0
  * @date    26-AUG-2015
  * @brief   This file contains all the functions prototypes for the uart firmware library.       		
  */

/* Includes ------------------------------------------------------------------*/
#include "W7500x_uart.h"

/**
  * @brief  Fills each UART_InitStruct member with its default value.
  * @param  - USART_InitStruct: pointer to a USART_InitTypeDef structure 
  *           which will be initialized.
  * @retval None
  */
void UART_StructInit(UART_InitTypeDef* UART_InitStruct)
{
  /* UART_InitStruct members default value */
  UART_InitStruct->UART_BaudRate = 115200;
  UART_InitStruct->UART_WordLength = UART_WordLength_8b ;
  UART_InitStruct->UART_StopBits = UART_StopBits_1;
  UART_InitStruct->UART_Parity = UART_Parity_No ;
  UART_InitStruct->UART_Mode = UART_Mode_Rx | UART_Mode_Tx;
  UART_InitStruct->UART_HardwareFlowControl = UART_HardwareFlowControl_None ;  
}

/**
  * @brief  Deinitializes the UART peripheral registers to their 
  *         default reset values.
  * @param  - UARTx: Select the UART peripheral. 
  *           This parameter can be one of the following values:
  *             - UART0, UART1. 
  * @retval None
  */
void UART_DeInit(UART_TypeDef *UARTx)
{
    
}

/**
  * @brief  Initializes the UART peripheral according to the specified 
  *         parameters in the USART_InitStruct.
  * @param  - UARTx: Select the UART peripheral. 
  *           This parameter can be one of the following values:
  *             - UART0, UART1.
  *         - UART_InitStruct: pointer to a UART_InitTypeDef structure
  *           that contains the configuration information for the
  *           specified UART peripheral.
  * @retval None
  */
uint32_t UART_Init(UART_TypeDef *UARTx, UART_InitTypeDef* UART_InitStruct)
{
    float baud_divisor;
    uint32_t tmpreg=0x00, uartclock=0x00;
    uint32_t integer_baud = 0x00, fractional_baud = 0x00;

    assert_param(IS_UART_01_PERIPH(UARTx));
    assert_param(IS_UART_WORD_LENGTH(UART_InitStruct->UART_WordLength));
    assert_param(IS_UART_PARITY(UART_InitStruct->UART_Parity));
    assert_param(IS_UART_STOPBITS(UART_InitStruct->UART_StopBits));
    assert_param(IS_UART_HARDWARE_FLOW_CONTROL(UART_InitStruct->UART_HardwareFlowControl));
    assert_param(IS_UART_MODE(UART_InitStruct->UART_Mode));


    UARTx->CR &= ~(UART_CR_UARTEN); 
//////////////////////////////////////////////////////////////////////////////////////
	// Set baudrate
	//CRG->UARTCLK_SSR = CRG_UARTCLK_SSR_RCLK;  // Set UART Clock using internal Oscilator ( 8MHz )
	//uartclock =  (8000000UL) /  (1 << CRG->UARTCLK_PVSR);
	// or
	//CRG->UARTCLK_SSR = CRG_UARTCLK_SSR_OCLK;  // Set UART Clock using external Oscilator
	//uartclock =  (48000000UL) /  (1 << CRG->UARTCLK_PVSR);
	// or
	//CRG->UARTCLK_SSR = CRG_UARTCLK_SSR_MCLK;
	//uartclock =  (48000000UL) /  (1 << CRG->UARTCLK_PVSR);
	
	// ## 20160315 Eric added
	// Set baudrate
	if(GetPLLSource() == CRG_PLL_IFSR_RCLK) // Internal OSC
	{
		CRG->UARTCLK_SSR = CRG_UARTCLK_SSR_RCLK;  // Set UART Clock using internal Oscilator ( 8MHz )
		uartclock =  INTERN_XTAL /  (1 << CRG->UARTCLK_PVSR); // INTERNA_XTAL = (8000000UL)
	}
	else // CRG_PLL_IFSR_OCLK, External OSC
	{
		CRG->UARTCLK_SSR = CRG_UARTCLK_SSR_MCLK;
		uartclock =  GetSystemClock() /  (1 << CRG->UARTCLK_PVSR);
	}
	
//////////////////////////////////////////////////////////////////////////////////////
    baud_divisor = ((float)uartclock / (16 * UART_InitStruct->UART_BaudRate));
    integer_baud = (uint32_t)baud_divisor;
    fractional_baud = (uint32_t)((baud_divisor - integer_baud) * 64 + 0.5);

    UARTx->IBRD = integer_baud;
    UARTx->FBRD = fractional_baud;


    tmpreg = UARTx->LCR_H;
    tmpreg &= ~(0x00EE);
    tmpreg |= (UART_InitStruct->UART_WordLength | UART_InitStruct->UART_StopBits | UART_InitStruct->UART_Parity);
    //UARTx->LCR_H |= tmpreg;
    UARTx->LCR_H = tmpreg;

    tmpreg = UARTx->CR;
    tmpreg &= ~(UART_CR_CTSEn | UART_CR_RTSEn | UART_CR_RXE | UART_CR_TXE | UART_CR_UARTEN);
    tmpreg |= (UART_InitStruct->UART_Mode | UART_InitStruct->UART_HardwareFlowControl);
    //UARTx->CR |= tmpreg;
    UARTx->CR = tmpreg;

    UARTx->CR |= UART_CR_UARTEN;

    return 0;
}


// rx_fifo_level(0=1/8 full, 1=1/4 full, 2=1/2 full, 3=3/4 full, 4=7/8 full)
// tx_fifo_level(0=1/8 full, 1=1/4 full, 2=1/2 full, 3=3/4 full, 4=7/8 full)
void UART_FIFO_Enable(UART_TypeDef *UARTx, uint16_t rx_fifo_level, uint16_t tx_fifo_level)
{
    UARTx->LCR_H |= UART_LCR_H_FEN;
    UARTx->IFLS = (UART_IFLS_RXIFLSEL(rx_fifo_level) | UART_IFLS_TXIFLSEL(tx_fifo_level));
}

void UART_FIFO_Disable(UART_TypeDef *UARTx)
{
    UARTx->LCR_H &= ~(UART_LCR_H_FEN);
}

/**
  * @brief  Transmits single data through the UARTx peripheral.
  * @param  - UARTx: Select the UART peripheral. 
  *           This parameter can be one of the following values:
  *             - UART0, UART1. 
  *         - Data: the data to transmit.
  * @retval None
  */
void UART_SendData(UART_TypeDef* UARTx, uint16_t Data)
{
	/* Check the parameters */
    assert_param(IS_UART_01_PERIPH(UARTx));

    UARTx->DR = Data;
}

/**
  * @brief  Returns the most recent received data by the UARTx peripheral.
  * @param  - UARTx: Select the UART peripheral. 
  *           This parameter can be one of the following values:
  *             - UART0, UART1. 
  * @retval The received data.
  */
uint16_t UART_ReceiveData(UART_TypeDef* UARTx)
{
	/* Check the parameters */
    assert_param(IS_UART_01_PERIPH(UARTx));

    return (uint16_t)(UARTx->DR);
}

/**
  * @brief  Transmits break characters.
  * @param  - UARTx: Select the UART peripheral. 
  *           This parameter can be one of the following values:
  *             - UART0, UART1. 
  *         - Data: the data to transmit.
  * @retval None
  */
void UART_SendBreak(UART_TypeDef* UARTx)
{
	/* Check the parameters */
    assert_param(IS_UART_01_PERIPH(UARTx));

    UARTx->LCR_H |= UART_LCR_H_BRK; 
}

/**
  * @brief  Checks the specified UART receive status.
  *         Notes:
  *				- The received data character must be read first from the UARTDR Register.
  *				- and then reading the error status associated with that data character from the UARTRSR Register. 
  *             - This read sequence cannot be reversed
  *             - because the UARTRSR Register is updated only when a read occurs from the UARTDR Register. 
  * @param  - UARTx: Select the UART peripheral. 
  *           This parameter can be one of the following values:
  *             - UART0, UART1. 
  *         - UART_RECV_STATUS: specifies the receive status to check.
  *           This parameter can be one of the following values:
  *             - UART_RECV_STATUS_OE: Overrun error
  *             - UART_RECV_STATUS_BE: Break error
  *             - UART_RECV_STATUS_PE: Parity error
  *             - UART_RECV_STATUS_FE: Framing error
  * @retval The new state of UART_RECV_STATUS (SET or RESET).
  */
FlagStatus UART_GetRecvStatus(UART_TypeDef* UARTx, uint16_t UART_RECV_STATUS)
{
    FlagStatus bitstatus = RESET;

	/* Check the parameters */
    assert_param(IS_UART_01_PERIPH(UARTx));
    assert_param(IS_UART_RECV_STATUS(UART_RECV_STATUS));

    if( (UARTx->STATUS.RSR & UART_RECV_STATUS) != (uint16_t)RESET)
    {
        bitstatus = SET;
    }
    else
    {
        bitstatus = RESET;
    }

    return bitstatus;
}

/**
  * @brief  Clear the specified UART receive status.
  * @param  - UARTx: Select the UART peripheral. 
  *           This parameter can be one of the following values:
  *             - UART0, UART1. 
  *         - UART_RECV_STATUS: specifies the receive status to clear.
  *           This parameter can be one of the following values:
  *             - UART_RECV_STATUS_OE: Overrun error
  *             - UART_RECV_STATUS_BE: Break error
  *             - UART_RECV_STATUS_PE: Parity error
  *             - UART_RECV_STATUS_FE: Framing error
  * @retval None
  */
void UART_ClearRecvStatus(UART_TypeDef* UARTx, uint16_t UART_RECV_STATUS)
{
	/* Check the parameters */
    assert_param(IS_UART_01_PERIPH(UARTx));
    assert_param(IS_UART_RECV_STATUS(UART_RECV_STATUS));

    UARTx->STATUS.ECR = (uint16_t)UART_RECV_STATUS;
}

/**
  * @brief  Checks whether the specified UART flag is set or not.
  * @param  - UARTx: Select the UART peripheral. 
  *           This parameter can be one of the following values:
  *             - UART0, UART1. 
  *         - UART_FLAG: specifies the flag to check.
  *           This parameter can be one of the following values:
  *             - UART_FLAG_RI:  	Ring indicator flag   
  *             - UART_FLAG_TXFE:	Transmit FIFO empty flag
  *             - UART_FLAG_RXFF:  	Receive FIFO full flag
  *             - UART_FLAG_TXFF:  	Transmit FIFO full flag
  *             - UART_FLAG_RXFE:  	Receive FIFO empty flag
  *             - UART_FLAG_BUSY:  	UART busy flag 
  *             - UART_FLAG_DCD:  	Data carrier detect flag
  *             - UART_FLAG_DSR:  	Data set ready flag 
  *             - UART_FLAG_CTS:  	Clear to send flag 
  * @retval The new state of UART_FLAG (SET or RESET).
  */
FlagStatus UART_GetFlagStatus(UART_TypeDef* UARTx, uint16_t UART_FLAG)
{
    FlagStatus bitstatus = RESET;

	/* Check the parameters */
    assert_param(IS_UART_01_PERIPH(UARTx));
    assert_param(IS_UART_FLAG(UART_FLAG));

    if ((UARTx->FR & UART_FLAG) != (uint16_t)RESET)
    {
        bitstatus = SET;    
    }
    else
    {
        bitstatus = RESET;
    }
 
    return bitstatus;
    
}

/**
  * @brief  Enables or disables the specified UART interrupts.
  * @param  - UARTx: Select the UART peripheral. 
  *           This parameter can be one of the following values:
  *             - UART0, UART1. 
  *         - UART_IT: specifies the USART interrupt sources to be enabled or disabled.
  *           This parameter can be one of the following values:
  *             - UART_IT_FLAG_OEI:  	Overrun error interrupt
  *             - UART_IT_FLAG_BEI:		Break error interrupt
  *             - UART_IT_FLAG_PEI:  	Parity error interrupt
  *             - UART_IT_FLAG_FEI:  	Framing error interrupt
  *             - UART_IT_FLAG_RTI:  	Receive timeout interrupt
  *             - UART_IT_FLAG_TXI:  	Transmit interrupt
  *             - UART_IT_FLAG_RXI:  	Receive interruptt 
  *             - UART_IT_FLAG_DSRMI:  	UARTDSR modem interrupt
  *             - UART_IT_FLAG_DCDMI:  	UARTDCD modem interrupt
  *             - UART_IT_FLAG_CTSMI:  	UARTCTS modem interrupt 
  *             - UART_IT_FLAG_RIMI:	UARTRI modem  interrupt
  *         - NewState: new state of the specified UARTx interrupts.
  *           This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void UART_ITConfig(UART_TypeDef* UARTx, uint16_t UART_IT, FunctionalState NewState)
{
	/* Check the parameters */
    assert_param(IS_UART_01_PERIPH(UARTx));
    assert_param(IS_UART_IT_FLAG(UART_IT));

    if ( NewState != DISABLE )
    {
        UARTx->IMSC |= UART_IT;
    }
    else
    {
		UARTx->IMSC &= ~UART_IT;
    }
}

/**
  * @brief  Checks whether the specified UART interrupt has Enables or disables
  * @param  - UARTx: Select the UART peripheral. 
  *           This parameter can be one of the following values:
  *             - UART0, UART1. 
  *         - UART_IT: specifies the USART interrupt sources to be enabled or disabled.
  *           This parameter can be one of the following values:
  *             - UART_IT_FLAG_OEI:  	Overrun error interrupt
  *             - UART_IT_FLAG_BEI:		Break error interrupt
  *             - UART_IT_FLAG_PEI:  	Parity error interrupt
  *             - UART_IT_FLAG_FEI:  	Framing error interrupt
  *             - UART_IT_FLAG_RTI:  	Receive timeout interrupt
  *             - UART_IT_FLAG_TXI:  	Transmit interrupt
  *             - UART_IT_FLAG_RXI:  	Receive interruptt 
  *             - UART_IT_FLAG_DSRMI:  	UARTDSR modem interrupt
  *             - UART_IT_FLAG_DCDMI:  	UARTDCD modem interrupt
  *             - UART_IT_FLAG_CTSMI:  	UARTCTS modem interrupt 
  *             - UART_IT_FLAG_RIMI:	UARTRI modem  interrupt
  * @retval The new state of UART_IT (SET or RESET).
  */
ITStatus UART_GetITConfigStatus(UART_TypeDef* UARTx, uint16_t UART_IT)
{
	ITStatus bitstatus = RESET;
	
	/* Check the parameters */
    assert_param(IS_UART_01_PERIPH(UARTx));
    assert_param(IS_UART_IT_FLAG(UART_IT));

    if ((UARTx->IMSC & UART_IT) != (uint16_t)RESET)
    {
        bitstatus = SET;
    }
    else
    {
        bitstatus = RESET;
    }

    return bitstatus;
}

/**
  * @brief  Checks whether the specified UART interrupt has occurred or not.
  * @param  - UARTx: Select the UART peripheral. 
  *           This parameter can be one of the following values:
  *             - UART0, UART1. 
  *         - UART_IT: specifies the UART interrupt source to check.
  *           This parameter can be one of the following values:
  *             - UART_IT_FLAG_OEI:  	Overrun error interrupt
  *             - UART_IT_FLAG_BEI:		Break error interrupt
  *             - UART_IT_FLAG_PEI:  	Parity error interrupt
  *             - UART_IT_FLAG_FEI:  	Framing error interrupt
  *             - UART_IT_FLAG_RTI:  	Receive timeout interrupt
  *             - UART_IT_FLAG_TXI:  	Transmit interrupt
  *             - UART_IT_FLAG_RXI:  	Receive interruptt 
  *             - UART_IT_FLAG_DSRMI:  	UARTDSR modem interrupt
  *             - UART_IT_FLAG_DCDMI:  	UARTDCD modem interrupt
  *             - UART_IT_FLAG_CTSMI:  	UARTCTS modem interrupt 
  *             - UART_IT_FLAG_RIMI:	UARTRI modem  interrupt
  * @retval The new state of UART_IT (SET or RESET).
  */
ITStatus UART_GetITStatus(UART_TypeDef* UARTx, uint16_t UART_IT)
{
    ITStatus bitstatus = RESET;

	/* Check the parameters */
    assert_param(IS_UART_01_PERIPH(UARTx));
    assert_param(IS_UART_IT_FLAG(UART_IT));

    if ((UARTx->MIS & UART_IT) != (uint16_t)RESET)
    {
        bitstatus = SET;
    }
    else
    {
        bitstatus = RESET;
    }

    return bitstatus;
}

/**
  * @brief  Clears the UART interrupt pending bits.
  * @param  - UARTx: Select the UART peripheral. 
  *           This parameter can be one of the following values:
  *             - UART0, UART1. 
  *         - UART_IT: specifies the UART interrupt source to check.
  *           This parameter can be one of the following values:
  *             - UART_IT_FLAG_OEI:  	Overrun error interrupt
  *             - UART_IT_FLAG_BEI:		Break error interrupt
  *             - UART_IT_FLAG_PEI:  	Parity error interrupt
  *             - UART_IT_FLAG_FEI:  	Framing error interrupt
  *             - UART_IT_FLAG_RTI:  	Receive timeout interrupt
  *             - UART_IT_FLAG_TXI:  	Transmit interrupt
  *             - UART_IT_FLAG_RXI:  	Receive interruptt 
  *             - UART_IT_FLAG_DSRMI:  	UARTDSR modem interrupt
  *             - UART_IT_FLAG_DCDMI:  	UARTDCD modem interrupt
  *             - UART_IT_FLAG_CTSMI:  	UARTCTS modem interrupt 
  *             - UART_IT_FLAG_RIMI:	UARTRI modem  interrupt
  * @retval None
  */
void UART_ClearITPendingBit(UART_TypeDef* UARTx, uint16_t UART_IT)
{
	/* Check the parameters */
    assert_param(IS_UART_01_PERIPH(UARTx));
    assert_param(IS_UART_IT_FLAG(UART_IT));

    UARTx->ICR |= UART_IT;
}

uint32_t S_UART_Init(uint32_t baud)
{
    uint32_t tmpreg=0x00;

    S_UART_SetBaud(baud);

    tmpreg = UART2->CTRL;
    tmpreg &= ~(S_UART_CTRL_RX_EN | S_UART_CTRL_TX_EN);
    tmpreg |= (S_UART_CTRL_RX_EN | S_UART_CTRL_TX_EN);
    UART2->CTRL = tmpreg;

    return 0;
}

void S_UART_DeInit()
{

}

void S_UART_SetBaud(uint32_t baud)
{
    uint32_t uartclock = 0x00, integer_baud = 0x00;
	
    assert_param(IS_UART_MODE(S_UART_InitStruct->UART_Mode));
    
    // ## Eric added
	if((CRG->FCLK_SSR == CRG_FCLK_SSR_RCLK) || (CRG->FCLK_SSR == CRG_FCLK_SSR_OCLK))
	{
		uartclock = GetSourceClock();
	}
	else
	{
		uartclock = GetSystemClock();
	}
	
/*
    if(CRG->FCLK_SSR == CRG_FCLK_SSR_RCLK)
    {
        uartclock = INTERN_XTAL;
    }
    else if(CRG->FCLK_SSR == CRG_FCLK_SSR_OCLK)
    {
        uartclock = EXTERN_XTAL;
    }
    else
    {
        uartclock = GetSystemClock();
    }
*/
    integer_baud = (uint32_t)(uartclock / baud);
    UART2->BAUDDIV = integer_baud;
}

void S_UART_SendData(uint16_t Data)
{
    while(UART2->STATE & S_UART_STATE_TX_BUF_FULL); 
    UART2->DATA = Data;
}

uint16_t S_UART_ReceiveData()
{
    return (uint16_t)(UART2->DATA);
}


FlagStatus S_UART_GetFlagStatus(uint16_t S_UART_STATE)
{
    FlagStatus bitstatus = RESET;

    assert_param(IS_S_UART_STATE(S_UART_STATE));

    if ((UART2->STATE & S_UART_STATE) != (FlagStatus)RESET)
    {
        bitstatus = SET;
    }
    else
    {
        bitstatus = RESET;
    }

    return bitstatus;
}

void S_UART_SetCTRL(uint16_t S_UART_CTRL, FunctionalState NewState)
{
    if ( NewState != DISABLE )
    {
        UART2->CTRL |= S_UART_CTRL;
    }
    else
    {
        UART2->CTRL &= ~(S_UART_CTRL);
    }
}


void S_UART_ITConfig(uint16_t S_UART_CTRL, FunctionalState NewState)
{
    assert_param(IS_S_UART_CTRL_FLAG(S_UART_CTRL));

    if ( NewState != DISABLE )
    {
        UART2->CTRL |= S_UART_CTRL;
    }
    else
    {
        UART2->CTRL &= ~(S_UART_CTRL);
    }
}

ITStatus S_UART_GetITStatus(uint16_t S_UART_INTSTATUS)
{
    ITStatus bitstatus = RESET;

    assert_param(IS_S_UART_INTSATUS(S_UART_INTSTATUS));

    if ((UART2->INT.STATUS & (S_UART_INTSTATUS)) != (uint16_t) RESET)
    {
        bitstatus = SET;
    }
    else
    {
        bitstatus = RESET;
    }

    return bitstatus;
}

void S_UART_ClearITPendingBit(uint16_t S_UART_INTSTATUS)
{
    assert_param(IS_S_UART_INTSATUS(S_UART_INTSTATUS));

    UART2->INT.CLEAR |= (S_UART_INTSTATUS);
}


/**************************************************/
// It will be moved to application board's driver */
/**************************************************/
uint8_t UartPutc(UART_TypeDef* UARTx, uint8_t ch)
{
    UART_SendData(UARTx,ch);

    while(UARTx->FR & UART_FR_BUSY);

    return (ch);
}

void UartPuts(UART_TypeDef* UARTx, uint8_t *str)
{
    uint8_t ch;

    do{
        ch = *str;
        if(ch != (uint8_t)0x0)
        {
            UartPutc(UARTx, ch);
        }
        *str++;
    }while(ch != 0);
}

uint8_t UartGetc(UART_TypeDef* UARTx)
{
    while(UARTx->FR & UART_FR_RXFE);
	
    return (UARTx->DR & 0xFF);
}


uint8_t S_UartPutc(uint8_t ch)
{
    S_UART_SendData(ch);

    return (ch);
}

void S_UartPuts(uint8_t *str)
{
    uint8_t ch;

    do{
        ch = *str;
        if(ch != (uint8_t)0x0)
        {
            S_UART_SendData(ch);
        }
        *str++;
    }while(ch != 0);
}

uint8_t S_UartGetc()
{
    while( (UART2->STATE & S_UART_STATE_RX_BUF_FULL) == 0 ); 
    return (uint8_t)S_UART_ReceiveData();
}



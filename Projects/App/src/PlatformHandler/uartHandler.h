#ifndef UARTHANDLER_H_
#define UARTHANDLER_H_

#include <stdint.h>
#include "W7500x_uart.h"
#include "common.h"
#include "ConfigData.h"
#include "ring_buffer.h"

#define _UART_DEBUG_

#define UART_SRB_SIZE 1024	/* Send */
#define UART_RRB_SIZE 1024	/* Receive */

// XON/XOFF: Transmitter On / Off, Software flow control
#define UART_XON				0x11 // 17
#define UART_XOFF				0x13 // 19
#define UART_ON_THRESHOLD	(uint16_t)(SEG_DATA_BUF_SIZE / 10)
#define UART_OFF_THRESHOLD	(uint16_t)(SEG_DATA_BUF_SIZE - UART_ON_THRESHOLD)

// UART interface selector, RS-232/TTL or RS-422/485
#define UART_IF_RS232_TTL			0
#define UART_IF_RS422_485			1
#define UART_IF_STR_RS232_TTL		"RS-232/TTL"
#define UART_IF_STR_RS422_485		"RS-422/485"

#define UART_IF_RS422				0
#define UART_IF_RS485				1
#define UART_IF_RS485_REVERSE	    2				//Added by James in March 29

// If the define '__USE_UART_IF_SELECTOR__' disabled, default UART interface is selected to be 'UART_IF_DEFAULT'
#define UART_IF_DEFAULT				UART_IF_RS232_TTL
//#define UART_IF_DEFAULT				UART_IF_RS422_485

// UART RTS/CTS pins
// RTS: output, CTS: input

#define UART_RTS_HIGH			1
#define UART_RTS_LOW			0

#define UART_CTS_HIGH			1
#define UART_CTS_LOW			0

enum baud {
	baud_300 = 0,
	baud_600 = 1,
	baud_1200 = 2,
	baud_1800 = 3,
	baud_2400 = 4,
	baud_4800 = 5,
	baud_9600 = 6,
	baud_14400 = 7,
	baud_19200 = 8,
	baud_28800 = 9,
	baud_38400 = 10,
	baud_57600 = 11,
	baud_115200 = 12,
	baud_230400 = 13
};
enum word_len {
	word_len7 = 0,
	word_len8 = 1,
	word_len9 = 2
};

enum stop_bit {
	stop_bit1 = 0,
	stop_bit2 = 1
};

enum parity {
	parity_none = 0,
	parity_odd = 1,
	parity_even = 2
};

enum flow_ctrl {
	flow_none = 0,
	flow_xon_xoff = 1,
	flow_rts_cts = 2,
	flow_rtsonly = 3,  // RTS_ONLY
	flow_reverserts = 4 // Reverse RTS
};

extern uint8_t flag_ringbuf_full[DEVICE_UART_CNT];

//extern uint32_t baud_table[]; // 14
extern uint8_t word_len_table[];
extern uint8_t stop_bit_table[];
extern uint8_t * parity_table[];
extern uint8_t * flow_ctrl_table[];
extern uint8_t * uart_if_table[];

void S2E_UART_IRQ_Handler(UART_TypeDef * UARTx, uint8_t channel);
void S2E_UART_Configuration(uint8_t channel);
void UART2_Configuration(void);


// #1 XON/XOFF Software flow control: Check the Buffer usage and Send the start/stop commands
void check_uart_flow_control(uint8_t socket, uint8_t flow_ctrl);
void serial_info_init(UART_InitTypeDef* UART_InitStructure, uint8_t channel);
uint32_t UART_Send_RB(UART_TypeDef* UARTx, RINGBUFF_T *pRB, const void *data, int bytes);
int UART_Read_RB(RINGBUFF_T *pRB, void *data, int bytes);
void UART_Buffer_Flush(RINGBUFF_T *buf);


// Hardware flow control by GPIOs (RTS/CTS)
uint8_t get_uart_cts_pin(uint8_t uartNum);
void set_uart_rts_pin_high(uint8_t uartNum);
void set_uart_rts_pin_low(uint8_t uartNum);

// UART interface selector by GPIO
uint8_t get_uart_rs485_sel(uint8_t uartNum);
void uart_rs485_rs422_init(uint8_t uartNum);
void uart_rs485_disable(uint8_t uartNum);
void uart_rs485_enable(uint8_t uartNum);

//#define MIN(_a, _b) (_a < _b) ? _a : _b
#define MEM_FREE(mem_p) do{ if(mem_p) { free(mem_p); mem_p = NULL; } }while(0)	//

#endif /* UARTHANDLER_H_ */

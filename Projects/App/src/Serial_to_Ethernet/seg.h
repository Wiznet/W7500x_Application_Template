#ifndef SEG_H_
#define SEG_H_

#include <stdint.h>
#include "common.h"

//#define _SEG_DEBUG_

///////////////////////////////////////////////////////////////////////////////////////////////////////

#define SEG_DATA_UART0		SOCK_CHANNEL0	
#define SEG_DATA_UART1		SOCK_CHANNEL1	
#define SEG_DEBUG_UART		2	

#define SEG_DATA_BUF_SIZE	DATA_BUF_SIZE	// UART Ring buffer size

///////////////////////////////////////////////////////////////////////////////////////////////////////
#define DEFAULT_MODESWITCH_INTER_GAP	500 // 500ms (0.5sec)

//#define MIXED_CLIENT_INFINITY_CONNECT
#ifndef MIXED_CLIENT_INFINITY_CONNECT
	#define MIXED_CLIENT_LIMITED_CONNECT 		//  TCP_MIXED_MODE: TCP CLIENT - limited count of connection retries
	#define MAX_RECONNECTION_COUNT			10
#endif

#define MAX_CONNECTION_AUTH_TIME		5000 // 5000ms (5sec)
///////////////////////////////////////////////////////////////////////////////////////////////////////

extern uint8_t opmode;
extern uint8_t flag_s2e_application_running;
extern uint8_t flag_process_dhcp_success;
extern uint8_t flag_process_dns_success;
extern char * str_working[];


//typedef enum{SEG_UART_RX, SEG_UART_TX, SEG_ETHER_RX, SEG_ETHER_TX, SEG_ALL} teDATADIR;

// Serial to Ethernet function handler; call by main loop
void do_seg(void);

// Timer for S2E core operations
void seg_timer_sec(void);
void seg_timer_msec(void);

void init_trigger_modeswitch(uint8_t mode);

void set_device_status(uint8_t socket, teDEVSTATUS status);
uint8_t get_device_status(uint8_t channel);

uint8_t process_socket_termination(uint8_t channel);

// Send Keep-alive packet manually (once)
void send_keepalive_packet_manual(uint8_t sock);

//These functions must be located in UART Rx IRQ Handler.
uint8_t check_serial_store_permitted(uint8_t channel, uint8_t ch);
uint8_t check_modeswitch_trigger(uint8_t ch);	// Serial command mode switch trigger code (3-bytes) checker
void init_time_delimiter_timer(uint8_t chanel); 			// Serial data packing option [Time]: Timer enalble function for Time delimiter

// UART tx/rx and Ethernet tx/rx data transfer bytes counter
//void clear_data_transfer_bytecount(uint8_t channel, teDATADIR dir);
//uint32_t get_data_transfer_bytecount(uint8_t channel, teDATADIR dir);

#endif /* SEG_H_ */


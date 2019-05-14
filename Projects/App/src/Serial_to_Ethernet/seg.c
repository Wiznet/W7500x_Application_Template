
/* Includes ------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>

#include "common.h"

#include "W7500x_wztoe.h"
#include "W7500x_gpio.h"
#include "W7500x_board.h"

#include "socket.h"
#include "seg.h"

#include "timerHandler.h"
#include "uartHandler.h"
#include "gpioHandler.h"

/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Ring Buffer */
extern RINGBUFF_T txring[DEVICE_UART_CNT];
extern RINGBUFF_T rxring[DEVICE_UART_CNT];
uint8_t flag_s2e_application_running = RESET;
uint8_t opmode = DEVICE_GW_MODE;
static uint8_t mixed_state[DEVICE_UART_CNT] = {MIXED_SERVER,};
static uint8_t sw_modeswitch_at_mode_on = DISABLE;
static uint16_t client_any_port[DEVICE_UART_CNT] = {0,};
/* Timer Enable flags */
uint8_t enable_inactivity_timer[DEVICE_UART_CNT] = {DISABLE,};
volatile uint16_t inactivity_time[DEVICE_UART_CNT] = {0,};
uint8_t enable_keepalive_timer[DEVICE_UART_CNT] = {DISABLE,};
volatile uint16_t keepalive_time[DEVICE_UART_CNT] = {0,};
uint8_t enable_modeswitch_timer = DISABLE;
volatile uint16_t modeswitch_time = 0;
volatile uint16_t modeswitch_gap_time = DEFAULT_MODESWITCH_INTER_GAP;
uint8_t enable_reconnection_timer[DEVICE_UART_CNT] = {DISABLE,};
volatile uint16_t reconnection_time[DEVICE_UART_CNT] = {0,};
uint8_t enable_serial_input_timer[DEVICE_UART_CNT] = {DISABLE,};
volatile uint16_t serial_input_time[DEVICE_UART_CNT] = {0,};
uint8_t flag_serial_input_time_elapse[DEVICE_UART_CNT] = {RESET,}; // for Time delimiter
/* added for auth timeout */
uint8_t enable_connection_auth_timer[DEVICE_UART_CNT] = {DISABLE,};
volatile uint16_t connection_auth_time[DEVICE_UART_CNT] = {0,};
/* flags */
uint8_t flag_connect_pw_auth[DEVICE_UART_CNT] = {RESET,}; // TCP_SERVER_MODE only
uint8_t flag_sent_keepalive[DEVICE_UART_CNT] = {RESET,};
uint8_t flag_sent_first_keepalive[DEVICE_UART_CNT] = {RESET,};
/* static variables for function: check_modeswitch_trigger() */
static uint8_t triggercode_idx;
static uint8_t ch_tmp[3];
/* User's buffer / size idx */
extern uint8_t g_send_buf[DEVICE_UART_CNT][DATA_BUF_SIZE];
extern uint8_t g_recv_buf[DEVICE_UART_CNT][DATA_BUF_SIZE+2];
volatile uint32_t u2e_size[DEVICE_UART_CNT] = {0,};
volatile uint32_t e2u_size[DEVICE_UART_CNT] = {0,};
/* UDP: Peer netinfo */
uint8_t peerip[4] = {0, };
uint8_t peerip_tmp[4] = {0xff, };
uint16_t peerport = 0;
/* XON/XOFF (Software flow control) flag, Serial data can be transmitted to peer when XON enabled. */
uint8_t isXON[DEVICE_UART_CNT] = {SET,};

char *str_working[] = {"TCP_CLIENT_MODE", "TCP_SERVER_MODE", "TCP_MIXED_MODE", "UDP_MODE"};
uint8_t flag_process_dhcp_success = RESET;
uint8_t flag_process_dns_success = RESET;
uint8_t isSocketOpen_TCPclient[DEVICE_UART_CNT] = {OFF,};

// ## timeflag for debugging
uint8_t tmp_timeflag_for_debug = RESET;

uint32_t check_tx_rb_old_status[DEVICE_UART_CNT] = {0,};
uint32_t check_tx_rb_now_status[DEVICE_UART_CNT] = {0,};
uint32_t check_tx_rb_status_cnt[DEVICE_UART_CNT] = {0,};

/* Private functions prototypes ----------------------------------------------*/
void proc_SEG_tcp_client(uint8_t channel);
void proc_SEG_tcp_server(uint8_t channel);
void proc_SEG_tcp_mixed(uint8_t channel);
void proc_SEG_udp(uint8_t channel);
void uart_to_ether(uint8_t channel);
void ether_to_uart(uint8_t channel);
uint16_t get_serial_data(uint8_t channel);
void reset_SEG_timeflags(uint8_t channel);
uint8_t check_connect_pw_auth(uint8_t * buf, uint16_t len, uint8_t channel);
void restore_serial_data(uint8_t idx);
uint8_t check_tcp_connect_exception(uint8_t channel);
void set_device_status(uint8_t socket, teDEVSTATUS status);
uint16_t get_tcp_any_port(uint8_t channel);
static void check_n_clear_uart_recv_status(uint8_t channel);

extern uint16_t sock_remained_size[_WIZCHIP_SOCK_NUM_]; //M20160411

/* Public & Private functions ------------------------------------------------*/

/**
  * @brief  None
  * @param  None
  * @retval None
  */
void do_seg(void)
{
	struct __network_connection *network_connection = (struct __network_connection *)get_DevConfig_pointer()->network_connection;
	struct __serial_option *serial_option = (struct __serial_option *)get_DevConfig_pointer()->serial_option;
	struct __firmware_update *firmware_update = (struct __firmware_update *)&(get_DevConfig_E_pointer()->firmware_update);
	
	uint8_t i;
	
#if 0	
	if(tmp_timeflag_for_debug == SET) // every 1 sec
	{
		tmp_timeflag_for_debug = RESET;
	}
#endif
	
	 
	if(firmware_update->fwup_flag == SET) return;	// Firmware update: Do not run SEG process

	
	/* Serial AT command mode enabled, initial settings */
	if((opmode == DEVICE_GW_MODE) && (sw_modeswitch_at_mode_on == ENABLE))
	{
		for(i=0; i<DEVICE_UART_CNT; i++)
			process_socket_termination(i);			// Socket disconnect (TCP only)
		
		init_trigger_modeswitch(DEVICE_AT_MODE);	// Mode switch 
		sw_modeswitch_at_mode_on = DISABLE;			// Mode switch flag disabled 
	}
	
	if(opmode == DEVICE_GW_MODE) 
	{
        for(i=0; i<DEVICE_UART_CNT; i++)
        {
            switch(network_connection[i].working_mode)
            {
                case TCP_CLIENT_MODE:
                    proc_SEG_tcp_client(i);
                    break;
                
                case TCP_SERVER_MODE:
                    proc_SEG_tcp_server(i);
                    break;
                
                case TCP_MIXED_MODE:
                    proc_SEG_tcp_mixed(i);
                    break;
                
                case UDP_MODE:
                    proc_SEG_udp(i);
                    break;
                
                default:
                    break;
            }
        
			check_n_clear_uart_recv_status(i);
            // XON/XOFF Software flow control: Check the Buffer usage and Send the start/stop commands
            // [WIZnet Device] -> [Peer]
            if((serial_option[i].flow_control == flow_xon_xoff) || serial_option[i].flow_control == flow_rts_cts) 
            {
                check_uart_flow_control(i, serial_option[i].flow_control);
            }
        }
	}
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
void set_device_status(uint8_t channel, teDEVSTATUS status)
{
	struct __network_connection *network_connection = (struct __network_connection *)get_DevConfig_pointer()->network_connection;
	
	switch(status)
	{
		case ST_OPEN:		/* TCP connection: disconnected (or UDP mode) */
			network_connection[channel].working_state = ST_OPEN;
			break;
		
		case ST_CONNECT:	/* TCP connection: connected */
			network_connection[channel].working_state = ST_CONNECT; 
			break;
		
		case ST_UPGRADE:	/* TCP connection: disconnected */
			network_connection[channel].working_state = ST_UPGRADE;
			break;
		
		case ST_ATMODE:		/* TCP connection: disconnected */
			network_connection[channel].working_state = ST_ATMODE;
			break;
		
		case ST_UDP:		/* UDP mode */
			network_connection[channel].working_state = ST_UDP;
		default:
			break;
	}
	
	/* Status indicator pins */
	if(network_connection[channel].working_state == ST_CONNECT)
		set_connection_status_io(TCPCONNECT, channel, ON);  	// Status I/O pin to low
	else
		set_connection_status_io(TCPCONNECT, channel, OFF); 	// Status I/O pin to high
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
uint8_t get_device_status(uint8_t channel)
{
	struct __network_connection *network_connection = (struct __network_connection *)get_DevConfig_pointer()->network_connection;
    
	return network_connection[channel].working_state;
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
void proc_SEG_udp(uint8_t channel)
{
	struct __network_connection *network_connection = (struct __network_connection *)get_DevConfig_pointer()->network_connection;
	struct __serial_common *serial_common = (struct __serial_common *)&get_DevConfig_pointer()->serial_common;
    struct __serial_data_packing *serial_data_packing = (struct __serial_data_packing *)get_DevConfig_pointer()->serial_data_packing;
	
	uint8_t state = getSn_SR(channel);
    
	switch(state)
	{
		case SOCK_UDP:
            if(RingBuffer_GetCount(&rxring[channel]) || u2e_size[channel]) uart_to_ether(channel);
			if(getSn_RX_RSR(channel) || e2u_size[channel]) ether_to_uart(channel);
			break;
			
		case SOCK_CLOSED:
			UART_Buffer_Flush(&rxring[channel]);
			UART_Buffer_Flush(&txring[channel]);
			u2e_size[channel] = 0;
			e2u_size[channel] = 0;
			
			if(socket(channel, Sn_MR_UDP, network_connection[channel].local_port, 0x00) == channel)
			{
				set_device_status(channel, ST_UDP);
				
				if(serial_data_packing[0].packing_time) 
                    modeswitch_gap_time = serial_data_packing[0].packing_time; // replace the GAP time (default: 500ms)
				
				if(serial_common->serial_debug_en == ENABLE)
					printf(" > SEG[%d]:UDP_MODE:SOCKOPEN\r\n", channel);
			}
			break;
			
		default:
			break;
	}
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
void proc_SEG_tcp_client(uint8_t channel)
{
	struct __tcp_option *tcp_option = (struct __tcp_option *)get_DevConfig_pointer()->tcp_option;
	struct __network_connection *network_connection = (struct __network_connection *)get_DevConfig_pointer()->network_connection;
	struct __serial_command *serial_command = (struct __serial_command *)&get_DevConfig_pointer()->serial_command;
	struct __serial_common *serial_common = (struct __serial_common *)&get_DevConfig_pointer()->serial_common;
	struct __serial_data_packing *serial_data_packing = (struct __serial_data_packing *)(get_DevConfig_pointer()->serial_data_packing);
	
	uint16_t source_port;
	uint8_t destip[4] = {0, };
	uint16_t destport = 0;
	
	uint8_t state = getSn_SR(channel);
    
	switch(state)
	{
		case SOCK_INIT:
			if(reconnection_time[channel] >= tcp_option[channel].reconnection)
			{
				reconnection_time[channel] = 0; // reconnection time variable clear
				
				/* TCP connect exception checker; e.g., dns failed / zero srcip ... and etc. */
				if(check_tcp_connect_exception(channel) == ERROR) 
                {
                    return;
                }
				
				/* TCP connect */
				connect(channel, network_connection[channel].remote_ip, network_connection[channel].remote_port);
#ifdef _SEG_DEBUG_
				printf(" > SEG[%d]:TCP_CLIENT_MODE:CLIENT_CONNECTION\r\n", channel);
#endif
			}
			break;
		
		case SOCK_ESTABLISHED:
			/* TCP client mode initialize after connection established (only once) */
			if(getSn_IR(channel) & Sn_IR_CON)
			{
				set_device_status(channel, ST_CONNECT);
				
				if(!inactivity_time[channel] && tcp_option[channel].inactivity)		
                {
                    enable_inactivity_timer[channel] = ENABLE;
                }
				if(!keepalive_time[channel] && tcp_option[channel].keepalive_en)	
                {
                    enable_keepalive_timer[channel] = ENABLE;
                }
				
				// TCP server mode only, This flag have to be enabled always at TCP client mode
				//if(option->pw_connect_en == ENABLE)		flag_connect_pw_auth = SET;
				flag_connect_pw_auth[channel] = SET;
				
				/* Reconnection timer disable */
				if(enable_reconnection_timer[channel] == ENABLE)
				{
					enable_reconnection_timer[channel] = DISABLE;
					reconnection_time[channel] = 0;
				}
				
				/* Serial debug message printout */
				if(serial_common->serial_debug_en == ENABLE)
				{
					getsockopt(channel, SO_DESTIP, &destip);
					getsockopt(channel, SO_DESTPORT, &destport);
					printf(" > SEG[%d]:CONNECTED TO - %d.%d.%d.%d : %d\r\n", channel, destip[0], destip[1], destip[2], destip[3], destport);
				}

                UART_Buffer_Flush(&rxring[channel]);
				UART_Buffer_Flush(&txring[channel]);

				check_tx_rb_old_status[channel] = 0; 
				check_tx_rb_now_status[channel] = 0;
				check_tx_rb_status_cnt[channel] = 0;
				
				
				/* Debug message enable flag: TCP client sokect open */
				isSocketOpen_TCPclient[channel] = OFF;
				
				setSn_IR(channel, Sn_IR_CON);
			}
			
			/* Serial to Ethernet process */
            if(RingBuffer_GetCount(&rxring[channel]) || u2e_size[channel])  
			{
				uart_to_ether(channel);
			}
			if(getSn_RX_RSR(channel) || e2u_size[channel])		
			{
				ether_to_uart(channel);
			}
			
			/* Check the inactivity timer */
			if((enable_inactivity_timer[channel] == ENABLE) && (inactivity_time[channel] >= tcp_option[channel].inactivity))
			{
				//disconnect(sock);
				process_socket_termination(channel);
				
				/* Keep-alive timer disabled */
				enable_keepalive_timer[channel] = DISABLE;
				keepalive_time[channel] = 0;
#ifdef _SEG_DEBUG_
				printf(" > [%d]INACTIVITY TIMER: TIMEOUT\r\n", channel);
#endif
			}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
			
			/* Check the keee-alive timer */
			if((tcp_option[channel].keepalive_en == ENABLE) && (enable_keepalive_timer[channel] == ENABLE))
			{
				/* Send the first keee-alive packet */
				if((flag_sent_first_keepalive[channel] == RESET) 
                    && (keepalive_time[channel] >= tcp_option[channel].keepalive_wait_time) 
                    && (tcp_option[channel].keepalive_wait_time != 0))
				{
#ifdef _SEG_DEBUG_
					printf(" >> [%d]send_keepalive_packet_first [%d]\r\n", channel, keepalive_time);
#endif
					send_keepalive_packet_manual(channel); // <-> send_keepalive_packet_auto()
					keepalive_time[channel] = 0;
					
					flag_sent_first_keepalive[channel] = SET;
				}
				/* Send the keee-alive packet periodically */
				if((flag_sent_first_keepalive[channel] == SET) 
                    && (keepalive_time[channel] >= tcp_option[channel].keepalive_retry_time) 
                    && (tcp_option[channel].keepalive_retry_time != 0))
				{
#ifdef _SEG_DEBUG_
					printf(" >> [%d]send_keepalive_packet_manual [%d]\r\n", channel, keepalive_time[channel]);
#endif
					send_keepalive_packet_manual(channel);
					keepalive_time[channel] = 0;
				}
			}
			
			break;
		
		case SOCK_CLOSE_WAIT:
			while(getSn_RX_RSR(channel) || e2u_size[channel]) 
            {
                ether_to_uart(channel);
            }
			disconnect(channel);
			break;
		
		case SOCK_FIN_WAIT:
		case SOCK_CLOSED:
			set_device_status(channel, ST_OPEN);
			reset_SEG_timeflags(channel);
			
			u2e_size[channel] = 0;
			e2u_size[channel] = 0;
			
			source_port = get_tcp_any_port(channel);
#ifdef _SEG_DEBUG_
			printf(" > [%d]TCP CLIENT: client_any_port = %d\r\n", channel, client_any_port[channel]);
#endif		
			//if(socket(channel, Sn_MR_TCP, source_port, Sn_MR_ND) == channel)
            if(socket(channel, Sn_MR_TCP, source_port, 0) == channel)
			{
				/* Replace the command mode switch code GAP time (default: 500ms) */
				if((serial_command->serial_command == ENABLE) && serial_data_packing[0].packing_time) 
                {
                    modeswitch_gap_time = serial_data_packing[0].packing_time;
                }
				
				/* Enable the reconnection Timer */
				if((enable_reconnection_timer[channel] == DISABLE) && tcp_option[channel].reconnection) 
                {
                    enable_reconnection_timer[channel] = ENABLE;
                }
				
				if(serial_common->serial_debug_en == ENABLE)
				{
					if(isSocketOpen_TCPclient[channel] == OFF)
					{
						printf(" > SEG[%d]:TCP_CLIENT_MODE:SOCKOPEN\r\n", channel);
						isSocketOpen_TCPclient[channel] = ON;
					}
				}
			}
			break;
		default:
			break;
	}
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
void proc_SEG_tcp_server(uint8_t channel)
{
	struct __tcp_option *tcp_option = (struct __tcp_option *)get_DevConfig_pointer()->tcp_option;
	struct __serial_common *serial_common = (struct __serial_common *)&get_DevConfig_pointer()->serial_common;
	struct __network_connection *network_connection = (struct __network_connection *)(get_DevConfig_pointer()->network_connection);
	struct __serial_command *serial_command = (struct __serial_command *)&get_DevConfig_pointer()->serial_command;
    struct __serial_data_packing *serial_data_packing = (struct __serial_data_packing *)get_DevConfig_pointer()->serial_data_packing;
    
	uint8_t destip[4] = {0, };
	uint16_t destport = 0;
	
	uint8_t state = getSn_SR(channel);
	switch(state)
	{
		case SOCK_INIT:
			//listen(sock); Function call Immediately after socket open operation
			break;
		
		case SOCK_LISTEN:
			break;
		
		case SOCK_ESTABLISHED:
			/* S2E: TCP server mode initialize after connection established (only once) */
			if(getSn_IR(channel) & Sn_IR_CON)
			{
				setSn_IR(channel, Sn_IR_CON);
				
				set_device_status(channel, ST_CONNECT);
				
				if(!inactivity_time[channel] && tcp_option[channel].inactivity)		
                {
                    enable_inactivity_timer[channel] = ENABLE;
                }
				
				//if(!keepalive_time && net->keepalive_en)	enable_keepalive_timer = ENABLE;
				if(tcp_option[channel].pw_connect_en == DISABLE)	
                {
                    flag_connect_pw_auth[channel] = SET;		// TCP server mode only (+ mixed_server)
                }
				else
				{
					/* Connection password auth timer initialize */
					enable_connection_auth_timer[channel] = ENABLE;
					connection_auth_time[channel]  = 0;
				}
				
				/* Serial debug message printout */
				if(serial_common->serial_debug_en == ENABLE)
				{
					getsockopt(channel, SO_DESTIP, &destip);
					getsockopt(channel, SO_DESTPORT, &destport);
					printf(" > SEG[%d]:CONNECTED FROM - %d.%d.%d.%d : %d\r\n", channel, destip[0], destip[1], destip[2], destip[3], destport);
				}
				
                UART_Buffer_Flush(&rxring[channel]);
				UART_Buffer_Flush(&txring[channel]);

				check_tx_rb_old_status[channel] = 0; 
				check_tx_rb_now_status[channel] = 0;
				check_tx_rb_status_cnt[channel] = 0;
			}
			
			/* Serial to Ethernet process */
			if(RingBuffer_GetCount(&rxring[channel]) || u2e_size[channel]) 
            {
                uart_to_ether(channel);
            }

			if(getSn_RX_RSR(channel) || e2u_size[channel])	
            {
                ether_to_uart(channel);
            }
			
			/* Check the inactivity timer */
			if((enable_inactivity_timer[channel] == ENABLE) && (inactivity_time[channel] >= tcp_option[channel].inactivity))
			{
				process_socket_termination(channel);
				
				/* Keep-alive timer disabled */
				enable_keepalive_timer[channel] = DISABLE;
				keepalive_time[channel] = 0;
#ifdef _SEG_DEBUG_
				printf(" > [%d]INACTIVITY TIMER: TIMEOUT\r\n", channel);
#endif
			}
			
			/* Check the keee-alive timer */
			if((tcp_option[channel].keepalive_en == ENABLE) && (enable_keepalive_timer[channel] == ENABLE))
			{
				/* Send the first keee-alive packet */
				if((flag_sent_first_keepalive[channel] == RESET) 
                    && (keepalive_time[channel] >= tcp_option[channel].keepalive_wait_time) 
                    && (tcp_option[channel].keepalive_wait_time != 0))
				{
#ifdef _SEG_DEBUG_
					printf(" >> [%d]send_keepalive_packet_first [%d]\r\n", channel, keepalive_time[channel]);
#endif
					send_keepalive_packet_manual(channel); // <-> send_keepalive_packet_auto()
					keepalive_time[channel] = 0;
					
					flag_sent_first_keepalive[channel] = SET;
				}
				/* Send the keee-alive packet periodically */
				if((flag_sent_first_keepalive[channel] == SET) 
                    && (keepalive_time[channel] >= tcp_option[channel].keepalive_retry_time) 
                    && (tcp_option[channel].keepalive_retry_time != 0))
				{
#ifdef _SEG_DEBUG_
					printf(" >> [%d]send_keepalive_packet_manual [%d]\r\n", channel, keepalive_time[channel]);
#endif
					send_keepalive_packet_manual(channel);
					keepalive_time[channel] = 0;
				}
			}
			
			/* Check the connection password auth timer */
			if(tcp_option[channel].pw_connect_en == ENABLE)
			{
				if((flag_connect_pw_auth[channel] == RESET) && (connection_auth_time[channel] >= MAX_CONNECTION_AUTH_TIME)) // timeout default: 5000ms (5 sec)
				{
					process_socket_termination(channel);
					
					enable_connection_auth_timer[channel] = DISABLE;
					connection_auth_time[channel] = 0;
#ifdef _SEG_DEBUG_
					printf(" > [%d]CONNECTION PW: AUTH TIMEOUT\r\n", channel);
#endif
				}
			}
			break;
		
		case SOCK_CLOSE_WAIT:
			while(getSn_RX_RSR(channel) || e2u_size[channel]) 
            {
				/* receive remaining packets */
                ether_to_uart(channel);
            }
			disconnect(channel);
			break;
		
		case SOCK_FIN_WAIT:
		case SOCK_CLOSED:
			set_device_status(channel, ST_OPEN);
			reset_SEG_timeflags(channel);
			
			u2e_size[channel] = 0;
			e2u_size[channel] = 0;

			//if(socket(channel, Sn_MR_TCP, network_connection[channel].local_port, Sn_MR_ND) == channel)
            if(socket(channel, Sn_MR_TCP, network_connection[channel].local_port, 0) == channel)
			{
				/* Replace the command mode switch code GAP time (default: 500ms) */
				if((serial_command->serial_command == ENABLE) && serial_data_packing[0].packing_time) 
                {
                    modeswitch_gap_time = serial_data_packing[0].packing_time;
                }
				
				listen(channel);
				
				if(serial_common->serial_debug_en == ENABLE)
				{
					printf(" > SEG[%d]:TCP_SERVER_MODE:SOCKOPEN\r\n", channel);
				}
			}
			break;
			
		default:
			break;
	}
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
void proc_SEG_tcp_mixed(uint8_t channel)
{
	struct __tcp_option *tcp_option = (struct __tcp_option *)get_DevConfig_pointer()->tcp_option;
    struct __network_connection *network_connection = (struct __network_connection *)get_DevConfig_pointer()->network_connection;
    struct __serial_common *serial_common = (struct __serial_common *)&get_DevConfig_pointer()->serial_common;
    struct __serial_command *serial_command = (struct __serial_command *)&get_DevConfig_pointer()->serial_command;
    struct __serial_data_packing *serial_data_packing = (struct __serial_data_packing *)&get_DevConfig_pointer()->serial_data_packing;
    
	uint16_t source_port = 0;
	uint8_t destip[4] = {0, };
	uint16_t destport = 0;
	
#ifdef MIXED_CLIENT_LIMITED_CONNECT
	static uint8_t reconnection_count[2];
#endif
	
	uint8_t state = getSn_SR(channel);
	switch(state)
	{
		case SOCK_INIT:
			if(mixed_state[channel] == MIXED_CLIENT)
			{
				if(reconnection_time[channel] >= tcp_option[channel].reconnection)
				{
					reconnection_time[channel] = 0; // reconnection time variable clear
					
					/* TCP connect exception checker; e.g., dns failed / zero srcip ... and etc. */
					if(check_tcp_connect_exception(channel) == ERROR)
					{
#ifdef MIXED_CLIENT_LIMITED_CONNECT
						process_socket_termination(channel);
						reconnection_count[channel] = 0;
						
                        UART_Buffer_Flush(&rxring[channel]);
						UART_Buffer_Flush(&txring[channel]);

						check_tx_rb_old_status[channel] = 0; 
						check_tx_rb_now_status[channel] = 0;
						check_tx_rb_status_cnt[channel] = 0;
                        
						mixed_state[channel] = MIXED_SERVER;
#endif
						return;
					}
					
					/* TCP connect */
					connect(channel, network_connection[channel].remote_ip, network_connection[channel].remote_port);
					
#ifdef MIXED_CLIENT_LIMITED_CONNECT
					reconnection_count[channel]++;
					if(reconnection_count[channel] >= MAX_RECONNECTION_COUNT)
					{
						process_socket_termination(channel);
						reconnection_count[channel] = 0;
                        UART_Buffer_Flush(&rxring[channel]);
                        
						mixed_state[channel] = MIXED_SERVER;
					}
	#ifdef _SEG_DEBUG_
					if(reconnection_count[channel] != 0)	
                    {
                        printf(" > SEG[%d]:TCP_MIXED_MODE:CLIENT_CONNECTION [%d]\r\n", channel, reconnection_count);
                    }
					else						
                    {
                        printf(" > SEG[%d]:TCP_MIXED_MODE:CLIENT_CONNECTION_RETRY FAILED\r\n", channel);
                    }
	#endif
#endif
				}
			}			
			break;
		
		case SOCK_LISTEN:
			// UART Rx interrupt detection in MIXED_SERVER mode
			// => Switch to MIXED_CLIENT mode
            if((mixed_state[channel] == MIXED_SERVER) && RingBuffer_GetCount(&rxring[channel]))
			{
				process_socket_termination(channel);
				mixed_state[channel] = MIXED_CLIENT;
				
				reconnection_time[channel] = tcp_option[channel].reconnection; // rapid initial connection
			}
			break;
		
		case SOCK_ESTABLISHED:
			/* S2E: TCP mixed (server or client) mode initialize after connection established (only once) */
			if(getSn_IR(channel) & Sn_IR_CON)
			{
				set_device_status(channel, ST_CONNECT);
				
				if((inactivity_time[channel] == 0) && (tcp_option[channel].inactivity > 0))		
                    enable_inactivity_timer[channel] = ENABLE;
				
				if((keepalive_time[channel] == 0) && (tcp_option[channel].keepalive_en == ENABLE))	
                    enable_keepalive_timer[channel] = ENABLE;
				
				/* Connection Password option: TCP server mode only (+ mixed_server) */
				if((tcp_option[channel].pw_connect_en == DISABLE) || (mixed_state[channel] == MIXED_CLIENT))
				{
					flag_connect_pw_auth[channel] = SET;
				}
				else if((mixed_state[channel] == MIXED_SERVER) && (flag_connect_pw_auth[channel] == RESET))
				{
					/* Connection password auth timer initialize */
					enable_connection_auth_timer[channel] = ENABLE;
					connection_auth_time[channel]  = 0;
				}
				
				/* Serial debug message printout */
				if(serial_common->serial_debug_en == ENABLE)
				{
					getsockopt(channel, SO_DESTIP, &destip);
					getsockopt(channel, SO_DESTPORT, &destport);
					
					if(mixed_state[channel] == MIXED_SERVER)		
                    {
                        printf(" > SEG[%d]:CONNECTED FROM - %d.%d.%d.%d : %d\r\n", channel, destip[0], destip[1], destip[2], destip[3], destport);
                    }
					else								
                    {
                        printf(" > SEG[%d]:CONNECTED TO - %d.%d.%d.%d : %d\r\n", channel, destip[0], destip[1], destip[2], destip[3], destport);
                    }
				}
				
				/* Mixed mode option init */
				if(mixed_state[channel] == MIXED_SERVER)
				{
					UART_Buffer_Flush(&rxring[channel]);
					UART_Buffer_Flush(&txring[channel]);

					check_tx_rb_old_status[channel] = 0; 
					check_tx_rb_now_status[channel] = 0;
					check_tx_rb_status_cnt[channel] = 0;
				}
				else if(mixed_state[channel] == MIXED_CLIENT)
				{
					/* Mixed-mode flag switching in advance */
					mixed_state[channel] = MIXED_SERVER;
				}
				
#ifdef MIXED_CLIENT_LIMITED_CONNECT
				reconnection_count[channel] = 0;
#endif
				
				setSn_IR(channel, Sn_IR_CON);
			}
			
			/* Serial to Ethernet process */
            if(RingBuffer_GetCount(&rxring[channel]) || u2e_size[channel])	
            {
                uart_to_ether(channel);
            }
			if(getSn_RX_RSR(channel) || e2u_size[channel])		
            {
                ether_to_uart(channel);
            }
			
			/* Check the inactivity timer */
			if((enable_inactivity_timer[channel] == ENABLE) && (inactivity_time[channel] >= tcp_option[channel].inactivity))
			{
				process_socket_termination(channel);
				
				/* Keep-alive timer disabled */
				enable_keepalive_timer[channel] = DISABLE;
				keepalive_time[channel] = 0;
#ifdef _SEG_DEBUG_
				printf(" > [%d]INACTIVITY TIMER: TIMEOUT\r\n", channel);
#endif
				/* TCP mixed mode state transition: initial state */
				mixed_state[channel] = MIXED_SERVER;
			}
			
			/* Check the keee-alive timer */
			if((tcp_option[channel].keepalive_en == ENABLE) && (enable_keepalive_timer[channel] == ENABLE))
			{
				/* Send the first keee-alive packet */
				if((flag_sent_first_keepalive[channel] == RESET) 
                    && (keepalive_time[channel] >= tcp_option[channel].keepalive_wait_time) 
                    && (tcp_option[channel].keepalive_wait_time != 0))
				{
#ifdef _SEG_DEBUG_
					printf(" >> [%d]send_keepalive_packet_first [%d]\r\n", channel, keepalive_time[channel]);
#endif
					send_keepalive_packet_manual(channel); // <-> send_keepalive_packet_auto()
					keepalive_time[channel] = 0;
					
					flag_sent_first_keepalive[channel] = SET;
				}
				/* Send the keee-alive packet periodically */
				if((flag_sent_first_keepalive[channel] == SET) 
                    && (keepalive_time[channel] >= tcp_option[channel].keepalive_retry_time) 
                    && (tcp_option[channel].keepalive_retry_time != 0))
				{
#ifdef _SEG_DEBUG_
					printf(" >> [%d]send_keepalive_packet_manual [%d]\r\n", channel, keepalive_time[channel]);
#endif
					send_keepalive_packet_manual(channel);
					keepalive_time[channel] = 0;
				}
			}
			
			/* Check the connection password auth timer */
			if((mixed_state[channel] == MIXED_SERVER) && (tcp_option[channel].pw_connect_en == ENABLE))
			{
				if((flag_connect_pw_auth[channel] == RESET) && (connection_auth_time[channel] >= MAX_CONNECTION_AUTH_TIME)) // timeout default: 5000ms (5 sec)
				{
					//disconnect(sock);
					process_socket_termination(channel);
					
					enable_connection_auth_timer[channel] = DISABLE;
					connection_auth_time[channel] = 0;
#ifdef _SEG_DEBUG_
					printf(" > [%d]CONNECTION PW: AUTH TIMEOUT\r\n", channel);
#endif
				}
			}
			break;
		
		case SOCK_CLOSE_WAIT:
			while(getSn_RX_RSR(channel) || e2u_size[channel])
            {
                ether_to_uart(channel); // receive remaining packets
            }
			disconnect(channel);
			break;
		
		case SOCK_FIN_WAIT:
		case SOCK_CLOSED:
			set_device_status(channel, ST_OPEN);

			if(mixed_state[channel] == MIXED_SERVER) // MIXED_SERVER
			{
				reset_SEG_timeflags(channel);
				
				u2e_size[channel] = 0;
				e2u_size[channel] = 0;
				
				if(socket(channel, Sn_MR_TCP, network_connection[channel].local_port, Sn_MR_ND) == channel)
				{
					/* Replace the command mode switch code GAP time (default: 500ms) */
					if((serial_command->serial_command == ENABLE) && serial_data_packing[0].packing_time) 
                    {
                        modeswitch_gap_time = serial_data_packing[0].packing_time;
                    }
					
					// TCP Server listen
					listen(channel);
					
					if(serial_common->serial_debug_en == ENABLE)
					{
						printf(" > SEG[%d]:TCP_MIXED_MODE:SERVER_SOCKOPEN\r\n", channel);
					}
				}
			}
			else	// MIXED_CLIENT
			{
				e2u_size[channel] = 0;
				
				source_port = get_tcp_any_port(channel);
#ifdef _SEG_DEBUG_
				printf(" > [%d]TCP CLIENT: any_port = %d\r\n", channel, source_port);
#endif		
				if(socket(channel, Sn_MR_TCP, source_port, Sn_MR_ND) == channel)
				{
					/* Replace the command mode switch code GAP time (default: 500ms) */
					if((serial_command->serial_command == ENABLE) && serial_data_packing[0].packing_time) 
                    {
                        modeswitch_gap_time = serial_data_packing[0].packing_time;
                    }
					
					/* Enable the reconnection Timer */
					if((enable_reconnection_timer[channel] == DISABLE) && tcp_option[channel].reconnection) 
                    {
                        enable_reconnection_timer[channel] = ENABLE;
                    }
					
					if(serial_common->serial_debug_en == ENABLE)
					{
						printf(" > SEG[%d]:TCP_MIXED_MODE:CLIENT_SOCKOPEN\r\n", channel);
					}
				}
			}
			break;
			
		default:
			break;
	}
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
void uart_to_ether(uint8_t channel)
{
	struct __network_connection *network_connection = (struct __network_connection *)(get_DevConfig_pointer()->network_connection);
	struct __serial_common *serial_common = (struct __serial_common *)&get_DevConfig_pointer()->serial_common;
    struct __tcp_option *tcp_option = (struct __tcp_option *)get_DevConfig_pointer()->tcp_option;
    
	uint16_t len;
	int16_t sent_len;
    
#if ((DEVICE_BOARD_NAME == WIZ750SR) || (DEVICE_BOARD_NAME == WIZ750SR_1xx))
	if(get_phylink() != 0) return; // PHY link down
#endif
	
	// UART ring buffer -> user's buffer
	len = get_serial_data(channel);
	
	/*
	// ## for debugging
	if(len)
	{
		printf("flag_connect_pw_auth: %d\r\n", flag_connect_pw_auth);
		printf("uart_to_ether: ");
		for(i = 0; i < len; i++) printf("%c ", g_send_buf[i]);
		printf("\r\n");
	}
	*/
	
	
	if(len > 0)
	{
		/*
		// ## for debugging
		printf("> U2E len: %d, ", len); // ## for debugging
		for(i = 0; i < len; i++) printf("%c", g_send_buf[i]);
		printf("\r\n");
		*/
		
		switch(getSn_SR(channel))
		{
			case SOCK_UDP: // UDP_MODE
				if((network_connection[channel].remote_ip[0] == 0x00) 
                    && (network_connection[channel].remote_ip[1] == 0x00) 
                    && (network_connection[channel].remote_ip[2] == 0x00) 
                    && (network_connection[channel].remote_ip[3] == 0x00))
				{
					if((peerip[0] == 0x00) 
                        && (peerip[1] == 0x00) 
                        && (peerip[2] == 0x00) 
                        && (peerip[3] == 0x00))
					{
						if(serial_common->serial_debug_en == ENABLE) 
                        {
                            printf(" > SEG:UDP_MODE:DATA SEND FAILED - UDP Peer IP/Port required (0.0.0.0)\r\n");
                        }
					}
					else
					{
						// UDP 1:N mode
                        sent_len = (int16_t)sendto(channel, g_send_buf[channel], len, peerip, peerport);
					}
				}
				else
				{
					// UDP 1:1 mode
                    sent_len = (int16_t)sendto(channel, g_send_buf[channel], len, network_connection[channel].remote_ip, network_connection[channel].remote_port);
				}
				
				if(sent_len > 0) 
                {
                    u2e_size[channel]-=sent_len;
                }
				
				break;
			
			case SOCK_ESTABLISHED: // TCP_SERVER_MODE, TCP_CLIENT_MODE, TCP_MIXED_MODE
			case SOCK_CLOSE_WAIT:
				/* Connection password is only checked in the TCP SERVER MODE / TCP MIXED MODE (MIXED_SERVER) */
				if(flag_connect_pw_auth[channel] == SET)
				{
                    sent_len = (int16_t)send(channel, g_send_buf[channel], len);
					if(sent_len > 0) 
                    {
                        u2e_size[channel]-=sent_len;
                    }

					if(tcp_option[channel].keepalive_en == ENABLE)
					{
						if(flag_sent_first_keepalive[channel] == RESET)
						{
							enable_keepalive_timer[channel] = ENABLE;
						}
						else
						{
							flag_sent_first_keepalive[channel] = RESET;
						}
						keepalive_time[channel] = 0;
					}
				}
				break;
			
			case SOCK_LISTEN:
				u2e_size[channel] = 0;
				return;
			
			default:
				break;
		}
	}
 
	inactivity_time[channel] = 0;
	//flag_serial_input_time_elapse = RESET; // this flag is cleared in the 'Data packing delimiter:time' checker routine
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
uint16_t get_serial_data(uint8_t channel)
{
	struct __serial_data_packing *serial_data_packing = (struct __serial_data_packing *)(get_DevConfig_pointer()->serial_data_packing);
    
	uint16_t i;
	uint16_t len;
    uint8_t ch;
	
    len = RingBuffer_GetCount(&rxring[channel]);

	if((len + u2e_size[channel]) >= DATA_BUF_SIZE) // Avoiding u2e buffer (g_send_buf) overflow	
	{
		/* Checking Data packing option: charactor delimiter */
		if((serial_data_packing[channel].packing_delimiter[0] != 0x00) && (len == 1))
		{
            if(UART_Read_RB(&rxring[channel], &ch, 1))
                g_send_buf[channel][u2e_size[channel]] = ch;
			
            if(serial_data_packing[channel].packing_delimiter[0] == g_send_buf[channel][u2e_size[channel]])
				return u2e_size[channel];
		}

		/* serial data length value update for avoiding u2e buffer overflow */
		len = DATA_BUF_SIZE - u2e_size[channel];
	}
	
	if((!serial_data_packing[channel].packing_time) 
        && (!serial_data_packing[channel].packing_size) 
        && (!serial_data_packing[channel].packing_delimiter[0])) // No Packing delimiters.
	{
        UART_Read_RB(&rxring[channel], g_send_buf[channel], len);
        u2e_size[channel] = u2e_size[channel] + len;
        
		return u2e_size[channel];
	}
	else
	{
		/* Checking Data packing options */
		for(i = 0; i < len; i++)
		{
            if(UART_Read_RB(&rxring[channel], &ch, 1))
                g_send_buf[channel][u2e_size[channel]++] = (uint8_t)ch;
			
			/* Packing delimiter: character option */
            if((serial_data_packing[channel].packing_delimiter[0] != 0x00) 
				&& (serial_data_packing[channel].packing_delimiter[0] == g_send_buf[channel][u2e_size[channel] - 1]))
				return u2e_size[channel];
			
			/* Packing delimiter: size option */
			if((serial_data_packing[channel].packing_size != 0) 
				&& (serial_data_packing[channel].packing_size == u2e_size[channel]))
				return u2e_size[channel];
		}
	}
	
	/* Packing delimiter: time option */
	if((serial_data_packing[channel].packing_time != 0) 
        && (u2e_size[channel] != 0) 
        && (flag_serial_input_time_elapse[channel] == SET))
	{
        if(RingBuffer_GetCount(&rxring[channel]) == 0) 
            flag_serial_input_time_elapse[channel] = RESET; // ##
		
		return u2e_size[channel];
	}
	
	return 0;
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
void ether_to_uart(uint8_t channel)
{
	uint16_t data_size[2]={0,};
	
	struct __serial_option *serial_option = (struct __serial_option *)&(get_DevConfig_pointer()->serial_option);
    struct __serial_common *serial_common = (struct __serial_common *)&(get_DevConfig_pointer()->serial_common);
    struct __network_connection *network_connection = (struct __network_connection *)(get_DevConfig_pointer()->network_connection);
    struct __tcp_option *tcp_option = (struct __tcp_option *)(get_DevConfig_pointer()->tcp_option);

	uint16_t len=0, rb_free=0;
	uint16_t i=0;
    uint8_t sock_state=0;
	uint8_t ch=0;
    
    UART_TypeDef* UARTx = (channel==0)?UART0:UART1;
    
	if(serial_option[channel].flow_control == flow_rts_cts)
	{
#ifdef __USE_GPIO_HARDWARE_FLOWCONTROL__
		if(get_uart_cts_pin(channel) != UART_CTS_LOW) 
            return;
#else
        if((UARTx->FR &UART_FR_CTS )!=0)
            return;
#endif
	}

	len = getSn_RX_RSR(channel);
	
	if(len > UART_SRB_SIZE)
		len = UART_SRB_SIZE;
	
	rb_free = RingBuffer_GetFree(&txring[channel]);
	
	#if 1
    if(rb_free > 0)
	{	
		check_tx_rb_now_status[channel] = rb_free;
		if(check_tx_rb_old_status[channel] == check_tx_rb_now_status[channel])
		{
			check_tx_rb_status_cnt[channel]++;
			if(check_tx_rb_status_cnt[channel] >= 10)
			{
				UART_ITConfig(UARTx, UART_IT_FLAG_TXI, DISABLE);
				if(RingBuffer_Pop(&txring[channel], &ch))
				{
					while(UART_GetFlagStatus(UARTx, UART_FR_TXFF) == SET);
					UART_SendData(UARTx, ch);
				}
				UART_ITConfig(UARTx, UART_IT_FLAG_TXI, ENABLE);
			}
		}
		else
		{
			check_tx_rb_status_cnt[channel] = 0;
			check_tx_rb_old_status[channel] = check_tx_rb_now_status[channel];
		}
	}
	#endif
	
	if((len > 0) && (len <= rb_free)) 
	{
		switch(getSn_SR(channel))
		{
			case SOCK_UDP: // UDP_MODE\
				// 1014 이상이면 바로 overflow....len을 버퍼사이즈 이하로 넣어줘야 한다.
				//  전달하는 버퍼 크기보다 큰 데이터가 넘어오는 경우 백프로 오류..누군가가 큰 패킷 보내면 바로 죽음.
				// 최대값을 일단 DATA_BUF_SIZE로 한다.
				if(len > DATA_BUF_SIZE) len = DATA_BUF_SIZE;
			
                if (sock_remained_size[channel] == 0)
				// udp 가 2개 이상이면 framelength 도 소켓 수 만큼 있어야 한다.
				{
					uint16_t framesize = 0;
					e2u_size[channel] = recvfrom(channel, (uint8_t *)(&g_recv_buf[channel][2]), len, peerip, &peerport);
					framesize = (uint16_t)(sock_remained_size[channel] + e2u_size[channel]) & 0xffff;
					printf("frame_size[%d] : %d + %d = %d \r\n", channel, e2u_size[channel], sock_remained_size[channel], framesize);
					printf("data_size[%d] : %d \r\n", channel, e2u_size[channel] );
					g_recv_buf[channel][0] = (framesize & 0xff00) >> 8;
					g_recv_buf[channel][1] = (framesize & 0x00ff) ;
					e2u_size[channel] += 2; //for nuvoone, add header length
				}
				else
				{ // 프레임 헤더 없이 남은 데이터만 들어오는 경우, 이 곳에서 수행한다.
					e2u_size[channel] = recvfrom(channel, (uint8_t *)(g_recv_buf[channel]), len, peerip, &peerport);
					printf("data_size[%d] : %d \r\n", channel, e2u_size[channel] );
				}
			
				//printf("data_size[0] : %d \r\n", data_size[0]);
				//printf("data_size[1] : %d \r\n", data_size[1]);
				
				if(memcmp(peerip_tmp, peerip, 4) !=  0)
				{
					memcpy(peerip_tmp, peerip, 4);
					if(serial_common->serial_debug_en == ENABLE) 
                    {
                        printf(" > [%d]UDP Peer IP/Port: %d.%d.%d.%d : %d\r\n", channel, peerip[0], peerip[1], peerip[2], peerip[3], peerport);
                    }
				}
				break;
			case SOCK_ESTABLISHED: // TCP_SERVER_MODE, TCP_CLIENT_MODE, TCP_MIXED_MODE
			case SOCK_CLOSE_WAIT:
                e2u_size[channel] = recv(channel, g_recv_buf[channel], sizeof(g_recv_buf[channel]));
				break;
			default:
				break;
		}
		inactivity_time[channel] = 0;
		keepalive_time[channel] = 0;
		flag_sent_first_keepalive[channel] = RESET;
	}
	
	if((network_connection[channel].working_state == TCP_SERVER_MODE) 
        || ((network_connection[channel].working_state == TCP_MIXED_MODE) && (mixed_state[channel] == MIXED_SERVER)))
	{
		/* Connection password authentication */
		if((tcp_option[channel].pw_connect_en == ENABLE) && (flag_connect_pw_auth == RESET))
		{
            if(check_connect_pw_auth(g_recv_buf[channel], len, channel) == ENABLE)
				flag_connect_pw_auth[channel] = SET;
			else
				flag_connect_pw_auth[channel] = RESET;
			
			e2u_size[channel] = 0;
			
			if(flag_connect_pw_auth[channel] == RESET)
			{
				disconnect(channel);
				return;
			}
		}
	}
	
	if(e2u_size[channel] != 0)
	{
		if(serial_option[channel].dsr_en == ENABLE) // DTR / DSR handshake (flowcontrol)
		{
			if(get_flowcontrol_dsr_pin() == 0) 
            {
                return;
            }
		}
		if(serial_option[channel].uart_interface == UART_IF_RS422_485)
		{
			uart_rs485_enable(channel);
            UART_Send_RB(UARTx, &txring[channel], g_recv_buf[channel], e2u_size[channel]);
            
            for(i = 0; i < 65535; i++); //wait
            
			uart_rs485_disable(channel);
			
			e2u_size[channel] = 0;
		}
		else if(serial_option[channel].flow_control == flow_xon_xoff) 
		{
			if(isXON[channel] == ENABLE)
			{
                UART_Send_RB(UARTx, &txring[channel], g_recv_buf[channel], e2u_size[channel]);
				e2u_size[channel] = 0;
			}
		}
		else
		{
			
			UART_Send_RB(UARTx, &txring[channel], g_recv_buf[channel], e2u_size[channel]);
			e2u_size[channel] = 0;
		}
	}
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
uint16_t get_tcp_any_port(uint8_t channel)
{
	if(client_any_port[channel])
	{
		if(client_any_port[channel] < 0xffff) 	
			client_any_port[channel]++;
		else							
			client_any_port[channel] = 0;
	}
	
	if(client_any_port[channel] == 0)
	{
		srand(get_phylink_downtime());
		client_any_port[channel] = (rand() % 10000) + 35000; // 35000 ~ 44999
	}
	
	return client_any_port[channel];
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
void send_keepalive_packet_manual(uint8_t sock)
{
	setsockopt(sock, SO_KEEPALIVESEND, 0);
	//keepalive_time = 0;
#ifdef _SEG_DEBUG_
	printf(" > SOCKET[%x]: SEND KEEP-ALIVE PACKET\r\n", sock);
#endif 
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
uint8_t process_socket_termination(uint8_t channel)
{
	struct __network_connection *network_connection = (struct __network_connection *)get_DevConfig_pointer()->network_connection;
    
	uint8_t sock_status = getSn_SR(channel);
	
	if(sock_status == SOCK_CLOSED) 
		return channel;
	
	if(network_connection[channel].working_mode != UDP_MODE) // TCP_SERVER_MODE / TCP_CLIENT_MODE / TCP_MIXED_MODE
	{
		if((sock_status == SOCK_ESTABLISHED) || (sock_status == SOCK_CLOSE_WAIT))
			disconnect(channel);
	}
	
	close(channel);
	
	return channel;
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
uint8_t check_connect_pw_auth(uint8_t * buf, uint16_t len, uint8_t channel)
{
	struct __tcp_option *tcp_option = (struct __tcp_option *)(get_DevConfig_pointer()->tcp_option);
	uint8_t ret = DISABLE;
	uint8_t pwbuf[11] = {0,};
	
	if(len >= sizeof(pwbuf)) 
    {
        len = sizeof(pwbuf) - 1;
    }
	
	memcpy(pwbuf, buf, len);
	
	if((len == strlen(tcp_option[channel].pw_connect)) && (memcmp(tcp_option[channel].pw_connect, pwbuf, len) == 0))
		ret = ENABLE; // Connection password auth success
	
#ifdef _SEG_DEBUG_
	printf(" > Connection password: %s, len: %d\r\n", tcp_option[channel].pw_connect, strlen(tcp_option[channel].pw_connect));
	printf(" > Entered password: %s, len: %d\r\n", pwbuf, len);
	printf(" >> Auth %s\r\n", ret ? "success":"failed");
#endif
	
	return ret;
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
void init_trigger_modeswitch(uint8_t mode)
{
	struct __serial_common *serial_common = (struct __serial_common *)&(get_DevConfig_pointer()->serial_common);
	struct __network_connection *network_connection = (struct __network_connection *)(get_DevConfig_pointer()->network_connection);
	
	uint8_t i;
	
	if(mode == DEVICE_AT_MODE)
	{
		opmode = DEVICE_AT_MODE;
		for(i=0; i<DEVICE_UART_CNT; i++)
		{
			set_device_status(i, ST_ATMODE);
		}
		
		if(serial_common->serial_debug_en == ENABLE)
		{
			printf(" > SEG:AT Mode\r\n");
            //UART_Send_RB(UART0, &txring[0], (uint8_t *)"SEG:AT Mode\r\n", sizeof("SEG:AT Mode\r\n"));
			UartPuts(UART0,(uint8_t *)"SEG:AT Mode\r\n");
		}
#if (DEVICE_BOARD_NAME == WIZ752SR_12x)
        LED_On(LED2);
#endif
	}
	else // DEVICE_GW_MODE
	{
		opmode = DEVICE_GW_MODE;
		for(i=0; i<DEVICE_UART_CNT; i++)
		{
			set_device_status(i, ST_OPEN);
			if(network_connection[i].working_mode == TCP_MIXED_MODE) 
					mixed_state[i] = MIXED_SERVER;
		}	
		if(serial_common->serial_debug_en)
		{
			printf(" > SEG:GW Mode\r\n");
            //UART_Send_RB(UART0, &txring[0], (uint8_t *)"SEG:GW Mode\r\n", sizeof("SEG:GW Mode\r\n"));
			UartPuts(UART0,(uint8_t *)"SEG:GW Mode\r\n");
		}
#if (DEVICE_BOARD_NAME == WIZ752SR_12x)
        LED_Off(LED2);
#endif
	}
	
	for(i=0; i<DEVICE_UART_CNT; i++)
	{
		u2e_size[i] = 0;

		enable_inactivity_timer[i] = DISABLE;
		enable_keepalive_timer[i] = DISABLE;
		enable_serial_input_timer[i] = DISABLE;
		
		
		inactivity_time[i] = 0;
		keepalive_time[i] = 0;
		serial_input_time[i] = 0;
		flag_serial_input_time_elapse[i] = RESET;
        
		UART_Buffer_Flush(&txring[i]);
		UART_Buffer_Flush(&rxring[i]);
	}
	enable_modeswitch_timer = DISABLE;
	modeswitch_time = 0;
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
uint8_t check_modeswitch_trigger(uint8_t ch)
{
	struct __serial_command *serial_command = (struct __serial_command *)&(get_DevConfig_pointer()->serial_command);
	
	uint8_t modeswitch_failed = DISABLE;
	uint8_t ret = 0;
	
	if(opmode != DEVICE_GW_MODE) 				
		return 0;
	if(serial_command->serial_command == DISABLE) 	
		return 0;
	
	switch(triggercode_idx)
	{
		case 0:
			if((ch == serial_command->serial_trigger[triggercode_idx]) && (modeswitch_time == modeswitch_gap_time)) // comparision succeed
			{
				ch_tmp[triggercode_idx] = ch;
				triggercode_idx++;
				enable_modeswitch_timer = ENABLE;
			}
			break;
		case 1:
		case 2:
			if((ch == serial_command->serial_trigger[triggercode_idx]) && (modeswitch_time < modeswitch_gap_time)) // comparision succeed
			{
				ch_tmp[triggercode_idx] = ch;
				triggercode_idx++;
			}
			else // comparision failed: invalid trigger code
			{
				modeswitch_failed = ENABLE; 
			}
			break;
		case 3:
			if(modeswitch_time < modeswitch_gap_time) // comparision failed: end gap
			{
				modeswitch_failed = ENABLE;
			}
			break;
	}
	
	if(modeswitch_failed == ENABLE)
	{
		restore_serial_data(triggercode_idx);
	}
	
	modeswitch_time = 0; // reset the inter gap time count for each trigger code recognition (Allowable interval)
	ret = triggercode_idx;
	
	return ret;
}

/**
  * @brief  when serial command mode trigger code comparision failed
  * @param  None
  * @retval None
  */
void restore_serial_data(uint8_t idx)
{
	uint8_t i;
	
	for(i = 0; i < idx; i++)
	{
        RingBuffer_Insert(&rxring[0], &ch_tmp[i]);
        ch_tmp[i] = 0x00;
	}
	
	enable_modeswitch_timer = DISABLE;
	triggercode_idx = 0;
}

uint8_t check_serial_store_permitted(uint8_t channel, uint8_t ch)
{
	struct __network_connection *network_connection = (struct __network_connection *)get_DevConfig_pointer()->network_connection;
    struct __serial_option *serial_option = (struct __serial_option *)get_DevConfig_pointer()->serial_option;

	uint8_t ret = DISABLE; // DISABLE: Doesn't put the serial data in a ring buffer
	switch(network_connection[channel].working_state)
	{
		case ST_OPEN:
			if(network_connection[channel].working_mode != TCP_MIXED_MODE) break;
		case ST_CONNECT:
		case ST_UDP:
		case ST_ATMODE:
			ret = ENABLE;
			break;
		default:
			break;
	}
	
	/** 
	  * Software flow control: Check the XON/XOFF start/stop commands
	  * [Peer] -> [WIZnet Device]
	  */
	
	if((ret == ENABLE) && (serial_option[channel].flow_control == flow_xon_xoff))
	{
		if(ch == UART_XON)
		{
			isXON[channel] = ENABLE;
			ret = DISABLE; 
		}
		else if(ch == UART_XOFF)
		{
			isXON[channel] = DISABLE;
			ret = DISABLE;
		}
	}
	return ret;
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
void reset_SEG_timeflags(uint8_t channel)
{
    /* Timer disable */
    enable_inactivity_timer[channel] = DISABLE;
    enable_serial_input_timer[channel] = DISABLE;
    enable_keepalive_timer[channel] = DISABLE;
    enable_connection_auth_timer[channel] = DISABLE;
    
    /* Flag clear */
    flag_serial_input_time_elapse[channel] = RESET;
    flag_sent_keepalive[channel] = RESET;
    //flag_sent_keepalive_wait = RESET;
    flag_connect_pw_auth[channel] = RESET; // TCP_SERVER_MODE only (+ MIXED_SERVER)
    
    /* Timer value clear */
    inactivity_time[channel] = 0;
    serial_input_time[channel] = 0;
    keepalive_time[channel] = 0;
    connection_auth_time[channel] = 0;	
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
void init_time_delimiter_timer(uint8_t channel)
{
	struct __serial_command *serial_command = (struct __serial_command *)&(get_DevConfig_pointer()->serial_command);
    struct __serial_data_packing *serial_data_packing = (struct __serial_data_packing *)(get_DevConfig_pointer()->serial_data_packing);
	
	if((serial_command->serial_command == ENABLE) && (opmode == DEVICE_GW_MODE))
	{
		if(serial_data_packing[channel].packing_time != 0)
		{
			if(enable_serial_input_timer[channel] == DISABLE) 
            {
                enable_serial_input_timer[channel] = ENABLE;
            }
			serial_input_time[channel] = 0;
		}
	}
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
uint8_t check_tcp_connect_exception(uint8_t channel)
{
    struct __network_option *network_option = (struct __network_option *)&get_DevConfig_pointer()->network_option;
    struct __serial_common *serial_common = (struct __serial_common *)&get_DevConfig_pointer()->serial_common;
    struct __network_connection *network_connection = (struct __network_connection *)get_DevConfig_pointer()->network_connection;
	
	uint8_t srcip[4] = {0, };
	uint8_t ret = SUCCESS;
	
	getSIPR(srcip);
	
	/* DNS failed */
	if((network_option->dns_use == ENABLE) && (flag_process_dns_success == RESET))
	{
		if(serial_common->serial_debug_en == ENABLE) 
        {
            printf(" > SEG:CONNECTION FAILED - DNS Failed\r\n");
        }
		ret = ERROR;
	}	
	/* if dhcp failed (0.0.0.0), this case do not connect to peer */
	else if((srcip[0] == 0x00) 
                && (srcip[1] == 0x00) 
                && (srcip[2] == 0x00) 
                && (srcip[3] == 0x00))
	{
		if(serial_common->serial_debug_en == ENABLE) 
        {
            printf(" > SEG:CONNECTION FAILED - Invalid IP address: Zero IP\r\n");
        }
		ret = ERROR;
	}
	/* Destination zero IP */
	else if((network_connection[channel].remote_ip[0] == 0x00) 
                && (network_connection[channel].remote_ip[1] == 0x00) 
                && (network_connection[channel].remote_ip[2] == 0x00) 
                && (network_connection[channel].remote_ip[3] == 0x00))
	{
		if(serial_common->serial_debug_en == ENABLE) 
            printf(" > SEG:CONNECTION FAILED - Invalid Destination IP address: Zero IP\r\n");
		ret = ERROR;
	}
	/* Duplicate IP address */
	else if((srcip[0] == network_connection[channel].remote_ip[0]) 
                && (srcip[1] == network_connection[channel].remote_ip[1]) 
                && (srcip[2] == network_connection[channel].remote_ip[2]) 
                && (srcip[3] == network_connection[channel].remote_ip[3]))
	{
		if(serial_common->serial_debug_en == ENABLE) 
            printf(" > SEG:CONNECTION FAILED - Duplicate IP address\r\n");
		ret = ERROR;
	}
	else if((srcip[0] == 192) && (srcip[1] == 168)) // local IP address == Class C private IP
	{
		/* Static IP address obtained */
		if((network_option->dhcp_use == DISABLE) 
            && ((network_connection[channel].remote_ip[0] == 192) && (network_connection[channel].remote_ip[1] == 168)))
		{
			if(srcip[2] != network_connection[channel].remote_ip[2]) // Class C Private IP network mismatch
			{
				if(serial_common->serial_debug_en == ENABLE) 
                    printf(" > SEG:CONNECTION FAILED - Invalid IP address range (%d.%d.[%d].%d)\r\n", network_connection[0].remote_ip[0], network_connection[0].remote_ip[1], network_connection[0].remote_ip[2], network_connection[0].remote_ip[3]);
				ret = ERROR; 
			}
		}
	}
	
	return ret;
}

/**
  * @brief  This function have to call every 1 millisecond by Timer IRQ handler routine.
  * @param  None
  * @retval None
  */
void seg_timer_msec(void)
{
	struct __serial_data_packing *serial_data_packing = (struct __serial_data_packing *)(get_DevConfig_pointer()->serial_data_packing);
	uint8_t i;
	
	/** 
	* Firmware update timer for timeout
	* DHCP timer for timeout
	* SEGCP Keep-alive timer (for configuration tool, TCP mode)
	* Reconnection timer: Time count routine (msec)
	*/
	for(i=0; i<DEVICE_UART_CNT; i++)
	{
		if(enable_reconnection_timer[i])
		{
			if(reconnection_time[i] < 0xFFFF) 	
				reconnection_time[i]++;
			else 							
				reconnection_time[i] = 0;
		}
		
		/* Keep-alive timer: Time count routine (msec) */
		if(enable_keepalive_timer[i])
		{
			if(keepalive_time[i] < 0xFFFF) 	
				keepalive_time[i]++;
			else							
				keepalive_time[i] = 0;
		}
	}
	
	/* switch timer: Time count routine (msec) (GW mode <-> Serial command mode, for s/w mode switch trigger code) */
	if(modeswitch_time < modeswitch_gap_time) 
        modeswitch_time++;
	
	if((enable_modeswitch_timer) && (modeswitch_time == modeswitch_gap_time))
	{
		/* result of command mode trigger code comparision */
		if(triggercode_idx == 3) 	
            sw_modeswitch_at_mode_on = ENABLE; 
		else						
            restore_serial_data(triggercode_idx);	
		
		triggercode_idx = 0;
		enable_modeswitch_timer = DISABLE;
	}
	
	/* Serial data packing time delimiter timer */
	for(i=0; i<DEVICE_UART_CNT; i++)
	{
		if(enable_serial_input_timer[i])
		{
			if(serial_input_time[i] < serial_data_packing[i].packing_time)
				serial_input_time[i]++;
			else
			{
				serial_input_time[i] = 0;
				enable_serial_input_timer[i] = 0;
				flag_serial_input_time_elapse[i] = SET;
			}
		}
		
		/* Connection password auth timer */
		if(enable_connection_auth_timer[i])
		{
			if(connection_auth_time[i] < 0xffff) 	
                connection_auth_time[i]++;
			else								
                connection_auth_time[i] = 0;
		}
	}
}

/**
  * @brief  This function have to call every 1 second by Timer IRQ handler routine.
  * @param  None
  * @retval None
  */
void seg_timer_sec(void)
{
    uint8_t i;

    for(i=0; i<DEVICE_UART_CNT; i++)
    {
        /* Inactivity timer: Time count routine (sec) */
        if(enable_inactivity_timer[i] == ENABLE)
		{
            if(inactivity_time[i] < 0xFFFF) 
                inactivity_time[i]++;
		}
    }
    tmp_timeflag_for_debug = SET;
}

/**
  * @brief  None
  * @param  None
  * @retval None
  */
void check_n_clear_uart_recv_status(uint8_t channel)
{
	uint16_t dummy;
	
	UART_TypeDef* UARTx = (channel==0)?UART0:UART1;
	
	if(UARTx->STATUS.RSR != RESET)
	{
		if(UART_GetRecvStatus(UARTx, UART_RECV_STATUS_OE))
			dummy = UART_ReceiveData(UARTx);
		
		UARTx->STATUS.ECR = ~UARTx->STATUS.RSR;
	}
}
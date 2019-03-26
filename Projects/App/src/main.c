/**
  ******************************************************************************
  * @file    W7500x Serial to Ethernet Project - WIZ750SR App
  * @author  Eric Jung, Team Platform
  * @version v1.1.0
  * @date    Nov-2016
  * @brief   Main program body
  ******************************************************************************
  * @attention
  * @par Revision history
  *    <2016/11/18> v1.1.0 Develop by Eric Jung
  *    <2016/03/29> v1.0.0 Develop by Eric Jung
  *    <2016/03/02> v0.8.0 Develop by Eric Jung
  *    <2015/11/24> v0.0.1 Develop by Eric Jung
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, WIZnet SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2017 WIZnet Co., Ltd.</center></h2>
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "common.h"

#include "W7500x_board.h"
#include "W7500x_crg.h"
#include "W7500x_gpio.h"
#include "W7500x_miim.h"
#include "W7500x_uart.h"
#include "W7500x_wztoe.h"

#include "dhcp.h"
#include "dhcp_cb.h"
#include "dns.h"

#include "seg.h"
#include "segcp.h"
#include "configData.h"

#include "timerHandler.h"
#include "uartHandler.h"
#include "deviceHandler.h"
#include "flashHandler.h"
#include "gpioHandler.h"

/* Private typedef -----------------------------------------------------------*/
extern RINGBUFF_T txring[DEVICE_UART_CNT];
extern RINGBUFF_T rxring[DEVICE_UART_CNT];
/* Private define ------------------------------------------------------------*/
//#define _MAIN_DEBUG_	// debugging message enable
/* Private function prototypes -----------------------------------------------*/
static void W7500x_Init(void);
static void W7500x_WZTOE_Init(void);
static int8_t process_dhcp(void);
static int8_t process_dns(void);
static void display_Dev_Info_header(void);
static void display_Dev_Info_main(void);
static void display_Dev_Info_dhcp(void);
static void display_Dev_Info_dns(void);
void delay(__IO uint32_t milliseconds); //Notice: used ioLibray
void TimingDelay_Decrement(void);
/* Private variables ---------------------------------------------------------*/
static __IO uint32_t TimingDelay;
/* Public variables ----------------------------------------------------------*/
uint8_t g_send_buf[DEVICE_UART_CNT][DATA_BUF_SIZE];
uint8_t g_recv_buf[DEVICE_UART_CNT][DATA_BUF_SIZE];

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
    uint8_t i;
    uint32_t time;
	
	DevConfig *dev_config = get_DevConfig_pointer();

	W7500x_Init();					// W7500x MCU Initialization
	W7500x_WZTOE_Init();			// W7500x WZTOE (Hardwired TCP/IP stack) Initialization 
	W7500x_Board_Init();			// W7500x Board Initialization 
	Mac_Conf();						// Set the MAC address to WIZCHIP 
	IO_Configuration();				// GPIO Initialization 
	load_DevConfig_from_storage();	// Load the Configuration data 
	
	for(i=0; i<DEVICE_UART_CNT; i++)
	{
		dev_config->network_connection[i].working_state = ST_OPEN;
		S2E_UART_Configuration(i);	// UART Initialization 
	}

	/* Debug UART: Device information print out */
	if(dev_config->serial_common.serial_debug_en == ENABLE)
	{
		display_Dev_Info_header();
		display_Dev_Info_main();
	}
	
    printf(" - PHY Link status: %x\r\n", get_phylink());
    printf("%s\r\n", STR_BAR);

	/* Initialize Network Information: DHCP or Static IP allocation */
	if(dev_config->network_option.dhcp_use == ENABLE)
	{
		if(process_dhcp() == DHCP_IP_LEASED) 
			flag_process_dhcp_success = SET;
		else 			// DHCP failed 
			Net_Conf(); // Set default static IP settings
	}
	else
	{
		Net_Conf(); 	// Set default static IP settings
	}
	
	/* Debug UART: Network information print out (includes DHCP IP allocation result) */
	if(dev_config->serial_common.serial_debug_en == ENABLE)
	{
		display_Net_Info();
		display_Dev_Info_dhcp();
	}
	
	/* DNS client */
	if(dev_config->network_connection[0].working_mode != TCP_SERVER_MODE)
	{
		if(dev_config->network_option.dns_use == ENABLE) 
		{
			if(process_dns() == SUCCESS)
				flag_process_dns_success = SET;
			
			/* Debug UART: DNS results print out */
			if(dev_config->serial_common.serial_debug_en == ENABLE)
				display_Dev_Info_dns();
		}
	}
	
	/* W7500x Application: Main Routine */
	flag_s2e_application_running = SET;
	
	/* HW_TRIG switch ON */
	if(flag_hw_trig_enable == SET)
	{
		init_trigger_modeswitch(DEVICE_AT_MODE);
		flag_hw_trig_enable = RESET;
	}
	
	printf("%s\r\n", STR_BAR);

	while(1) 
	{
		do_segcp();
		
		do_seg();
		
		if(dev_config->network_option.dhcp_use == ENABLE) 
			DHCP_run(); // DHCP client handler for IP renewal
		
		if(flag_check_phylink == SET)
		{
			// ## debugging: PHY link
			//printf("PHY Link status: %x\r\n", get_phylink());
            flag_check_phylink = RESET;	
		}
		
		for(i=0; i<DEVICE_UART_CNT; i++)
		{
			if(flag_ringbuf_full[i] == SET)
			{
				if(dev_config->serial_common.serial_debug_en == ENABLE) 
					printf(" > CHANNEL[%d]Rx Ring buffer Full\r\n", i);
				flag_ringbuf_full[i] = RESET;
			}
		}
	} 
} 

/**
  * @brief  W7500x Initialize
  * @param  None
  * @retval None
  */
static void W7500x_Init(void)
{
	/* W7500x MCU Initialize------------------------------------------------------*/
	/* Reset supervisory IC Init */
	Supervisory_IC_Init();
    
	SystemInit_User(DEVICE_CLOCK_SELECT, DEVICE_PLL_SOURCE_CLOCK, DEVICE_TARGET_SYSTEM_CLOCK);

	/* DualTimer0 Initialization */
	Timer0_Configuration();
	
	/* Simple UART init for Debugging */
	UART2_Configuration();
	
	/* SysTick_Config */
	SysTick_Config((GetSystemClock()/1000));
	delay(10);

#ifdef _MAIN_DEBUG_
	printf("\r\n >> W7500x MCU Clock Settings ===============\r\n"); 
	printf(" - GetPLLSource: %s, %lu (Hz)\r\n", GetPLLSource()?"External":"Internal", DEVICE_PLL_SOURCE_CLOCK);
	printf("\t+ CRG->PLL_FCR: 0x%.8x\r\n", CRG->PLL_FCR);
	printf(" - SetSystemClock: %lu (Hz) \r\n", DEVICE_TARGET_SYSTEM_CLOCK);
	printf(" - GetSystemClock: %d (Hz) \r\n", GetSystemClock());
#endif
}

/**
  * @brief  W7500x WZTOE (Hardwired TCP/IP core) Initialize
  * @param  None
  * @retval None
  */
static void W7500x_WZTOE_Init(void)
{
	/* Set Network Configuration: HW Socket Tx/Rx buffer size */
	uint8_t tx_size[8] = { 2, 2, 2, 2, 2, 2, 0, 0 }; // default: { 2, 2, 2, 2, 2, 2, 2, 2 }
	uint8_t rx_size[8] = { 2, 2, 2, 2, 2, 2, 0, 0 }; // default: { 2, 2, 2, 2, 2, 2, 2, 2 }
	
#ifdef _MAIN_DEBUG_
	uint8_t i;
#endif
	
	/* Structure for timeout control: RTR, RCR */
	wiz_NetTimeout * net_timeout;
	
	/* Set WZ_100US Register */
	setTIC100US((GetSystemClock()/10000));
#ifdef _MAIN_DEBUG_
	//printf(" GetSystemClock: %X, getTIC100US: %X, (%X) \r\n", GetSystemClock(), getTIC100US(), *(uint32_t *)WZTOE_TIC100US); // for debugging
	printf("\r\n >> WZTOE Settings ==========================\r\n");
	printf(" - getTIC100US: %X, (%X) \r\n", getTIC100US(), *(uint32_t *)WZTOE_TIC100US); // for debugging
#endif
	
	/* Set TCP Timeout: retry count / timeout val */
	net_timeout->retry_cnt = 8;
	net_timeout->time_100us = 2500;
	
	wizchip_settimeout(net_timeout);
    
#ifdef _MAIN_DEBUG_
	wizchip_gettimeout(net_timeout); // TCP timeout settings
	printf(" Network Timeout Settings - RTR: %d, RCR: %d\r\n", net_timeout->retry_cnt, net_timeout->time_100us);
#endif
	
	/* Set Network Configuration */
	wizchip_init(tx_size, rx_size);
	
#ifdef _MAIN_DEBUG_
	printf(" - WZTOE H/W Socket Buffer Settings (kB)\r\n");
	printf("   [Tx] ");
	for(i = 0; i < _WIZCHIP_SOCK_NUM_; i++) 
		printf("%d ", getSn_TXBUF_SIZE(i));
	printf("\r\n");
	printf("   [Rx] ");
	for(i = 0; i < _WIZCHIP_SOCK_NUM_; i++) 
		printf("%d ", getSn_RXBUF_SIZE(i));
	printf("\r\n");
#endif
}

/**
  * @brief  Process DHCP
  * @param  None
  * @retval The result of process DHCP (SUCCESS or ERROR).
  */
int8_t process_dhcp(void)
{
	uint8_t ret = 0;
	uint8_t dhcp_retry = 0;
    uint8_t i;

#ifdef _MAIN_DEBUG_
	printf(" - DHCP Client running\r\n");
#endif
    
    DHCP_init(SOCK_DHCP, g_send_buf[0]);

	reg_dhcp_cbfunc(w7500x_dhcp_assign, w7500x_dhcp_assign, w7500x_dhcp_conflict);
	
    for(i=0; i<DEVICE_UART_CNT; i++)
        set_device_status(i, ST_UPGRADE);
	
	while(1)
	{
		ret = DHCP_run();
		
		if(ret == DHCP_IP_LEASED)
		{
#ifdef _MAIN_DEBUG_
			printf(" - DHCP Success\r\n");
#endif
			break;
		}
		else if(ret == DHCP_FAILED)
		{
			dhcp_retry++;
#ifdef _MAIN_DEBUG_
			if(dhcp_retry <= 3) 
				printf(" - DHCP Timeout occurred and retry [%d]\r\n", dhcp_retry);
#endif
		}

		if(dhcp_retry > 3)
		{
#ifdef _MAIN_DEBUG_
			printf(" - DHCP Failed\r\n\r\n");
#endif
			DHCP_stop();
			break;
		}

		do_segcp(); // Process the requests of configuration tool during the DHCP client run.
	}
	
    for(i=0; i<DEVICE_UART_CNT; i++)
        set_device_status(i, ST_OPEN);
	
	return ret;
}

/**
  * @brief  Process DNS
  * @param  None
  * @retval The result of process DNS (SUCCESS or ERROR).
  */
int8_t process_dns(void)
{
	DevConfig *dev_config = get_DevConfig_pointer();
	int8_t ret = 0;
	uint8_t dns_retry = 0;
	uint8_t dns_server_ip[4];
    uint8_t i;
	
#ifdef _MAIN_DEBUG_
	printf(" - DNS Client running\r\n");
#endif

    DNS_init(SOCK_DNS, g_send_buf[0]);
	
	dns_server_ip[0] = dev_config->network_option.dns_server_ip[0];
	dns_server_ip[1] = dev_config->network_option.dns_server_ip[1];
	dns_server_ip[2] = dev_config->network_option.dns_server_ip[2];
	dns_server_ip[3] = dev_config->network_option.dns_server_ip[3];
	
    for(i=0; i<DEVICE_UART_CNT; i++)
        set_device_status(i, ST_UPGRADE);
	
	while(1) 
	{
		if((ret = DNS_run(dns_server_ip, (uint8_t *)dev_config->network_option.dns_domain_name, dev_config->network_connection[0].remote_ip)) == 1)
		{
#ifdef _MAIN_DEBUG_
			printf(" - DNS Success\r\n");
#endif
			break;
		}
		else
		{
			dns_retry++;
#ifdef _MAIN_DEBUG_
			if(dns_retry <= 2) 
				printf(" - DNS Timeout occurred and retry [%d]\r\n", dns_retry);
#endif
		}

		if(dns_retry > 2) 
		{
#ifdef _MAIN_DEBUG_
			printf(" - DNS Failed\r\n\r\n");
#endif
			break;
		}

		do_segcp(); // Process the requests of configuration tool during the DNS client run.

		if(dev_config->network_option.dhcp_use == ENABLE) 
			DHCP_run();
	}
	
    for(i=0; i<DEVICE_UART_CNT; i++)
        set_device_status(i, ST_OPEN);
	
	return ret;
}

/**
  * @brief  Display device informations
  * @param  None
  * @retval None
  */
void display_Dev_Info_header(void)
{
	DevConfig *dev_config = get_DevConfig_pointer();

	printf("\r\n");
	printf("%s\r\n", STR_BAR);
	
#if ((DEVICE_BOARD_NAME == WIZ750SR) || (DEVICE_BOARD_NAME == W7500P_S2E) || (DEVICE_BOARD_NAME == WIZ750SR_1xx) || (DEVICE_BOARD_NAME == WIZ752SR_12x))
	printf(" %s[%d]Channel \r\n", DEVICE_ID_DEFAULT, DEVICE_UART_CNT);
	printf(" >> WIZnet Serial to Ethernet Device\r\n");
#else
#ifndef __W7500P__
		printf(" [W7500] Serial to Ethernet Device\r\n");
#else
		printf(" [W7500P] Serial to Ethernet Device\r\n");
#endif
#endif
	
	printf(" >> Firmware version: %d.%d.%d %s\r\n", dev_config->device_common.fw_ver[0], dev_config->device_common.fw_ver[1], dev_config->device_common.fw_ver[2], STR_VERSION_STATUS);
	printf("%s\r\n", STR_BAR);
}

/**
  * @brief  Display device informations
  * @param  None
  * @retval None
  */
void display_Dev_Info_main(void)
{
	DevConfig *dev_config = get_DevConfig_pointer();
    
    uint8_t i;
	uint32_t baud_table[] = {300, 600, 1200, 1800, 2400, 4800, 9600, 14400, 19200, 28800, 38400, 57600, 115200, 230400};
	    
	printf(" - Device name: %s\r\n", dev_config->device_common.module_name);
	printf(" - Network settings: \r\n");
		printf("\t- Obtaining IP settings: [%s]\r\n", (dev_config->network_option.dhcp_use == 1)?"Automatic - DHCP":"Static");
		
    printf(" - Search ID code: \r\n");
		printf("\t- %s: [%s]\r\n", (dev_config->config_common.pw_search[0] != 0)?"Enabled":"Disabled", (dev_config->config_common.pw_search[0] != 0)?dev_config->config_common.pw_search:"None");
	
    printf(" - TCP/UDP setting port: [%d]\r\n", DEVICE_SEGCP_PORT);
	printf(" - Firmware update port: [%d]\r\n", DEVICE_FWUP_PORT);
	
    printf("%s\r\n", STR_BAR);
    
    for(i=0; i<DEVICE_UART_CNT; i++)
    {
    printf(" - [%d] Channel mode: %s\r\n", i, str_working[dev_config->network_connection[i].working_mode]);
    
        printf("\t- TCP/UDP ports\r\n");
   
		printf("\t   + S2E data port: [%d]\r\n", dev_config->network_connection[i].local_port);
   
	printf(" - [%d] Channel Ethernet connection password: \r\n", i);
		printf("\t- %s %s\r\n", (dev_config->tcp_option[i].pw_connect_en == 1)?"Enabled":"Disabled", "(TCP server / mixed mode only)");
    
	printf(" - [%d] Channel Connection timer settings: \r\n", i);

		printf("\t- Inactivity timer: ");
			if(dev_config->tcp_option[i].inactivity) printf("[%d] (sec)\r\n", dev_config->tcp_option[i].inactivity);
			else printf("%s\r\n", STR_DISABLED);
    
		printf("\t- Reconnect interval: ");
			if(dev_config->tcp_option[i].reconnection) printf("[%d] (msec)\r\n", dev_config->tcp_option[i].reconnection);
			else printf("%s\r\n", STR_DISABLED);
    
	printf(" - [%d] Channel Serial settings: \r\n", i);

		printf("\t- Data %s port:  [%s%d]\r\n", STR_UART, STR_UART, i);
		printf("\t   + UART IF: [%s]\r\n", uart_if_table[dev_config->serial_option[i].uart_interface]);
		printf("\t   + %d-", baud_table[dev_config->serial_option[i].baud_rate]);
		printf("%d-", word_len_table[dev_config->serial_option[i].data_bits]);
		printf("%s-", parity_table[dev_config->serial_option[i].parity]);
		printf("%d / ", stop_bit_table[dev_config->serial_option[i].stop_bits]);
		if(dev_config->serial_option[i].uart_interface == UART_IF_RS232_TTL)
			printf("Flow control: %s\r\n", flow_ctrl_table[dev_config->serial_option[i].flow_control]);
		else
			printf("Flow control: %s\r\n", flow_ctrl_table[0]); // RS-422/485; flow control - NONE only

	printf(" - [%d] Channel Serial data packing options:\r\n", i);
		printf("\t- Time: ");
			if(dev_config->serial_data_packing[i].packing_time) printf("[%d] (msec)\r\n", dev_config->serial_data_packing[i].packing_time);
			else printf("%s\r\n", STR_DISABLED);
		printf("\t- Size: ");
			if(dev_config->serial_data_packing[i].packing_size) printf("[%d] (bytes)\r\n", dev_config->serial_data_packing[i].packing_size);
			else printf("%s\r\n", STR_DISABLED);
		printf("\t- Char: ");
			if(dev_config->serial_data_packing[i].packing_delimiter_length == 1) printf("[%.2X] (hex only)\r\n", dev_config->serial_data_packing[i].packing_delimiter[0]);
			else printf("%s\r\n", STR_DISABLED);
        
    printf("%s\r\n", STR_BAR);
        
    }

    printf(" - Debug %s port: [%s%d]\r\n", STR_UART, STR_UART, SEG_DEBUG_UART);
		printf("   + %s / %s %s\r\n", "115200-8-N-1", "NONE", "(fixed)");
    
    printf("%s\r\n", STR_BAR);
    
		printf(" - Serial command mode swtich code:\r\n");
		printf("\t- %s\r\n", (dev_config->serial_command.serial_command == 1)?STR_ENABLED:STR_DISABLED);
		printf("\t- [%.2X][%.2X][%.2X] (Hex only)\r\n", dev_config->serial_command.serial_trigger[0], dev_config->serial_command.serial_trigger[1], dev_config->serial_command.serial_trigger[2]);
	
#if ((DEVICE_BOARD_NAME == WIZ750SR) || (DEVICE_BOARD_NAME == W7500P_S2E) || (DEVICE_BOARD_NAME == WIZ750SR_1xx))
	printf(" - Hardware information: Status pins\r\n");
		printf("\t- Status 1: [%s] - %s\r\n", "PA_10", dev_config->serial_option[0].dtr_en?"DTR":"PHY link");
		printf("\t- Status 2: [%s] - %s\r\n", "PA_01", dev_config->serial_option[0].dsr_en?"DSR":"TCP connection"); // shared pin; HW_TRIG (input) / TCP connection indicator (output)

        printf("\t- Status 1: [%s] - %s\r\n", "PA_10", dev_config->serial_option[1].dtr_en?"DTR":"PHY link");
		printf("\t- Status 2: [%s] - %s\r\n", "PA_01", dev_config->serial_option[1].dsr_en?"DSR":"TCP connection"); // shared pin; HW_TRIG (input) / TCP connection indicator (output)

#ifdef __USE_USERS_GPIO__
	printf(" - Hardware information: User I/O pins\r\n");
		printf("\t- UserIO A: [%s] - %s / %s\r\n", "PC_13", USER_IO_TYPE_STR[get_user_io_type(USER_IO_SEL[0])], USER_IO_DIR_STR[get_user_io_direction(USER_IO_SEL[0])]); 
		printf("\t- UserIO B: [%s] - %s / %s\r\n", "PC_12", USER_IO_TYPE_STR[get_user_io_type(USER_IO_SEL[1])], USER_IO_DIR_STR[get_user_io_direction(USER_IO_SEL[1])]); 
		printf("\t- UserIO C: [%s] - %s / %s\r\n", "PC_09", USER_IO_TYPE_STR[get_user_io_type(USER_IO_SEL[2])], USER_IO_DIR_STR[get_user_io_direction(USER_IO_SEL[2])]); 
		printf("\t- UserIO D: [%s] - %s / %s\r\n", "PC_08", USER_IO_TYPE_STR[get_user_io_type(USER_IO_SEL[3])], USER_IO_DIR_STR[get_user_io_direction(USER_IO_SEL[3])]); 
#endif

#endif
	printf("%s\r\n", STR_BAR);
}

/**
  * @brief  Display DHCP informations
  * @param  None
  * @retval None
  */
void display_Dev_Info_dhcp(void)
{
	DevConfig *dev_config = get_DevConfig_pointer();
	
	if(dev_config->network_option.dhcp_use == ENABLE) 
	{
		if(flag_process_dhcp_success == SET) 
			printf(" # DHCP IP Leased time : %u seconds\r\n", getDHCPLeasetime());
		else 
			printf(" # DHCP Failed\r\n");
	}
}

/**
  * @brief  Display DNS informations
  * @param  None
  * @retval None
  */
void display_Dev_Info_dns(void)
{
	DevConfig *dev_config = get_DevConfig_pointer();
	
	if(dev_config->network_option.dns_use == ENABLE) 
	{
		if(flag_process_dns_success == SET)
		{
			printf(" # DNS: %s => %d.%d.%d.%d : %d\r\n", dev_config->network_option.dns_domain_name, 
														dev_config->network_connection[0].remote_ip[0],
														dev_config->network_connection[0].remote_ip[1],
														dev_config->network_connection[0].remote_ip[2],
														dev_config->network_connection[0].remote_ip[3],
														dev_config->network_connection[0].remote_port);
		}
		else
		{			
			printf(" # DNS Failed\r\n");
		}
	}
}

/**
  * @brief  Inserts a delay time.
  * @param  - milliseconds: specifies the delay time length, in milliseconds.
  * @retval None
  */
void delay(__IO uint32_t milliseconds)
{
	TimingDelay = milliseconds;
	while(TimingDelay != 0);
}

/**
  * @brief  Decrements the TimingDelay variable.
  * @param  None
  * @retval None
  */
void TimingDelay_Decrement(void)
{
	if(TimingDelay != 0x00)
		TimingDelay--;
}


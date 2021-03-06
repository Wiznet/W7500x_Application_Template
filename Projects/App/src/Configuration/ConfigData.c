/*
 * ConfigData.c 
 */

#include <stdio.h>
#include <string.h>

#include "W7500x_wztoe.h"
#include "W7500x_board.h"

#include "common.h"
#include "ConfigData.h"
#include "storageHandler.h"
#include "deviceHandler.h"
#include "segcp.h"
#include "uartHandler.h"

static DevConfig dev_config;
static DevConfig_E dev_config_e;

DevConfig* get_DevConfig_pointer(void)
{
	return &dev_config;
}
DevConfig_E* get_DevConfig_E_pointer(void)
{
	return &dev_config_e;
}
void set_DevConfig_to_factory_value(void)
{
	uint8_t i;
	
	dev_config.config_common.packet_size = sizeof(DevConfig);
	
	/** 
	  * Product code 
	  * WIZ550S2E: 000
	  * WIZ550web: 120
	  * W7500S2E : 010 (temporary) 
	  */
	dev_config.device_common.module_type[0] = 0x00;
	dev_config.device_common.module_type[1] = 0x01;
	dev_config.device_common.module_type[2] = 0x00;
	
	memset(dev_config.device_common.module_name, 0x00, sizeof(DEVICE_ID_DEFAULT));
	memcpy(dev_config.device_common.module_name, DEVICE_ID_DEFAULT, sizeof(DEVICE_ID_DEFAULT));

	dev_config.device_common.fw_ver[0] = MAJOR_VER;
	dev_config.device_common.fw_ver[1] = MINOR_VER;
	dev_config.device_common.fw_ver[2] = MAINTENANCE_VER;

	dev_config.network_common.local_ip[0] = 192;
	dev_config.network_common.local_ip[1] = 168;
	dev_config.network_common.local_ip[2] = 11;
	dev_config.network_common.local_ip[3] = 2;
    
	dev_config.network_common.gateway[0] = 192;
	dev_config.network_common.gateway[1] = 168;
	dev_config.network_common.gateway[2] = 11;
	dev_config.network_common.gateway[3] = 1;
    
	dev_config.network_common.subnet[0] = 255;
	dev_config.network_common.subnet[1] = 255;
	dev_config.network_common.subnet[2] = 255;
	dev_config.network_common.subnet[3] = 0;

	for(i=0; i<DEVICE_UART_CNT; i++)
	{
		dev_config.network_connection[i].working_mode = TCP_SERVER_MODE; 
		dev_config.network_connection[i].working_state = ST_OPEN;
        
		dev_config.network_connection[i].remote_ip[0] = 192;
		dev_config.network_connection[i].remote_ip[1] = 168;
		dev_config.network_connection[i].remote_ip[2] = 11;
		dev_config.network_connection[i].remote_ip[3] = 3;
        
		dev_config.network_connection[i].local_port = 5000+i;
		dev_config.network_connection[i].remote_port = 5000+i;
		
		dev_config.tcp_option[i].inactivity = 0;		
		dev_config.tcp_option[i].reconnection = 3000;	
		
		dev_config.serial_data_packing[i].packing_time = 0;
		dev_config.serial_data_packing[i].packing_size = 0;
		dev_config.serial_data_packing[i].packing_delimiter[0] = 0; // packing_delimiter used only one-byte (for WIZ107SR compatibility)
		dev_config.serial_data_packing[i].packing_delimiter[1] = 0;
		dev_config.serial_data_packing[i].packing_delimiter[2] = 0;
		dev_config.serial_data_packing[i].packing_delimiter[3] = 0;
		dev_config.serial_data_packing[i].packing_delimiter_length = 0;
		dev_config.serial_data_packing[i].packing_data_appendix = 0;
		
		dev_config.tcp_option[i].keepalive_en = ENABLE;
		dev_config.tcp_option[i].keepalive_wait_time = 7000;
		dev_config.tcp_option[i].keepalive_retry_time = 5000;
		
		dev_config.serial_option[i].uart_interface = UART_IF_RS232_TTL;
		dev_config.serial_option[i].baud_rate = baud_115200;
		dev_config.serial_option[i].data_bits = word_len8;
		dev_config.serial_option[i].parity = parity_none;
		dev_config.serial_option[i].stop_bits = stop_bit1;
		dev_config.serial_option[i].flow_control = flow_none;

#ifdef	__USE_UART_DTR_DSR__
		dev_config.serial_option[i].dtr_en = ENABLE;
		dev_config.serial_option[i].dsr_en = ENABLE;
#else
		dev_config.serial_option[i].dtr_en = DISABLE;
		dev_config.serial_option[i].dsr_en = DISABLE;
#endif
	}
	
	dev_config.serial_common.serial_debug_en = ENABLE;
	
	memset(dev_config.config_common.pw_search, 0x00, sizeof(dev_config.config_common.pw_search));		// Added for WIZ107SR compatibility;
	for(i=0; i<DEVICE_UART_CNT; i++)
	{
		memset(dev_config.tcp_option[i].pw_connect, 0x00, sizeof(dev_config.tcp_option[i].pw_connect));
		dev_config.tcp_option[i].pw_connect_en = DISABLE;
	}
	
	dev_config.network_option.dhcp_use = DISABLE;
	dev_config.network_option.dns_use = DISABLE;
	
	dev_config.network_option.dns_server_ip[0] = 8;	// Default DNS server IP: Google Public DNS (8.8.8.8)
	dev_config.network_option.dns_server_ip[1] = 8;
	dev_config.network_option.dns_server_ip[2] = 8;
	dev_config.network_option.dns_server_ip[3] = 8;
	memset(dev_config.network_option.dns_domain_name, 0x00, sizeof(dev_config.network_option.dns_domain_name));

	dev_config.serial_command.serial_command = ENABLE;
	dev_config.serial_command.serial_command_echo = DISABLE;
	dev_config.serial_command.serial_trigger[0] = 0x2b;	// Defualt serial command mode trigger code: '+++' (0x2b, 0x2b, 0x2b)
	dev_config.serial_command.serial_trigger[1] = 0x2b;
	dev_config.serial_command.serial_trigger[2] = 0x2b;

#ifdef __USE_USERS_GPIO__
	dev_config.user_io_info.user_io_enable = USER_IO_A | USER_IO_B | USER_IO_C | USER_IO_D; // [Enabled] / Disabled	
	dev_config.user_io_info.user_io_type = USER_IO_A; // Analog: USER_IO_A, / Digital: USER_IO_B, USER_IO_C, USER_IO_D
	dev_config.user_io_info.user_io_direction = USER_IO_C | USER_IO_D; // IN / IN / OUT / OUT
	dev_config.user_io_info.user_io_status = 0;
#else
	dev_config.user_io_info.user_io_enable = 0;
	dev_config.user_io_info.user_io_type = 0;
	dev_config.user_io_info.user_io_direction = 0;
	dev_config.user_io_info.user_io_status = 0;
#endif

	dev_config_e.firmware_update.fwup_flag = DISABLE;
	dev_config_e.firmware_update.fwup_port = DEVICE_FWUP_PORT;
	dev_config_e.firmware_update.fwup_size = 0;
	
	// Extended Fields: Firmware update by HTTP server
	dev_config_e.firmware_update.fwup_server_port = FWUP_SERVER_PORT;
	dev_config_e.firmware_update.fwup_server_use_default = ENABLE;
	
	// Planned to apply
	memset(dev_config_e.firmware_update.fwup_server_domain, 0x00, sizeof(dev_config_e.firmware_update.fwup_server_domain));
	memcpy(dev_config_e.firmware_update.fwup_server_domain, FWUP_SERVER_DOMAIN, sizeof(FWUP_SERVER_DOMAIN));
	memset(dev_config_e.firmware_update.fwup_server_binpath, 0x00, sizeof(dev_config_e.firmware_update.fwup_server_binpath));
	memcpy(dev_config_e.firmware_update.fwup_server_binpath, FWUP_SERVER_BINPATH, sizeof(FWUP_SERVER_BINPATH));
}

void load_DevConfig_from_storage(void)
{
    uint8_t i;
	
	init_uart_if_sel();
	
	read_storage(STORAGE_CONFIG, 0, &dev_config, sizeof(DevConfig));
    read_storage(STORAGE_CONFIG_E, 0, &dev_config_e, sizeof(DevConfig_E));

	if(dev_config.config_common.packet_size == 0x0000 || dev_config.config_common.packet_size == 0xFFFF)
	{
		set_DevConfig_to_factory_value();
		write_storage(STORAGE_CONFIG, 0, &dev_config, sizeof(DevConfig));
        write_storage(STORAGE_CONFIG_E, 0, &dev_config_e, sizeof(DevConfig_E));
	}
	
    for(i=0; i<DEVICE_UART_CNT; i++)
    {
		if((dev_config.serial_option[i].flow_control == flow_rtsonly) || (dev_config.serial_option[i].flow_control == flow_reverserts))	
			dev_config.serial_option[i].uart_interface = UART_IF_RS485;
		else
			dev_config.serial_option[i].uart_interface = get_uart_if_sel();
    }
    
	dev_config.device_common.fw_ver[0] = MAJOR_VER;
	dev_config.device_common.fw_ver[1] = MINOR_VER;
	dev_config.device_common.fw_ver[2] = MAINTENANCE_VER;
}

void save_DevConfig_to_storage(void)
{
	write_storage(STORAGE_CONFIG, 0, &dev_config, sizeof(DevConfig));
    write_storage(STORAGE_CONFIG_E, 0, &dev_config_e, sizeof(DevConfig_E));
}

void get_DevConfig_value(void *dest, const void *src, uint16_t size)
{
	memcpy(dest, src, size);
}

void set_DevConfig_value(void *dest, const void *value, const uint16_t size)
{
	memcpy(dest, value, size);
}

void set_DevConfig(wiz_NetInfo *net)
{
	set_DevConfig_value(dev_config.network_common.mac, net->mac, sizeof(dev_config.network_common.mac));
	set_DevConfig_value(dev_config.network_common.local_ip, net->ip, sizeof(dev_config.network_common.local_ip));
	set_DevConfig_value(dev_config.network_common.gateway, net->gw, sizeof(dev_config.network_common.gateway));
	set_DevConfig_value(dev_config.network_common.subnet, net->sn, sizeof(dev_config.network_common.subnet));
	set_DevConfig_value(dev_config.network_option.dns_server_ip, net->dns, sizeof(dev_config.network_option.dns_server_ip));
	
	if(net->dhcp == NETINFO_STATIC)
		dev_config.network_option.dhcp_use = DISABLE;
	else
		dev_config.network_option.dhcp_use = ENABLE;
}

void get_DevConfig(wiz_NetInfo *net)
{
	get_DevConfig_value(net->mac, dev_config.network_common.mac, sizeof(net->mac));
	get_DevConfig_value(net->ip, dev_config.network_common.local_ip, sizeof(net->ip));
	get_DevConfig_value(net->gw, dev_config.network_common.gateway, sizeof(net->gw));
	get_DevConfig_value(net->sn, dev_config.network_common.subnet, sizeof(net->sn));
	get_DevConfig_value(net->dns, dev_config.network_option.dns_server_ip, sizeof(net->dns));
	
	if(dev_config.network_option.dhcp_use == ENABLE)
		net->dhcp = NETINFO_DHCP;
	else
		net->dhcp = NETINFO_STATIC;
}

void display_Net_Info(void)
{
	uint8_t i;
	
	DevConfig *value = get_DevConfig_pointer();
	wiz_NetInfo gWIZNETINFO;
	
	ctlnetwork(CN_GET_NETINFO, (void*) &gWIZNETINFO);
	printf(" # MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n", gWIZNETINFO.mac[0], gWIZNETINFO.mac[1], gWIZNETINFO.mac[2], gWIZNETINFO.mac[3], gWIZNETINFO.mac[4], gWIZNETINFO.mac[5]);
	printf(" # IP : %d.%d.%d.%d / Port1: %d, Port2: %d\r\n", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3], value->network_connection[0].local_port, value->network_connection[1].local_port);
	printf(" # GW : %d.%d.%d.%d\r\n", gWIZNETINFO.gw[0], gWIZNETINFO.gw[1], gWIZNETINFO.gw[2], gWIZNETINFO.gw[3]);
	printf(" # SN : %d.%d.%d.%d\r\n", gWIZNETINFO.sn[0], gWIZNETINFO.sn[1], gWIZNETINFO.sn[2], gWIZNETINFO.sn[3]);
	printf(" # DNS: %d.%d.%d.%d\r\n", gWIZNETINFO.dns[0], gWIZNETINFO.dns[1], gWIZNETINFO.dns[2], gWIZNETINFO.dns[3]);
	
	for(i=0; i<DEVICE_UART_CNT; i++)
	{
		if(value->network_connection[i].working_mode != TCP_SERVER_MODE)
		{
			if(value->network_option.dns_use == ENABLE)
			{
				if(i==0)
				{
					printf(" # Destination Domain: %s / Port: %d\r\n", value->network_option.dns_domain_name, value->network_connection[i].remote_port);
				}
			}
			else
			{
				printf(" # Destination IP: %d.%d.%d.%d / Port: %d\r\n", value->network_connection[i].remote_ip[0], 
                                                                        value->network_connection[i].remote_ip[1], 
                                                                        value->network_connection[i].remote_ip[2], 
                                                                        value->network_connection[i].remote_ip[3], 
                                                                        value->network_connection[i].remote_port);
				if(value->network_connection[i].working_mode == UDP_MODE)
				{
					if((value->network_connection[i].remote_ip[0] == 0) && 
						(value->network_connection[i].remote_ip[1] == 0) && 
						(value->network_connection[i].remote_ip[2] == 0) && 
						(value->network_connection[i].remote_ip[3] == 0))
					{
						printf(" ## UDP 1:N Mode\r\n");
					}
					else
					{
						printf(" ## UDP 1:1 Mode\r\n");
					}
				}
			}
		}	
	}
}

void Mac_Conf(void)
{
	DevConfig *value = get_DevConfig_pointer();
	
	setSHAR(value->network_common.mac);
}

void Net_Conf(void)
{
	DevConfig *value = get_DevConfig_pointer();
	wiz_NetInfo gWIZNETINFO;

	/* wizchip netconf */
	get_DevConfig_value(gWIZNETINFO.mac, value->network_common.mac, sizeof(gWIZNETINFO.mac[0]) * 6);
	get_DevConfig_value(gWIZNETINFO.ip, value->network_common.local_ip, sizeof(gWIZNETINFO.ip[0]) * 4);
	get_DevConfig_value(gWIZNETINFO.gw, value->network_common.gateway, sizeof(gWIZNETINFO.gw[0]) * 4);
	get_DevConfig_value(gWIZNETINFO.sn, value->network_common.subnet, sizeof(gWIZNETINFO.sn[0]) * 4);
	get_DevConfig_value(gWIZNETINFO.dns, value->network_option.dns_server_ip, sizeof(gWIZNETINFO.dns));
	
	if(value->network_option.dhcp_use)
		gWIZNETINFO.dhcp = NETINFO_DHCP;
	else
		gWIZNETINFO.dhcp = NETINFO_STATIC;

	ctlnetwork(CN_SET_NETINFO, (void*) &gWIZNETINFO);
}

void set_dhcp_mode(void)
{
	DevConfig *value = get_DevConfig_pointer();

	value->network_option.dhcp_use = ENABLE;
}

void set_static_mode(void)
{
	DevConfig *value = get_DevConfig_pointer();

	value->network_option.dhcp_use = DISABLE;
}

void set_mac(uint8_t *mac)
{
	DevConfig *value = get_DevConfig_pointer();

	memcpy(value->network_common.mac, mac, sizeof(value->network_common.mac));
}
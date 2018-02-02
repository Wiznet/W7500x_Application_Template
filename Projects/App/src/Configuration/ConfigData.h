/*
 * ConfigData.h
 */

#ifndef __CONFIGDATA_H__
#define __CONFIGDATA_H__

#include <stdint.h>
#include "wizchip_conf.h"
#include "W7500x_board.h"

#define FWUP_DOMAIN_SIZE		30
#define FWUP_BINPATH_SIZE		40

/* REMOTE Firmware update */
// HTTP Request: DEVICE_FWUP_DOMAIN + DEVICE_FWUP_BINPATH
// FWUP Domain array size / binpath array size depends on FWUP_DOMAIN_SIZE / FWUP_BINPATH_SIZE (ConfigData.h)
#define FWUP_SERVER_PORT			80
#define FWUP_SERVER_DOMAIN			"device.wizwiki.net"
#define FWUP_SERVER_BINPATH			"/wiz750sr/fw/W7500x_S2E_App.bin"

struct __device_common {
    uint8_t module_type[3];		// 모듈의 종류별로 코드를 부여하고 이를 사용한다.
    uint8_t fw_ver[3];			// 10진수. Major Version . Minor Version . Maintenance Version 버전으로 나뉨
	uint8_t module_name[20];
    /* Check */
    uint8_t module_mode;
} __attribute__((packed));

struct __config_common {
    uint16_t app_protocol;
    uint16_t packet_size;
    char pw_search[10];			// ## Eric, Field added for compatibility with WIZ107SR
} __attribute__((packed));

// ## Eric, Field added for compatibility with WIZ107SR
struct __firmware_update {
    uint8_t fwup_flag;
	uint16_t fwup_port;
	uint32_t fwup_size;
    uint8_t fwup_server_flag;
	uint16_t fwup_server_port;
	uint8_t fwup_server_use_default;
	uint8_t fwup_server_domain[FWUP_DOMAIN_SIZE];
	uint8_t fwup_server_binpath[FWUP_BINPATH_SIZE];
} __attribute__((packed));

struct __network_common {
    uint8_t mac[6];
    uint8_t local_ip[4];
    uint8_t gateway[4];
    uint8_t subnet[4];
} __attribute__((packed));

struct __network_connection {
    uint8_t working_mode;			// TCP_CLIENT_MODE (0), TCP_SERVER_MODE (1), TCP_MIXED_MODE (2), UDP_MODE (3)
    uint8_t working_state;	
    //uint8_t state;					// WIZ107SR: BOOT(0), OPEN (1), CONNECT (2), UPGRADE (3), ATMODE (4) // WIZ550S2E: 소켓의 상태 TCP의 경우 Not Connected, Connected, UDP의 경우 UDP
    uint8_t remote_ip[4];			// Must Be 4byte Alignment
    uint16_t local_port;
    uint16_t remote_port;
} __attribute__((packed));

struct __network_option {
    uint8_t dhcp_use;
    uint8_t dns_use;
    uint8_t dns_server_ip[4];
    char dns_domain_name[40];
} __attribute__((packed));

struct __tcp_option {
    uint16_t inactivity;
    uint8_t keepalive_en;				// ## Eric, Field added for compatibility with WIZ107SR
    uint16_t keepalive_wait_time;		// ## Eric, Field added for compatibility with WIZ107SR
    uint16_t keepalive_retry_time;		// ## Eric, Field added for compatibility with WIZ107SR
    uint16_t reconnection;
    //char pw_setting[10];
	char pw_connect[10];
	uint8_t pw_connect_en;
} __attribute__((packed));

struct __serial_common {
    uint8_t uart_interface_cnt;
    uint8_t serial_debug_en;	// ## Eric, Field added for compatibility with WIZ107SR
} __attribute__((packed));

struct __serial_option {
    uint8_t uart_interface;		// UART interface; [0] RS-232/TTL, [1] RS-422/485, This value is determined at the initial routine of device.
    uint8_t baud_rate;			// WIZ107SR: 0 ~ (enum) / WIZ550S2E: baud rate values //uint32_t baud_rate;			// WIZ107SR: 0 ~ (enum) / WIZ550S2E: baud rate values
    uint8_t data_bits;			// 7, 8, 9
    uint8_t parity;				// None, Odd, Even
    uint8_t stop_bits;			// 1, 1.5, 2
    uint8_t flow_control;		// None, RTS/CTS, XON/XOFF, RTS Only for RS422/485l
    uint8_t dtr_en;				// DTR/DSR Enable, Pins for these signals are shared with [Connection status pins]
    uint8_t dsr_en;				// DTR/DSR Enable, Pins for these signals are shared with [Connection status pins]
} __attribute__((packed));

struct __serial_data_packing {
    uint16_t packing_time;			// 0~65535
    uint16_t packing_size;			// 0~65535
    uint8_t packing_delimiter[4];
    uint8_t packing_delimiter_length;	// 0~4
    uint8_t packing_data_appendix;		// 0~2(구분자까지 전송, 구분자 +1바이트 까지 전송, 구분자 +2바이트 까지 전송)
} __attribute__((packed));

struct __serial_command {
    uint8_t serial_command;			// Serial Command Mode 사용 여부
    uint8_t serial_trigger[3];		// Serial Command Mode 진입을 위한 Trigger 코드
    uint8_t serial_command_echo;	// Serial Command 입력 echoback 여부
} __attribute__((packed));

struct __user_io_info {
    uint16_t user_io_enable;		// 0: Disable / 1: Enable
	uint16_t user_io_type;			// 0: Digital / 1: Analog
	uint16_t user_io_direction;		// 0: Input / 1: Output
	uint16_t user_io_status;		// Digital Output only`
} __attribute__((packed));


typedef struct __DevConfig {

    struct __device_common device_common;
    struct __config_common config_common;				
    struct __network_common network_common;
    struct __network_connection network_connection[DEVICE_UART_CNT];	
    struct __network_option network_option;
    struct __tcp_option tcp_option[DEVICE_UART_CNT];
    struct __serial_common serial_common;	
    struct __serial_command serial_command;	
    struct __serial_option serial_option[DEVICE_UART_CNT];	
    struct __serial_data_packing serial_data_packing[DEVICE_UART_CNT];	
    struct __user_io_info user_io_info;		
    
} __attribute__((packed)) DevConfig;

typedef struct __DevConfig_E {

    struct __firmware_update firmware_update;						
    
} __attribute__((packed)) DevConfig_E;

DevConfig* get_DevConfig_pointer(void);
DevConfig_E* get_DevConfig_E_pointer(void);

void set_DevConfig_to_factory_value(void);
void load_DevConfig_from_storage(void);
void save_DevConfig_to_storage(void);
void get_DevConfig_value(void *dest, const void *src, uint16_t size);
void set_DevConfig_value(void *dest, const void *value, const uint16_t size);
void set_DevConfig(wiz_NetInfo *net);
void get_DevConfig(wiz_NetInfo *net);

void display_Net_Info(void);
void Mac_Conf(void);
void Net_Conf(void);
void set_dhcp_mode(void);
void set_static_mode(void);
void set_mac(uint8_t *mac);

#endif /* S2E_PACKET_H_ */
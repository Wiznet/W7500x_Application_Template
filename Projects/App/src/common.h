/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _COMMON_H
#define _COMMON_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Application Firmware Version ----------------------------------------------*/
#define MAJOR_VER			2
#define MINOR_VER			1
#define MAINTENANCE_VER		0

#define STR_VERSION_STATUS	"Develop" // or "Stable"
//#define STR_VERSION_STATUS	"Stable"

/* W7500x HW Socket Definition -----------------------------------------------*/
#define SOCK_MAX_USED		6

#define SOCK_CHANNEL0		0
#define SOCK_CHANNEL1		1

#define SOCK_CONFIG_UDP		2
#define SOCK_CONFIG_TCP		3

#define SEGCP_UDP_SOCK		SOCK_CONFIG_UDP
#define SEGCP_TCP_SOCK		SOCK_CONFIG_TCP

#define SOCK_DHCP			4
#define SOCK_DNS			5
#define SOCK_FWUPDATE		SOCK_DNS

/* W7500x Board Internal / External OSC clock Settings -----------------------*/
#define CLOCK_SOURCE_INTERNAL	(0x0UL)
#define CLOCK_SOURCE_EXTERNAL	(0x1UL)

/* Source clock frequency ----------------------------------------------------*/
#define PLL_SOURCE_8MHz		(8000000UL)     
#define PLL_SOURCE_12MHz	(12000000UL)   
#define PLL_SOURCE_24MHz	(24000000UL)    

/* Targer system clock frequency ---------------------------------------------*/
#define SYSTEM_CLOCK_8MHz	(8000000UL)
#define SYSTEM_CLOCK_12MHz	(12000000UL)
#define SYSTEM_CLOCK_16MHz	(16000000UL)
#define SYSTEM_CLOCK_24MHz	(24000000UL)
#define SYSTEM_CLOCK_32MHz	(32000000UL)
#define SYSTEM_CLOCK_36MHz	(36000000UL)
#define SYSTEM_CLOCK_48MHz	(48000000UL)    

/* Buffer size ---------------------------------------------------------------*/
#define DATA_BUF_SIZE		1024
#define CONFIG_BUF_SIZE		512

/*  Available board list -----------------------------------------------------*/
#define WIZwiki_W7500		0
#define WIZwiki_W7500ECO	1
#define WIZ750SR			2
#define W7500P_S2E			3
#define WIZ750SR_1xx		5
#define WIZ752SR_12x    	6	
#define UNKNOWN_DEVICE		0xff

/* Defines for S2E Status ----------------------------------------------------*/
typedef enum
{
	ST_BOOT, 
	ST_OPEN, 
	ST_CONNECT, 
	ST_UPGRADE, 
	ST_ATMODE, 
	ST_UDP
} teDEVSTATUS;   

#define DEVICE_AT_MODE		0
#define DEVICE_GW_MODE		1

#define TCP_CLIENT_MODE		0
#define TCP_SERVER_MODE		1
#define TCP_MIXED_MODE		2
#define UDP_MODE			3

#define MIXED_SERVER		0
#define MIXED_CLIENT		1

/* On/Off Status -------------------------------------------------------------*/
typedef enum
{
	OFF		= 0,
	ON		= 1
} OnOff_State_Type;
typedef enum
{
	FALSE	= 0,
	TRUE	= 1
} bool;

#define OP_COMMAND	0
#define OP_DATA		1

#define RET_OK			0
#define RET_NOK			-1
#define RET_TIMEOUT		-2

#define STR_UART		"UART"
#define STR_ENABLED		"Enabled"
#define STR_DISABLED	"Disabled"
#define STR_BAR			"=================================================="

#endif /* _COMMON_H */

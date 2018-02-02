#ifndef DEVICEHANDLER_H_
#define DEVICEHANDLER_H_

#include <stdint.h>
//#include "W7500x_board.h"
#include "storageHandler.h"
#include "flashHandler.h"
/* Debug message enable */
//#define _FWUP_DEBUG_

/* Application Port */
#define DEVICE_SEGCP_PORT			50001	// Search / Setting Port (UDP Broadcast / TCP unicast)
#define DEVICE_FWUP_PORT			50002	// Firmware Update Port
#define DEVICE_DDNS_PORT			3030	// Not 	used

// HTTP Response: Status code
#define STATUS_HTTP_OK				200

/* W7500S2E Application flash memory map */
#define DEVICE_BOOT_SECT_SIZE       112
#define DEVICE_BOOT_SIZE			(SECT_SIZE * DEVICE_BOOT_SECT_SIZE)//28k byte

#ifdef __USE_APPBACKUP_AREA__
    #define DEVICE_APP_SECT_SIZE    199
    #define DEVICE_APP_SIZE			(SECT_SIZE * DEVICE_APP_SECT_SIZE)
    
    #if ((DEVICE_BOOT_SECT_SIZE +(DEVICE_APP_SECT_SIZE * 2))>511)
    #error "No space in flash regions of w7500x."
    #endif
    
#else
    #define DEVICE_APP_SECT_SIZE    399
    #define DEVICE_APP_SIZE			(SECT_SIZE * DEVICE_APP_SECT_SIZE)
    
    #if ((DEVICE_BOOT_SECT_SIZE + DEVICE_APP_SECT_SIZE) > 511)
    #error "No space in flash regions of w7500x."
    #endif
    
#endif

#define DEVICE_BOOT_ADDR					(FLASH_START_ADDR) // Boot: 28kB
#define DEVICE_APP_MAIN_ADDR				(DEVICE_BOOT_ADDR + DEVICE_BOOT_SIZE)
#define DEVICE_APP_BACKUP_ADDR				(DEVICE_APP_MAIN_ADDR + DEVICE_APP_SIZE)
#define DEVICE_MAC_ADDR						(MAC_ADDR)
#define DEVICE_CONFIG_ADDR					(DAT0_START_ADDR)
#define DEVICE_CONFIG_E_ADDR				(DAT1_START_ADDR)

/* Defines for firmware update */
#define DEVICE_FWUP_SIZE			DEVICE_APP_SIZE // Firmware size - 50kB MAX
#define DEVICE_FWUP_TIMEOUT			10000 // 10 secs.

#define DEVICE_FWUP_RET_SUCCESS		0x80
#define DEVICE_FWUP_RET_FAILED		0x40
#define DEVICE_FWUP_RET_PROGRESS	0x20
#define DEVICE_FWUP_RET_NONE		0x00

void device_set_factory_default(void);
void device_socket_termination(void);
void device_reboot(void);

uint8_t device_firmware_update(teDATASTORAGE stype); // Firmware update by Configuration tool / Flash to Flash

// function for timer
void device_timer_msec(void);

#endif /* DEVICEHANDLER_H_ */

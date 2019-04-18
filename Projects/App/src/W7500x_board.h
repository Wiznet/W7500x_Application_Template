/*
*
@file		W7500x_board.h
@brief
*/

#ifndef __W7500X_BOARD_H__ 
#define __W7500X_BOARD_H__ 

#include <stdint.h>
#include "common.h"

////////////////////////////////
// Product Configurations     //
////////////////////////////////

/* Target Board Selector */
#define DEVICE_BOARD_NAME	WIZ752SR_12x

#ifdef DEVICE_BOARD_NAME
	#if (DEVICE_BOARD_NAME == WIZ750SR)
        #define __W7500P__                                    
        #define __USE_UART_IF_SELECTOR__	    // RS-232/TTL or RS-422/485 selector using UART IF selector pin
		#define __USE_EXT_EEPROM__			    // External EEPROM or Internal Data flash (DAT0/1)
		#define __USE_BOOT_ENTRY__			    // Application boot mode entry pin activated
		#define __USE_APPBACKUP_AREA__		    // If this option activated, Application firmware area is consists of App (50kB) and App backup (50kB). If not, user's application can be 100kB size. (Does not use the backup area)
		#define __USE_GPIO_HARDWARE_FLOWCONTROL__
		#define __USE_USERS_GPIO__
        //#define __USE_HW_TRIG_PIN__
        //#define __USE_PHYLINK_CHECK_PIN__
        //#define __USE_UART_DTR_DSR__
        //#define __USE_STATUS_PHYLINK_PIN__
        //#define __USE_STATUS_TCPCONNECT_PIN__
		#define DEVICE_CLOCK_SELECT	            CLOCK_SOURCE_EXTERNAL
		#define DEVICE_PLL_SOURCE_CLOCK         PLL_SOURCE_12MHz
		#define DEVICE_TARGET_SYSTEM_CLOCK      SYSTEM_CLOCK_48MHz
		#define DEVICE_ID_DEFAULT               "WIZ750SR" 
        #define DEVICE_UART_CNT                 1                                          
	#elif (DEVICE_BOARD_NAME == W7500P_S2E) 
		#define __W7500P__                  
		#define __USE_UART_IF_SELECTOR__	
		#define __USE_BOOT_ENTRY__			
		#define __USE_APPBACKUP_AREA__
		#define __USE_GPIO_HARDWARE_FLOWCONTROL__
		#define __USE_USERS_GPIO__
        #define __USE_HW_TRIG_PIN__
        #define __USE_PHYLINK_CHECK_PIN__
        #define __USE_UART_DTR_DSR__
        #define __USE_STATUS_PHYLINK_PIN__
        #define __USE_STATUS_TCPCONNECT_PIN__
		#define DEVICE_CLOCK_SELECT	            CLOCK_SOURCE_EXTERNAL
		#define DEVICE_PLL_SOURCE_CLOCK         PLL_SOURCE_12MHz
		#define DEVICE_TARGET_SYSTEM_CLOCK      SYSTEM_CLOCK_48MHz
		#define DEVICE_ID_DEFAULT               "W7500P-S2E"
        #define DEVICE_UART_CNT		            1 
	#elif (DEVICE_BOARD_NAME == WIZ750SR_1xx)                    
		//#define __USE_UART_IF_SELECTOR__	
		//#define __USE_EXT_EEPROM__			
		#define __USE_BOOT_ENTRY__			
		#define __USE_APPBACKUP_AREA__		
		#define __USE_GPIO_HARDWARE_FLOWCONTROL__
		#define __USE_USERS_GPIO__
		#define __USE_UART_DTR_DSR__
        #define __USE_HW_TRIG_PIN__
        #define __USE_PHYLINK_CHECK_PIN__
        #define __USE_STATUS_PHYLINK_PIN__
        #define __USE_STATUS_TCPCONNECT_PIN__
		#define DEVICE_CLOCK_SELECT	            CLOCK_SOURCE_EXTERNAL
		#define DEVICE_PLL_SOURCE_CLOCK         PLL_SOURCE_12MHz
		#define DEVICE_TARGET_SYSTEM_CLOCK      SYSTEM_CLOCK_48MHz
		#define DEVICE_ID_DEFAULT               "WIZ750SR-1xx" // Device name
        #define DEVICE_UART_CNT		            1 // Not used
	#elif (DEVICE_BOARD_NAME == W7500_S2E)  
		//#define __USE_UART_IF_SELECTOR__	
		//#define __USE_EXT_EEPROM__			
		#define __USE_BOOT_ENTRY__			
		#define __USE_APPBACKUP_AREA__		
		#define __USE_GPIO_HARDWARE_FLOWCONTROL__
		#define __USE_USERS_GPIO__
		//#define __USE_UART_DTR_DSR__
        #define __USE_HW_TRIG_PIN__
        #define __USE_PHYLINK_CHECK_PIN__
        #define __USE_STATUS_PHYLINK_PIN__
        #define __USE_STATUS_TCPCONNECT_PIN__
		#define DEVICE_CLOCK_SELECT	            CLOCK_SOURCE_EXTERNAL
		#define DEVICE_PLL_SOURCE_CLOCK         PLL_SOURCE_12MHz
		#define DEVICE_TARGET_SYSTEM_CLOCK      SYSTEM_CLOCK_48MHz
		#define DEVICE_ID_DEFAULT               "W7500-S2E" // Device name
        #define DEVICE_UART_CNT		            1 // Not used
    #elif (DEVICE_BOARD_NAME == WIZ752SR_12x)  
		#define __W7500P__  //mason
		//#define __USE_UART_IF_SELECTOR__	
		//#define __USE_EXT_EEPROM__			
		//#define __USE_BOOT_ENTRY__			
		#define __USE_APPBACKUP_AREA__		
		#define __USE_GPIO_HARDWARE_FLOWCONTROL__
		#define __USE_USERS_GPIO__
    //#define __USE_HW_TRIG_PIN__
        #define __USE_PHYLINK_CHECK_PIN__
    //#define __USE_UART_DTR_DSR__
        #define __USE_STATUS_PHYLINK_PIN__
        #define __USE_STATUS_TCPCONNECT_PIN__
		//#define DEVICE_CLOCK_SELECT	            CLOCK_SOURCE_EXTERNAL //mason
		//#define DEVICE_PLL_SOURCE_CLOCK         PLL_SOURCE_12MHz
		//#define DEVICE_TARGET_SYSTEM_CLOCK      SYSTEM_CLOCK_48MHz

    #define DEVICE_CLOCK_SELECT	            CLOCK_SOURCE_EXTERNAL
		#define DEVICE_PLL_SOURCE_CLOCK         PLL_SOURCE_8MHz
		#define DEVICE_TARGET_SYSTEM_CLOCK      SYSTEM_CLOCK_48MHz
		
		#define DEVICE_ID_DEFAULT               "WIZ752SR-12x" // Device name
        #define DEVICE_UART_CNT		            2 // Not used
		#define PHY_CHECK_TIME					1
	#else
		//#define __USE_UART_IF_SELECTOR__
		#define __USE_EXT_EEPROM__
		#define __USE_APPBACKUP_AREA__
		#define __USE_GPIO_HARDWARE_FLOWCONTROL__
		#define DEVICE_CLOCK_SELECT	         CLOCK_SOURCE_EXTERNAL
		#define DEVICE_PLL_SOURCE_CLOCK      PLL_SOURCE_8MHz
		#define DEVICE_TARGET_SYSTEM_CLOCK   SYSTEM_CLOCK_48MHz
		#define DEVICE_ID_DEFAULT            "W7500-S2E" // Device name: WIZwiki_W7500 or WIZwiki_W7500ECO Board
        #define DEVICE_UART_CNT		1 // Not used
	#endif
#else
	#define __USE_APPBACKUP_AREA__
	#define DEVICE_CLOCK_SELECT	             CLOCK_SOURCE_INTERNAL
	#define DEVICE_PLL_SOURCE_CLOCK          PLL_SOURCE_8MHz
	#define DEVICE_TARGET_SYSTEM_CLOCK       SYSTEM_CLOCK_48MHz
	#define DEVICE_BOARD_NAME                UNKNOWN_DEVICE
	#define DEVICE_ID_DEFAULT                "UNKNOWN"
    #define DEVICE_UART_CNT		1 // Not used
#endif

// PHY init defines: USE MDC/MDIO
#define __DEF_USED_MDIO__ 

#ifdef __DEF_USED_MDIO__ // MDC/MDIO defines
	#ifndef __W7500P__
        #if (DEVICE_BOARD_NAME == WIZwiki_W7500)
            #define __DEF_USED_IC101AG__ 
        #endif
		#define W7500x_MDIO    GPIO_Pin_14
		#define W7500x_MDC     GPIO_Pin_15
	#else
		#define W7500x_MDIO    GPIO_Pin_15
		#define W7500x_MDC     GPIO_Pin_14
	#endif
#endif

/* PHY Link check  */
#define PHYLINK_CHECK_CYCLE_MSEC	1000
#define PHYLINK                     0
#define TCPCONNECT                  1

////////////////////////////////
// Pin definitions			  //
////////////////////////////////
#ifdef __USE_UART_DTR_DSR__
    #if (DEVICE_BOARD_NAME == WIZ750SR_1xx)
        #define DTR_0_PIN					GPIO_Pin_10
        #define DTR_0_PORT					GPIOA
        #define DTR_0_PAD_AF				PAD_AF1

        #define DSR_0_PIN					GPIO_Pin_9
        #define DSR_0_PORT					GPIOA
        #define DSR_0_PAD_AF				PAD_AF1
    #elif (DEVICE_BOARD_NAME == WIZ750SR)
        #define DTR_0_PIN					GPIO_Pin_10
        #define DTR_0_PORT					GPIOA
        #define DTR_0_PAD_AF				PAD_AF1

        #define DSR_0_PIN					GPIO_Pin_1
        #define DSR_0_PORT					GPIOA
        #define DSR_0_PAD_AF				PAD_AF1
    #else
        #error "Please specify a pin name of UART DTR/DSR."
    #endif
    #if (DEVICE_UART_CNT == 2)
        #ifndef DTR_1_PIN
            #error "Please specify a pin name of channel 1 DTR/DSR."
        #endif
    #endif 
#endif

#ifdef __USE_PHYLINK_CHECK_PIN__
    #if (DEVICE_BOARD_NAME == WIZ750SR)
        #define PHYLINK_IN_PIN				GPIO_Pin_9
        #define PHYLINK_IN_PORT				GPIOA
        #define PHYLINK_IN_PAD_AF			PAD_AF1
    #elif (DEVICE_BOARD_NAME == WIZ752SR_12x || DEVICE_BOARD_NAME == WIZ750SR_1xx)
        #define PHYLINK_IN_PIN				GPIO_Pin_0
        #define PHYLINK_IN_PORT				GPIOA
        #define PHYLINK_IN_PAD_AF			PAD_AF1
    #else
        #error "Please specify a pin name of PHY link check."
    #endif
#endif

#ifdef __USE_STATUS_PHYLINK_PIN__
    //#if (DEVICE_BOARD_NAME == WIZ750SR_1xx || DEVICE_BOARD_NAME == WIZ750SR) //mason 190327
    #if (DEVICE_BOARD_NAME == WIZ750SR_1xx || DEVICE_BOARD_NAME == WIZ750SR || DEVICE_BOARD_NAME == WIZ752SR_12x)  
        #define STATUS_PHYLINK_PIN			GPIO_Pin_10
        #define STATUS_PHYLINK_PORT			GPIOA
        #define STATUS_PHYLINK_PAD_AF		PAD_AF1
    #else
        #error "Please specify a pin name of status PHY link."
    #endif
#endif
#ifdef __USE_STATUS_TCPCONNECT_PIN__
    #if (DEVICE_BOARD_NAME == WIZ750SR)
        #define STATUS_TCPCONNECT_0_PIN		GPIO_Pin_1
        #define STATUS_TCPCONNECT_0_PORT	GPIOA
        #define STATUS_TCPCONNECT_0_PAD_AF	PAD_AF1
		
	 #elif (DEVICE_BOARD_NAME == WIZ750SR_1xx)
        #define STATUS_TCPCONNECT_0_PIN		GPIO_Pin_9
        #define STATUS_TCPCONNECT_0_PORT	GPIOA
        #define STATUS_TCPCONNECT_0_PAD_AF	PAD_AF1
        
    #elif (DEVICE_BOARD_NAME == WIZ752SR_12x)
        #define STATUS_TCPCONNECT_0_PIN		GPIO_Pin_6
        #define STATUS_TCPCONNECT_0_PORT	GPIOA
        #define STATUS_TCPCONNECT_0_PAD_AF	PAD_AF1

        #define STATUS_TCPCONNECT_1_PIN		GPIO_Pin_7
        #define STATUS_TCPCONNECT_1_PORT	GPIOA
        #define STATUS_TCPCONNECT_1_PAD_AF	PAD_AF1
    #else
        #error "Please specify a pin name of status TCP connect."
    #endif
    
    #if (DEVICE_UART_CNT == 2)
        #ifndef STATUS_TCPCONNECT_1_PIN
            #error "Please specify a pin name of channel 2 status TCP connect."
        #endif
    #endif 
#endif

#ifdef __USE_UART_IF_SELECTOR__
    #if (DEVICE_BOARD_NAME == WIZ750SR || DEVICE_BOARD_NAME == WIZ750SR_1xx) 
        #define UART_IF_SEL_PIN			GPIO_Pin_6
        #define UART_IF_SEL_PORT		GPIOC
        #define UART_IF_SEL_PAD_AF		PAD_AF1 
    #else
        #error "Please specify a pin name of UART inferface selector."
    #endif
#endif

#ifdef __USE_EXT_EEPROM__
	#include "i2cHandler.h"
	#define EEPROM_I2C_SCL_PIN		I2C_SCL_PIN
	#define EEPROM_I2C_SCL_PORT		I2C_SCL_PORT
	#define EEPROM_I2C_SDA_PIN		I2C_SDA_PIN
	#define EEPROM_I2C_SDA_PORT		I2C_SDA_PORT
	#define EEPROM_I2C_PAD_AF		I2C_PAD_AF // GPIO
#endif

#ifdef __USE_BOOT_ENTRY__
    #if ((DEVICE_BOARD_NAME == WIZ750SR_1xx) || (DEVICE_BOARD_NAME == WIZ752SR_12x)) 
        #define BOOT_ENTRY_PIN			GPIO_Pin_8
        #define BOOT_ENTRY_PORT			GPIOA
        #define BOOT_ENTRY_PAD_AF		PAD_AF1
    #elif (DEVICE_BOARD_NAME == WIZ750SR) 
        #define BOOT_ENTRY_PIN			GPIO_Pin_14
        #define BOOT_ENTRY_PORT			GPIOC
        #define BOOT_ENTRY_PAD_AF		PAD_AF1
    #else
        #error "Please specify a pin name of app/boot."
    #endif
#endif

#ifdef __USE_GPIO_HARDWARE_FLOWCONTROL__
    #if ((DEVICE_BOARD_NAME == WIZ750SR) || (DEVICE_BOARD_NAME == WIZ750JR)) // ##20170215 WIZ750Jr
        // UART0
        #define UART0_RTS_PIN				GPIO_Pin_12
        #define UART0_RTS_PORT				GPIOA
        #define UART0_RTS_PAD_AF			PAD_AF1 
        #define UART0_CTS_PIN				GPIO_Pin_11
        #define UART0_CTS_PORT				GPIOA
        #define UART0_CTS_PAD_AF			PAD_AF1

    #elif (DEVICE_BOARD_NAME == WIZ752SR_12x) 
        // UART0
        #define UART0_RTS_PIN				GPIO_Pin_12
        #define UART0_RTS_PORT				GPIOA
        #define UART0_RTS_PAD_AF			PAD_AF1 
        #define UART0_CTS_PIN				GPIO_Pin_11
        #define UART0_CTS_PORT				GPIOA
        #define UART0_CTS_PAD_AF			PAD_AF1
        
        // UART1
        #define UART1_RTS_PIN				GPIO_Pin_1
        #define UART1_RTS_PORT				GPIOC
        #define UART1_RTS_PAD_AF			PAD_AF1 
        #define UART1_CTS_PIN				GPIO_Pin_0
        #define UART1_CTS_PORT				GPIOC
        #define UART1_CTS_PAD_AF			PAD_AF1
    #endif
    #if (DEVICE_UART_CNT == 2)
        #ifndef UART1_RTS_PIN
            #error "Please specify a pin name of channel 2 status TCP connect."
        #endif
    #endif 
#endif

#ifdef __USE_HW_TRIG_PIN__
    #if (DEVICE_BOARD_NAME == WIZ750SR_1xx) // ##20161031 WIZ750Jr
        // HW_TRIG - Command mode switch enable pin
        // Direction: Input (Shared pin with TCP connection status pin)
        #define HW_TRIG_PIN					GPIO_Pin_9
        #define HW_TRIG_PORT				GPIOA
        #define HW_TRIG_PAD_AF				PAD_AF1
    #elif (DEVICE_BOARD_NAME == WIZ750SR)
        // HW_TRIG - Command mode switch enable pin
        // Direction: Input (Shared pin with TCP connection status pin)
        #define HW_TRIG_PIN					GPIO_Pin_1
        #define HW_TRIG_PORT				GPIOA
        #define HW_TRIG_PAD_AF				PAD_AF1
    #else
        #error "Please specify a pin name of Hardware Trigger."
    #endif
#endif

// Expansion GPIOs (4-Pins, GPIO A / B / C / D)
#ifdef __USE_USERS_GPIO__

	//#define MAX_USER_IOn    16
	#define USER_IOn       4
	#define USER_IO_A      (uint16_t)(0x01 <<  0)     // USER's I/O A
	#define USER_IO_B      (uint16_t)(0x01 <<  1)     // USER's I/O B
	#define USER_IO_C      (uint16_t)(0x01 <<  2)     // USER's I/O C
	#define USER_IO_D      (uint16_t)(0x01 <<  3)     // USER's I/O D

	#define USER_IO_DEFAULT_PAD_AF		PAD_AF1 // [2nd] GPIO
	#define USER_IO_AIN_PAD_AF			PAD_AF3 // [4th] AIN

	#define USER_IO_NO_ADC				0xff

	#if (DEVICE_BOARD_NAME == WIZ750SR_1xx) 
		#define USER_IO_A_PIN				GPIO_Pin_15
		#define USER_IO_A_PORT				GPIOC
		#define USER_IO_A_ADC_CH			ADC_CH0

		#define USER_IO_B_PIN				GPIO_Pin_14
		#define USER_IO_B_PORT				GPIOC
		#define USER_IO_B_ADC_CH			ADC_CH1

		#define USER_IO_C_PIN				GPIO_Pin_13
		#define USER_IO_C_PORT				GPIOC
		#define USER_IO_C_ADC_CH			ADC_CH2

		#define USER_IO_D_PIN				GPIO_Pin_12
		#define USER_IO_D_PORT				GPIOC
		#define USER_IO_D_ADC_CH			ADC_CH3
	#elif ((DEVICE_BOARD_NAME == WIZ750SR) || (DEVICE_BOARD_NAME == WIZwiki_W7500ECO))
		#define USER_IO_A_PIN				GPIO_Pin_13 
		#define USER_IO_A_PORT				GPIOC
		#define USER_IO_A_ADC_CH			ADC_CH2

		#define USER_IO_B_PIN				GPIO_Pin_12 
		#define USER_IO_B_PORT				GPIOC
		#define USER_IO_B_ADC_CH			ADC_CH3

		#define USER_IO_C_PIN				GPIO_Pin_9 
		#define USER_IO_C_PORT				GPIOC
		#define USER_IO_C_ADC_CH			ADC_CH6

		#define USER_IO_D_PIN				GPIO_Pin_8 
		#define USER_IO_D_PORT				GPIOC
		#define USER_IO_D_ADC_CH			ADC_CH7
    #elif (DEVICE_BOARD_NAME == WIZ752SR_12x) 
		#define USER_IO_A_PIN				GPIO_Pin_15
		#define USER_IO_A_PORT				GPIOC
		#define USER_IO_A_ADC_CH			ADC_CH0

		#define USER_IO_B_PIN				GPIO_Pin_14
		#define USER_IO_B_PORT				GPIOC
		#define USER_IO_B_ADC_CH			ADC_CH1

		#define USER_IO_C_PIN				GPIO_Pin_13
		#define USER_IO_C_PORT				GPIOC
		#define USER_IO_C_ADC_CH			ADC_CH2

		#define USER_IO_D_PIN				GPIO_Pin_12
		#define USER_IO_D_PORT				GPIOC
		#define USER_IO_D_ADC_CH			ADC_CH3
	#endif

#endif


// Status LEDs define
#if ((DEVICE_BOARD_NAME == WIZ750SR) || (DEVICE_BOARD_NAME == W7500P_S2E))

	#define LED1_PIN			GPIO_Pin_2 
    #define LED1_GPIO_PORT		GPIOB
    #define LED1_GPIO_PAD		PAD_PB
    #define LED1_GPIO_PAD_AF	PAD_AF1		// PAD Config - LED used 2nd Function

	#define LED2_PIN			GPIO_Pin_3
	#define LED2_GPIO_PORT		GPIOB
	#define LED2_GPIO_PAD		PAD_PB
	#define LED2_GPIO_PAD_AF	PAD_AF1

#elif (DEVICE_BOARD_NAME == WIZ750SR_1xx) // ##20161031 WIZ750Jr
	
	#define LED1_PIN			GPIO_Pin_8
	#define LED1_GPIO_PORT		GPIOC
	#define LED1_GPIO_PAD		PAD_PC
	#define LED1_GPIO_PAD_AF	PAD_AF1		// PAD Config - LED used 2nd Function

	#define LED2_PIN			GPIO_Pin_9
	#define LED2_GPIO_PORT		GPIOC
	#define LED2_GPIO_PAD		PAD_PC
	#define LED2_GPIO_PAD_AF	PAD_AF1
	
#elif (DEVICE_BOARD_NAME == WIZwiki_W7500ECO)

	#define LED1_PIN			GPIO_Pin_1
	#define LED1_GPIO_PORT		GPIOA
	#define LED1_GPIO_PAD		PAD_PA
	#define LED1_GPIO_PAD_AF	PAD_AF1		// PAD Config - LED used 2nd Function

	#define LED2_PIN			GPIO_Pin_2
	#define LED2_GPIO_PORT		GPIOA
	#define LED2_GPIO_PAD		PAD_PA
	#define LED2_GPIO_PAD_AF	PAD_AF1

#elif (DEVICE_BOARD_NAME == WIZ752SR_12x)

	#define LED1_PIN			GPIO_Pin_8 
	#define LED1_GPIO_PORT		GPIOC
	#define LED1_GPIO_PAD		PAD_PC
	#define LED1_GPIO_PAD_AF	PAD_AF1

	#define LED2_PIN			GPIO_Pin_9 
	#define LED2_GPIO_PORT		GPIOC
	#define LED2_GPIO_PAD		PAD_PC
	#define LED2_GPIO_PAD_AF	PAD_AF1

#elif (DEVICE_BOARD_NAME == WIZwiki_W7500) // WIZwiki-W7500 board

	// [RGB LED] R: PC_08, G: PC_09, B: PC_05
	#define LED1_PIN			GPIO_Pin_8 // RGB LED: RED
	#define LED1_GPIO_PORT		GPIOC
	#define LED1_GPIO_PAD		PAD_PC
	#define LED1_GPIO_PAD_AF	PAD_AF1

	#define LED2_PIN			GPIO_Pin_9 // RGB LED: GREEN
	#define LED2_GPIO_PORT		GPIOC
	#define LED2_GPIO_PAD		PAD_PC
	#define LED2_GPIO_PAD_AF	PAD_AF1
	
#endif
	
	// LED
	#define LEDn		2
	typedef enum
	{
		LED1 = 0,	// PHY link status
		LED2 = 1	// TCP connection status
	} Led_TypeDef;

	extern volatile uint16_t phylink_check_time_msec;
	extern uint8_t flag_hw_trig_enable;
	
	void W7500x_Board_Init(void);
	void Supervisory_IC_Init(void);
	
	void init_hw_trig_pin(void);
	uint8_t get_hw_trig_pin(void);
	
	void init_phylink(void);
	uint8_t get_phylink(void);
	
	void init_uart_if_sel(void);
	uint8_t get_uart_if_sel(void);
	
	void init_boot_entry_pin(void);
	uint8_t get_boot_entry_pin(void);
	
	void LED_Init(Led_TypeDef Led);
	void LED_On(Led_TypeDef Led);
	void LED_Off(Led_TypeDef Led);
	void LED_Toggle(Led_TypeDef Led);
	uint8_t get_LED_Status(Led_TypeDef Led);
	
#endif


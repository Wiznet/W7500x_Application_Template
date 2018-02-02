#ifndef TIMERHANDLER_H_
#define TIMERHANDLER_H_

#include <stdint.h>

void Timer0_Configuration(void);
void Timer0_IRQ_Handler(void);

void Timer1_Configuration(void);
void Timer1_IRQ_Handler(void);

uint32_t getDeviceUptime_hour(void);
uint8_t  getDeviceUptime_min(void);
uint8_t  getDeviceUptime_sec(void);
uint16_t getDeviceUptime_msec(void);

uint32_t getNow(void);
uint32_t getDevtime(void);
void setDevtime(uint32_t timeval_sec);
uint32_t millis(void);

void set_phylink_time_check(uint8_t enable);
uint32_t get_phylink_downtime(void);

#endif /* TIMERHANDLER_H_ */

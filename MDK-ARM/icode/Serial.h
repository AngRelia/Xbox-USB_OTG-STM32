#ifndef __SERIAL_H
#define __SERIAL_H

#include "main.h"

/*--------USART2 相关变量---------*/
extern char Serial2_RxPacket[500];
extern uint8_t Serial2_RxFlag;
extern uint8_t RxByte2;


/*--------USART2 函数---------*/
void Serial2_SendByte(uint8_t byte);
void Serial2_SendArray(uint8_t *array, uint16_t length);
void Serial2_SendString(char *str);
void Serial2_SendNumber(uint32_t num, uint8_t length);
void Serial2_Printf(char *format, ...);

void Serial2_RxHandler(void);
#endif

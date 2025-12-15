#include "Serial.h"
#include "main.h"
#include "stm32f4xx_hal.h" // 关键修正：必须包含此头文件以识别 HAL 库定义
#include <stdio.h>
#include <stdarg.h>
#include <string.h>


//usart2
char Serial2_RxPacket[500];
uint8_t Serial2_RxFlag;

// 外部引用 CubeMX 生成的串口句柄
extern UART_HandleTypeDef huart2;

// 接收相关变量
uint8_t RxByte2;

/*--------USART2 发送字节---------*/
void Serial2_SendByte(uint8_t byte)
{
    // 使用 HAL 库发送单个字节
    HAL_UART_Transmit(&huart2, &byte, 1, HAL_MAX_DELAY);
}


/*--------USART2 发送数组---------*/
void Serial2_SendArray(uint8_t *array, uint16_t length)
{
    HAL_UART_Transmit(&huart2, array, length, HAL_MAX_DELAY);
}


/*--------USART2 发送字符串---------*/
void Serial2_SendString(char *str)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
}


/*--------计算幂运算---------*/
uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
    uint32_t result = 1;
    while (Y--)
        result *= X;
    return result;
}

/*--------USART2 发送数字---------*/
void Serial2_SendNumber(uint32_t num, uint8_t length)
{
    for (uint8_t i = 0; i < length; i++)
    {
        uint8_t digit = (num / Serial_Pow(10, length - i - 1)) % 10;
        Serial2_SendByte(digit + '0');
    }
}


/*--------USART2 打印输出 (printf格式)---------*/
void Serial2_Printf(char *format, ...)
{
    // 修正：使用 static 避免堆栈溢出（Stack Overflow）
    // 如果栈空间较小(默认0x400)，分配500字节的大数组可能导致程序直接跑飞
    static char buffer_Serial2[500]; 
    
    va_list args;
    va_start(args, format);
    vsprintf(buffer_Serial2, format, args);
    va_end(args);
    
    Serial2_SendString(buffer_Serial2);
}

/*==========================================================
 * 各串口独立处理函数
 *==========================================================*/

/**
 * @brief USART2 接收处理逻辑
 */
void Serial2_RxHandler(void)
{
    static uint8_t RxState2 = 0;
    static uint8_t pRxPacket2 = 0;

    if (RxState2 == 0)
    {
        // 等待包头 '@'
        if (RxByte2 == '@' && Serial2_RxFlag == 0)
        {
            RxState2 = 1;
            pRxPacket2 = 0;
        }
    }
    else if (RxState2 == 1)
    {
        // 接收数据，直到遇到 '\r'
        if (RxByte2 == '\r')
        {
            RxState2 = 2;
        }
        else
        {
            // 防止缓冲区溢出
            if(pRxPacket2 < 499) 
            {
                Serial2_RxPacket[pRxPacket2++] = RxByte2;
            }
        }
    }
    else if (RxState2 == 2)
    {
        // 等待包尾 '\n'
        if (RxByte2 == '\n')
        {
            RxState2 = 0;
            Serial2_RxPacket[pRxPacket2] = '\0';
            Serial2_RxFlag = 1;
        }
    }

    // 重新开启中断接收
    HAL_UART_Receive_IT(&huart2, &RxByte2, 1); 
}
#ifndef __KEY_H
#define __KEY_H

#include "main.h"


uint8_t Key_GetNum(void);     // 获取一次按键值（无按键返回0）
void Key_Tick(void);          // 定时器1ms调用一次

#endif

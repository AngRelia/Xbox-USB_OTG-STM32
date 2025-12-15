#ifndef __USBH_XBOX_H
#define __USBH_XBOX_H

#include "usbh_core.h"
#include "main.h"
// -------------------------------------------------------
// 1. Xbox 360 / XInput 20字节数据包结构
// -------------------------------------------------------
// ARMCC V5 要求：如果外层是 packed，内部所有的 struct/union 都必须显式 packed
typedef struct __attribute__((packed)) {
    uint8_t MsgType;      // 0x00: 状态报告
    uint8_t PacketSize;   // 0x14: 20字节
    
    union {
        uint16_t ButtonMask;
        struct __attribute__((packed)) { 
            uint8_t DpadUp : 1;
            uint8_t DpadDown : 1;
            uint8_t DpadLeft : 1;
            uint8_t DpadRight : 1;
            uint8_t Start : 1;
            uint8_t Back : 1;
            uint8_t LeftStickClick : 1;
            uint8_t RightStickClick : 1;
            
            uint8_t LB : 1;
            uint8_t RB : 1;
            uint8_t XboxGuide : 1; // 西瓜键
            uint8_t Unused : 1;
            uint8_t A : 1;
            uint8_t B : 1;
            uint8_t X : 1;
            uint8_t Y : 1;
        } bits;
    } __attribute__((packed)) Buttons; // <--- 关键修复：给 union 加上 packed 属性

    uint8_t LeftTrigger;  // 0-255
    uint8_t RightTrigger; // 0-255

    int16_t LeftStickX;   // 摇杆数据
    int16_t LeftStickY;
    int16_t RightStickX;
    int16_t RightStickY;

    uint8_t Reserved[6];  // 填充位
} XBOX_Report_t;

// -------------------------------------------------------
// 2. 驱动内部句柄定义
// -------------------------------------------------------
typedef enum {
  XBOX_IDLE,
  XBOX_GET_DATA,
  XBOX_POLL,
} XBOX_StateTypeDef;

typedef struct {
  XBOX_StateTypeDef state;
  XBOX_Report_t     Report;
  uint8_t           DataReady;
  
  uint8_t           InPipe; 
  uint8_t           OutPipe;
  uint8_t           InEp;
  uint8_t           OutEp;
  uint16_t          InEpSize;
  uint16_t          OutEpSize;
  uint8_t           buff[32]; // 接收缓冲区
} XBOX_HandleTypeDef;

// -------------------------------------------------------
// 3. 导出给 main.c 使用的接口
// -------------------------------------------------------
extern USBH_ClassTypeDef  XBOX_Class;
#define USBH_XBOX_CLASS   &XBOX_Class

XBOX_Report_t* USBH_XBOX_GetReport(USBH_HandleTypeDef *phost);

#endif
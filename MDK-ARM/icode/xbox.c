#include "xbox.h"

// XInput 设备特征：Class 0xFF, SubClass 0x5D, Protocol 0x01
#define XBOX_INTERFACE_CLASS    0xFF
#define XBOX_INTERFACE_SUBCLASS 0x5D 
#define XBOX_INTERFACE_PROTOCOL 0x01

static USBH_StatusTypeDef USBH_XBOX_InterfaceInit(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_XBOX_InterfaceDeInit(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_XBOX_ClassRequest(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_XBOX_Process(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_XBOX_SOFProcess(USBH_HandleTypeDef *phost);

XBOX_HandleTypeDef xbox_handle;

// 定义 USB Host 类操作表
USBH_ClassTypeDef XBOX_Class = {
  "XBOX_HOST",
  XBOX_INTERFACE_CLASS,
  USBH_XBOX_InterfaceInit,
  USBH_XBOX_InterfaceDeInit,
  USBH_XBOX_ClassRequest,
  USBH_XBOX_Process,
  USBH_XBOX_SOFProcess,
  NULL,
};

// 1. 接口初始化：寻找符合 XInput 特征的接口
static USBH_StatusTypeDef USBH_XBOX_InterfaceInit(USBH_HandleTypeDef *phost) {
  USBH_StatusTypeDef status = USBH_OK;
  uint8_t interface = 0xFF;
  
  // -----------------------------------------------------------
  // 改进：手动遍历所有接口，寻找 Class = 0xFF (Vendor Specific)
  // -----------------------------------------------------------
  USBH_DbgLog("XBOX: Scanning Interfaces...");
  
  for (int i = 0; i < phost->device.CfgDesc.bNumInterfaces; i++) {
      if (phost->device.CfgDesc.Itf_Desc[i].bInterfaceClass == 0xFF) {
          interface = i;
          USBH_DbgLog("XBOX: Found Interface #%d (Class 0xFF)", i);
          break;
      }
  }

  if (interface == 0xFF) {
    USBH_DbgLog("XBOX: Cannot Find XInput Interface!");
    return USBH_FAIL;
  }

  USBH_SelectInterface(phost, interface);
  
  // 查找输入/输出端点
  phost->pActiveClass->pData = &xbox_handle;
  memset(&xbox_handle, 0, sizeof(XBOX_HandleTypeDef));

  if(phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[0].bEndpointAddress & 0x80) {
    xbox_handle.InEp = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[0].bEndpointAddress;
    xbox_handle.InEpSize = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[0].wMaxPacketSize;
  } else {
    xbox_handle.OutEp = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[0].bEndpointAddress;
    xbox_handle.OutEpSize = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[0].wMaxPacketSize;
  }

  if(phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[1].bEndpointAddress & 0x80) {
    xbox_handle.InEp = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[1].bEndpointAddress;
    xbox_handle.InEpSize = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[1].wMaxPacketSize;
  } else {
    xbox_handle.OutEp = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[1].bEndpointAddress;
    xbox_handle.OutEpSize = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[1].wMaxPacketSize;
  }

  // 分配并打开管道
  xbox_handle.InPipe = USBH_AllocPipe(phost, xbox_handle.InEp);
  xbox_handle.OutPipe = USBH_AllocPipe(phost, xbox_handle.OutEp);

  USBH_OpenPipe(phost, xbox_handle.InPipe, xbox_handle.InEp, phost->device.address, phost->device.speed, USB_EP_TYPE_INTR, xbox_handle.InEpSize);
  USBH_OpenPipe(phost, xbox_handle.OutPipe, xbox_handle.OutEp, phost->device.address, phost->device.speed, USB_EP_TYPE_INTR, xbox_handle.OutEpSize);

  USBH_LL_SetToggle(phost, xbox_handle.InPipe, 0);
  USBH_LL_SetToggle(phost, xbox_handle.OutPipe, 0);

  xbox_handle.state = XBOX_GET_DATA; // 初始化完成后直接开始请求数据
  return status;
}

// 2. 接口去初始化
static USBH_StatusTypeDef USBH_XBOX_InterfaceDeInit(USBH_HandleTypeDef *phost) {
  if (xbox_handle.InPipe != 0) {
    USBH_ClosePipe(phost, xbox_handle.InPipe);
    USBH_FreePipe(phost, xbox_handle.InPipe);
    xbox_handle.InPipe = 0;
  }
  if (xbox_handle.OutPipe != 0) {
    USBH_ClosePipe(phost, xbox_handle.OutPipe);
    USBH_FreePipe(phost, xbox_handle.OutPipe);
    xbox_handle.OutPipe = 0;
  }
  xbox_handle.state = XBOX_IDLE;
  return USBH_OK;
}

// 3. 类请求 (XInput 不需要复杂的 SetProtocol)
static USBH_StatusTypeDef USBH_XBOX_ClassRequest(USBH_HandleTypeDef *phost) {
  return USBH_OK;
}

// 4. 核心处理循环 (轮询数据)
static USBH_StatusTypeDef USBH_XBOX_Process(USBH_HandleTypeDef *phost) {
  USBH_StatusTypeDef status = USBH_OK;
  USBH_URBStateTypeDef urb_status;

  switch (xbox_handle.state) {
  case XBOX_GET_DATA:
    // 发起一次中断传输请求
    USBH_InterruptReceiveData(phost, xbox_handle.buff, xbox_handle.InEpSize, xbox_handle.InPipe);
    xbox_handle.state = XBOX_POLL;
    break;

  case XBOX_POLL:
    // 查询传输结果
    urb_status = USBH_LL_GetURBState(phost, xbox_handle.InPipe);
    
    if (urb_status == USBH_URB_DONE) {
      // 成功接收到数据！
      // 校验包头 00 14 (标准20字节包)
      if (xbox_handle.buff[0] == 0x00 && xbox_handle.buff[1] == 0x14) {
          // 拷贝数据到 Report 结构体
          memcpy(&xbox_handle.Report, xbox_handle.buff, 20);
          xbox_handle.DataReady = 1;
      }
      xbox_handle.state = XBOX_GET_DATA; // 立即准备下一次接收
    } 
    else if (urb_status == USBH_URB_NOTREADY) {
      // 设备忙，重试
      xbox_handle.state = XBOX_GET_DATA; 
    }
    else if (urb_status == USBH_URB_STALL || urb_status == USBH_URB_ERROR) {
      // 错误恢复
      USBH_ClrFeature(phost, xbox_handle.InEp);
      xbox_handle.state = XBOX_GET_DATA;
    }
    break;

  default:
    xbox_handle.state = XBOX_GET_DATA;
    break;
  }
  return status;
}

static USBH_StatusTypeDef USBH_XBOX_SOFProcess(USBH_HandleTypeDef *phost) {
  return USBH_OK;
}

// 供用户调用的 API
XBOX_Report_t* USBH_XBOX_GetReport(USBH_HandleTypeDef *phost) {
    if(phost->gState == HOST_CLASS) {
        return &xbox_handle.Report;
    }
    return NULL;
}
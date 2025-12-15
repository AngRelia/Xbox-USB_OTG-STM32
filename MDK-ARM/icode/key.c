#include "key.h"

static uint8_t Key_Num = 0;   // 保存最终的按键值



static uint8_t Key_GetState(void)
{
    if (HAL_GPIO_ReadPin(KEY_1_GPIO_Port, KEY_1_Pin) == GPIO_PIN_RESET) return 1;
    if (HAL_GPIO_ReadPin(KEY_2_GPIO_Port, KEY_2_Pin) == GPIO_PIN_RESET) return 2;
    if (HAL_GPIO_ReadPin(KEY_3_GPIO_Port, KEY_3_Pin) == GPIO_PIN_RESET) return 3;
	if (HAL_GPIO_ReadPin(KEY_4_GPIO_Port, KEY_4_Pin) == GPIO_PIN_RESET) return 4;
    return 0;
}

uint8_t Key_GetNum(void)
{
    uint8_t temp = Key_Num;
    Key_Num = 0;   // 取走后清零，保证一次只响应一次
    return temp;
}

void Key_Tick(void)
{
    static uint8_t Count = 0;
    static uint8_t CurrState = 0, PrevState = 0;

    Count++;
    if (Count >= 20)   // 20ms消抖
    {
        Count = 0;
        PrevState = CurrState;
        CurrState = Key_GetState();

        // 松手时触发
        if (CurrState == 0 && PrevState != 0)
        {
            Key_Num = PrevState;
        }
    }
}

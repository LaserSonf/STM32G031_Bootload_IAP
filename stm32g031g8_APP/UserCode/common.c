#include "common.h"
#include "flash_config.h"

uint32_t uart2_rx_tick = 0;
void uart2_rx_handle(void)
{
    uint8_t flash_status = 0;
    uint8_t i = 0;

    if ((HAL_GetTick() - uart2_rx_tick) > 20)
    {
        if(HAL_UART_Receive(&UartHandle, Rx_Buf, Rx_len, 1000) == HAL_OK)
        {
			// 收到 60 F1 55 55 升级指令
            if(Rx_Buf[0] == CMD_IAP && Rx_Buf[1] == 0xF1 && Rx_Buf[2] == 0x55 && Rx_Buf[3] == 0x55) 
            {
                IAP_updata();
			}
        }
        uart2_rx_tick = HAL_GetTick();
    }
}

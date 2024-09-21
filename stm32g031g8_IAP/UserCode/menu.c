/**
  ******************************************************************************
  * @file    IAP_Main/Src/menu.c 
  * @author  MCD Application Team
  * @brief   This file provides the software which contains the main menu routine.
  *          The main menu gives the options of:
  *             - downloading a new binary file, 
  *             - uploading internal flash memory,
  *             - executing the binary file already loaded 
  *             - configuring the write protection of the Flash sectors where the 
  *               user loads his binary file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/** @addtogroup STM32F1xx_IAP
  * @{
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "menu.h"
#include "usart.h"
#include "string.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
pFunction JumpToApplication;
uint32_t JumpAddress;
uint8_t aFileName[FILE_NAME_LENGTH];

config_data_t Write_Config ={
    .device_name = DEVICE_NAME,
    .FW_vision = FW_VERSION,
    .HW_vision = HW_VERSION,
    .updata_flg = NOT_UPDATA
};

/* Private function prototypes -----------------------------------------------*/
void SerialDownload(void);
void SerialUpload(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Download a file via serial port
  * @param  None
  * @retval None
  */
void SerialDownload(void)
{
  uint8_t number[11] = {0};
  uint32_t size = 0;
  COM_StatusTypeDef result;

  Serial_PutString((uint8_t *)"等待文件发送…(按'A'或者'a'终止)\n\r");
  result = Ymodem_Receive( &size );
  HAL_Delay(100);
  if (result == COM_OK)
  {
	 if (FLASH_Erase(CONFIG_START_ADDRESS) == FLASHIF_OK)
	 {
         Write_Config.FW_vision = Read_Config.FW_vision;
         Write_Config.updata_flg = NOT_UPDATA;
         if (FLASH_If_Write(CONFIG_START_ADDRESS, (uint32_t *)&Write_Config, sizeof(Write_Config)) == FLASHIF_OK)
         {
             Serial_PutString((uint8_t *)"\n\n\r 程序下载完成!\n\r--------------------------------\r\n 文件: ");
             Serial_PutString(aFileName);
             Int2Str(number, size);
             Serial_PutString((uint8_t *)"\n\r 大小: ");
             Serial_PutString(number);
             Serial_PutString((uint8_t *)" 字节\r\n");
             Serial_PutString((uint8_t *)"--------------------------------\n");
		}
	 }else{
		 Serial_PutString((uint8_t *)"Config Erase Flash Err!\n");
	 }
	 
  }
  else if (result == COM_LIMIT)
  {
    Serial_PutString((uint8_t *)"\n\n\r镜像文件大小高于允许的内存空间!\n\r");
  }
  else if (result == COM_DATA)
  {
    Serial_PutString((uint8_t *)"\n\n\r验证失败!\n\r");
  }
  else if (result == COM_ABORT)
  {
    Serial_PutString((uint8_t *)"\r\n\n用户终止.\n\r");
  }
  else
  {
    Serial_PutString((uint8_t *)"\n\r文件接收失败!\n\r");
  }
}

/**
  * @brief  Upload a file via serial port.
  * @param  None
  * @retval None
  */
void SerialUpload(void)
{
  uint8_t status = 0;

  Serial_PutString((uint8_t *)"\n\n\rWaiting to receive file\n\r");

  HAL_UART_Receive(&UartHandle, &status, 1, RX_TIMEOUT);
  if ( status == CRC16)
  {
    /* Transmit the flash image through ymodem protocol */
    status = Ymodem_Transmit((uint8_t*)APPLICATION_ADDRESS, (const uint8_t*)"UploadedFlashImage.bin", USER_FLASH_SIZE);

    if (status != 0)
    {
      Serial_PutString((uint8_t *)"\n\rAn error occurred while transferring the file\n\r");
    }
    else
    {
      Serial_PutString((uint8_t *)"\n\rFile uploaded successfully! \n\r");
    }
  }
}

/**
  * @brief  Display the Main Menu on HyperTerminal
  * @param  None
  * @retval None
  */
void Main_Menu(void)
{
  uint8_t key = 0;

  Serial_PutString((uint8_t *)"\r\n========================================================");
  Serial_PutString((uint8_t *)"\r\n===================  Updata    =========================");
  Serial_PutString((uint8_t *)"\r\n========================================================");
  Serial_PutString((uint8_t *)"\r\n\r\n");

  while (1)
  {

    Serial_PutString((uint8_t *)"\r\n===================== Main Menu ======================\r\n\n");
    Serial_PutString((uint8_t *)"  Download image to the internal Flash ----------------- 1\r\n\n");
    Serial_PutString((uint8_t *)"  Upload image from the internal Flash ----------------- 2\r\n\n");
    Serial_PutString((uint8_t *)"  Execute the loaded application ----------------------- 3\r\n\n");
    Serial_PutString((uint8_t *)"  Delete application ----------------------------------- 4\r\n\n");
    Serial_PutString((uint8_t *)"========================================================\r\n\n");

    /* Clean the input path */
    __HAL_UART_FLUSH_DRREGISTER(&UartHandle);
	
    /* Receive key */
    HAL_UART_Receive(&UartHandle, &key, 1, RX_TIMEOUT);

    switch (key)
    {
    case '1' :
      /* Download user application in the Flash */
      SerialDownload();

      break;
    case '2' :
      /* Upload user application from the Flash */
      SerialUpload();
      break;
    case '3' :
		JumpToApplication_Funtion();
      break;
    case '4' :
      /* Delete an application from the Flash */
      if(FLASH_Erase(APPLICATION_ADDRESS) == FLASHIF_OK)
			{
				Serial_PutString((uint8_t *)"Delete Success!\r\n\n");
			}
			else
			{
				Serial_PutString((uint8_t *)"Delete Fail!\r\n\n");
			}
      break;
	default:
	Serial_PutString((uint8_t *)"Invalid Number ! ==> The number should be either 1, 2, 3 or 4\r");
	break;
    }
  }
}


void ReadyToUpdate(void)
{
	FLASH_Init();
	Main_Menu();
}

void JumpToApplication_Funtion(void)
{
	if (((*(__IO uint32_t*)APPLICATION_ADDRESS) & 0x2FFE0000 ) == 0x20000000)
	{
		/* Jump to user application */
		JumpAddress = *(__IO uint32_t*) (APPLICATION_ADDRESS + 4);
		JumpToApplication = (pFunction) JumpAddress;
		/* Initialize user application's Stack Pointer */
		__set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);
		JumpToApplication();
	}
	else{
		Serial_PutString((uint8_t *)"No image file is currently available!\r\n\n");
	}
}
/**
  * @}
  */

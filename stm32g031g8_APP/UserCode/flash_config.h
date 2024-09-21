#ifndef FLASH_CONFIG_H
#define FLASH_CONFIG_H

#include "main.h"



#define DEVICE_NAME		"GX01"
#define HW_VERSION		0x01
#define FW_VERSION		0x01
#define UPDATA			0x01
#define NOT_UPDATA		0x00

typedef struct config_data_s
{
	char device_name[10];		//设备名称
	uint8_t HW_vision;	     //硬件版本
	uint8_t FW_vision;		//软件版本
	uint8_t updata_flg;       //更新标志
	uint8_t crc_cal; 		//crc8校验
	
}__attribute__((packed)) config_data_t;
extern config_data_t Config_Write;

typedef enum{
	DOING = 0,
	OK,
	ERR
}config_status;

typedef enum{
	READ = 0,
	ERASE,
	WRITE,
	CHECK,
	END	
}config_step_typedef;

void init_crc8_table(void);
void IAP_updata(void);

#endif
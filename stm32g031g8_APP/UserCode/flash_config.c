#include <stdint.h>
#include <stddef.h>
#include "flash_config.h"
#include "string.h"

config_data_t Read_Config = {0};

config_data_t Config_Default = {
    .device_name = DEVICE_NAME,
    .FW_vision = FW_VERSION,
    .HW_vision = HW_VERSION,
    .updata_flg = NOT_UPDATA};

config_data_t Config_Write = {
    .device_name = DEVICE_NAME,
    .FW_vision = FW_VERSION,
    .HW_vision = HW_VERSION,
    .updata_flg = UPDATA};

// CRC-8 多项式
#define CRC8_POLYNOMIAL 0x07

// CRC-8 表
static uint8_t crc8_table[256];

// 初始化 CRC-8 表
void init_crc8_table(void)
{
    for (uint16_t i = 0; i < 256; i++)
    {
        uint8_t crc = i;
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ CRC8_POLYNOMIAL;
            else
                crc = crc << 1;
        }
        crc8_table[i] = crc;
    }
}

// 计算 CRC-8 值
static uint8_t calculate_crc8(uint8_t *data, size_t length)
{
    uint8_t crc = 0;
    for (size_t i = 0; i < length; i++)
    {
        uint8_t byte = data[i];
        crc = crc8_table[crc ^ byte];
    }
    return crc;
}

static config_status Flash_Config_Read(config_data_t buf)
{
    config_status status = DOING;
    if (STMFLASH_Read(CONFIG_START_ADDRESS, (uint8_t *)&buf, sizeof(buf)) == FLASHIF_OK)
    {
        status = OK;
    }
    return status;
}

static config_status Flash_Config_Erase(void)
{
    config_status status = DOING;
    if (FLASH_Erase(CONFIG_START_ADDRESS) == FLASHIF_OK)
    {
        status = OK;
    }
    return status;
}

static config_status Flash_Config_Write(config_data_t buf)
{
    config_status status = DOING;
    buf.crc_cal = calculate_crc8((uint8_t *)&buf, sizeof(buf) - 1); // crc只校验前面的数据
    if (FLASH_If_Write(CONFIG_START_ADDRESS, (uint32_t *)&buf, sizeof(buf)) == FLASHIF_OK)
    {
        status = OK;
    }
    return status;
}

static void Flash_Config_Set_Defalt(void)
{
    uint8_t calculated_crc = 0;
    if (FLASH_Erase(CONFIG_START_ADDRESS) == FLASHIF_OK)
    {
        calculated_crc = calculate_crc8((uint8_t *)&Config_Default, sizeof(Config_Default) - 1);
        Flash_Config_Write(Config_Default);
    }
}

static config_status Flash_Config_Check(config_data_t buf)
{
    uint8_t calculated_crc = 0;
    uint8_t i = 0;
    config_status status = DOING;

    /*读取上一步写入的buf*/
    STMFLASH_Read(CONFIG_START_ADDRESS, (uint8_t *)&buf, sizeof(buf));
    calculated_crc = calculate_crc8((uint8_t *)&buf, sizeof(buf) - 1);

    if (calculated_crc == buf.crc_cal) // CRC 校验通过
    {
        status = OK;
    }
    else
    {
		status = ERR;
        Serial_PutString((uint8_t *)"CRC Erro !\n");
    }
    return status;
}

void IAP_updata(void)
{
    static config_step_typedef config_step = READ;
    while (config_step != END)
    {
        switch (config_step)
        {
        case READ:
            if (Flash_Config_Read(Read_Config) == OK)
            {
                config_step = ERASE;
            }
            break;
        case ERASE:
            if (Flash_Config_Erase() == OK)
            {
                Serial_PutString((uint8_t *)"ERASE ok \n");
                config_step = WRITE;
            }
            break;
        case WRITE:
            if (Flash_Config_Write(Config_Write) == OK)
            {
                config_step = CHECK;
            }
            break;
        case CHECK:
            if (Flash_Config_Check(Config_Write) == OK)
            {
                config_step = END;
            }
            break;

        default:
            Flash_Config_Set_Defalt();
            break;
        }
	}
        if (config_step == END)
        {
            Serial_PutString((uint8_t *)"into bootloader \n");
            config_step = READ;
            MX_IWDG_Init(); // 使用看门狗复位
        }

}
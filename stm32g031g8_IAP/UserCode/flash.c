#include "flash.h"

/**
 * @brief  Unlocks Flash for write access
 * @param  None
 * @retval None
 */
void FLASH_Init(void)
{
  /* Unlock the Program memory */
  HAL_FLASH_Unlock();

  /* Clear all FLASH flags */
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR);
  /* Unlock the Program memory */
  HAL_FLASH_Lock();
}

/**
 * @brief  This function does an erase of all user flash area
 * @param  start: start of user flash area
 * @retval FLASHIF_OK : user flash area successfully erased
 *         FLASHIF_ERASEKO : error occurred
 */
uint32_t FLASH_Erase(uint32_t start)
{
  uint32_t status = FLASHIF_ERASEKO;
  FLASH_EraseInitTypeDef erase_init;
  uint32_t error = 0u;
  erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
  erase_init.Page = (start - FLASH_BASE) / FLASH_PAGE_SIZE;
  erase_init.Banks = FLASH_BANK_1;
  /* Calculate the number of pages from "address" and the end of flash. */
  erase_init.NbPages = (USER_FLASH_END_ADDRESS - start + 1) / FLASH_PAGE_SIZE;
  /* Do the actual erasing. */
  HAL_FLASH_Unlock();
  if (start < FLASH_END_ADDRESS)
  {
    if (HAL_OK == HAL_FLASHEx_Erase(&erase_init, &error))
    {
      status = FLASHIF_OK;
    }
  }
  else
    status = FLASHIF_ERASEKO;
  HAL_FLASH_Lock();

  return status;
}

/* Public functions ---------------------------------------------------------*/
/**
 * @brief  This function writes a data buffer in flash (data are 32-bit aligned).
 * @note   After writing data buffer, the flash content is checked.
 * @param  destination: start address for target location
 * @param  p_source: pointer on buffer with data to write
 * @param  length: length of data buffer (unit is 32-bit word)
 * @retval uint32_t 0: Data successfully written to Flash memory
 *         1: Error occurred while writing data in Flash memory
 *         2: Written Data in flash memory is different from expected one
 */

uint32_t FLASH_If_Write(uint32_t destination, uint32_t *p_source, uint32_t length)
{
  uint32_t status = FLASHIF_OK;
  uint32_t i = 0;

  HAL_FLASH_Unlock();

  for (i = 0; (i < length / 2) && (destination <= (USER_FLASH_END_ADDRESS - 8)); i++)
  {
    /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
       be done by word */
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, destination, *((uint64_t *)(p_source + 2 * i))) == HAL_OK)
    {
      /* Check the written value */
      if (*(uint64_t *)destination != *(uint64_t *)(p_source + 2 * i))
      {
        /* Flash content doesn't match SRAM content */
        status = FLASHIF_WRITINGCTRL_ERROR;
        break;
      }
      /* Increment FLASH destination address */
      destination += 8;
    }
    else
    {
      /* Error occurred while writing data in Flash memory */
      status = FLASHIF_WRITING_ERROR;
      break;
    }
  }
  HAL_FLASH_Lock();

  return status;
}

/**
  * @brief  Returns the write protection status of application flash area.
  * @param  None
  * @retval If a sector in application area is write-protected returned value is a combinaison
            of the possible values : FLASHIF_PROTECTION_WRPENABLED, FLASHIF_PROTECTION_PCROPENABLED, ...
  *         If no sector is write-protected FLASHIF_PROTECTION_NONE is returned.
  */
uint32_t FLASH_If_GetWriteProtectionStatus(void)
{
  uint32_t ProtectedPAGE = FLASHIF_PROTECTION_NONE;
  FLASH_OBProgramInitTypeDef OptionsBytesStruct;

  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();

  /* Check if there are write protected sectors inside the user flash area ****/
  HAL_FLASHEx_OBGetConfig(&OptionsBytesStruct);

  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();

  /* Get pages already write protected ****************************************/
  //  ProtectedPAGE = ~(OptionsBytesStruct.WRPPage) & FLASH_PAGE_TO_BE_PROTECTED;

  /* Check if desired pages are already write protected ***********************/
  if (ProtectedPAGE != 0)
  {
    /* Some sectors inside the user flash area are write protected */
    return FLASHIF_PROTECTION_WRPENABLED;
  }
  else
  {
    /* No write protected sectors inside the user flash area */
    return FLASHIF_PROTECTION_NONE;
  }
}

/**
 * @brief  Configure the write protection status of user flash area.
 * @param  protectionstate : FLASHIF_WRP_DISABLE or FLASHIF_WRP_ENABLE the protection
 * @retval uint32_t FLASHIF_OK if change is applied.
 */
uint32_t FLASH_If_WriteProtectionConfig(uint32_t protectionstate)
{
  FLASH_OBProgramInitTypeDef config_new, config_old;
  HAL_StatusTypeDef result;

  HAL_FLASH_Unlock();
  HAL_FLASH_OB_Unlock();

  config_old.WRPArea = OB_WRPAREA_ZONE_A;
  HAL_FLASHEx_OBGetConfig(&config_old);

  if (protectionstate == FLASHIF_WRP_ENABLE)
  {
    /* We want to modify only the Write protection */
    config_new.OptionType = OPTIONBYTE_WRP;

    /* No read protection, keep BOR and reset settings */
    // config_new.RDPLevel = OB_RDP_LEVEL_0;
    config_new.USERConfig = config_old.USERConfig;

    // as the G0 here only contains one bank hence we just need to modify the WRP area 1/2(as it contains only one bank hence we can
    // protect the areas from the application start address till the flash end address) with the
    config_new.WRPArea = OB_WRPAREA_ZONE_A;

    config_new.WRPStartOffset = START_0x08008000;
    config_new.WRPEndOffset = END_OF_RAM;
  }
  else
  {
    /* We want to modify only the Write protection */
    config_new.OptionType = OPTIONBYTE_WRP;

    /* No read protection, keep BOR and reset settings */
    config_new.USERConfig = config_old.USERConfig;
    config_new.WRPArea = OB_WRPAREA_ZONE_A;

    config_new.WRPStartOffset = END_OF_RAM;
    config_new.WRPEndOffset = START_OF_RAM;
  }

  result = HAL_FLASHEx_OBProgram(&config_new);

  return (result == HAL_OK ? FLASHIF_OK : FLASHIF_PROTECTION_ERRROR);
}

static uint8_t stmflash_read_byte(uint32_t faddr)
{

  return *(volatile uint8_t *)faddr;
}

uint32_t STMFLASH_Read_Word(uint32_t ReadAddr, uint32_t *pBuffer, uint32_t NumToRead) // 连续读取
{
  uint32_t i;

  if (NumToRead == 0) // 数据长度为0时，直接返回
    goto ERROR;

  for (i = 0; i < NumToRead; i++)
  {
    pBuffer[i] = stmflash_read_byte(ReadAddr); // 读取4个字节.
    ReadAddr += 4;                             // 偏移4个字节.
  }
  return FLASHIF_OK;

ERROR:
  return FLASHIF_READ_ERROR;
}

uint32_t STMFLASH_Read(uint32_t ReadAddr, uint8_t *pBuffer, uint8_t len) 
{
  uint32_t i;

  if (len == 0) // 数据长度为0时，直接返回
    goto ERROR;

  for (i = 0; i < len; i++)
  {
    pBuffer[i] = stmflash_read_byte(ReadAddr); // 读取4个字节.
    ReadAddr += 1;                             // 偏移1个字节.
  }
  return FLASHIF_OK;

ERROR:
  return FLASHIF_READ_ERROR;
}


/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-27     SummerGift   add spi flash port file
 */

#include <rtthread.h>
#include "spi_flash.h"
#include "spi_flash_sfud.h"
#include "drv_spi.h"


#if defined(BSP_USING_SPI_FLASH)


static int rt_hw_spi_flash_init(void)
{
    __HAL_RCC_GPIOF_CLK_ENABLE();
    rt_hw_spi_device_attach("spi5", "EXFLASH", GPIOF, GPIO_PIN_6);

    if (RT_NULL == rt_sfud_flash_probe("W25Q128", "EXFLASH"))
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}
INIT_COMPONENT_EXPORT(rt_hw_spi_flash_init);

#if defined(PKG_USING_FAL)

#include "fal_cfg.h"
#include "fal_def.h"

static int fal_exflash_read(long offset, rt_uint8_t *buf, size_t size)
{
    return 0;//stm32_flash_read(stm32_onchip_flash_16k.addr + offset, buf, size);
}
static int fal_exflash_write(long offset, const rt_uint8_t *buf, size_t size)
{
    return 0;//stm32_flash_read(stm32_onchip_flash_64k.addr + offset, buf, size);
}
static int fal_exflash_erase(long offset, size_t size)
{
    return 0;//stm32_flash_read(stm32_onchip_flash_128k.addr + offset, buf, size);
}

const struct fal_flash_dev stm32_exchip_flash_16M = \
	{ "exchip_flash_16M", STM32_EXFLASH_START_ADRESS_16M, FLASH_SIZE_GRANULARITY_16M, (16 * 1024), {NULL, fal_exflash_read, fal_exflash_write, fal_exflash_erase} };

#endif




#endif


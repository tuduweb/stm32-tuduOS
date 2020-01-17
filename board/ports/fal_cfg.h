/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-12-5      SummerGift   first version
 */

#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#include <rtthread.h>
#include <board.h>

#define FLASH_SIZE_GRANULARITY_16K   (4 * 16 * 1024)
#define FLASH_SIZE_GRANULARITY_64K   (4  * 64 * 1024)
#define FLASH_SIZE_GRANULARITY_128K  (7 * 128 * 1024)

#define STM32_FLASH_START_ADRESS_16K  STM32_FLASH_START_ADRESS
#define STM32_FLASH_START_ADRESS_64K  (STM32_FLASH_START_ADRESS_16K + FLASH_SIZE_GRANULARITY_16K)
#define STM32_FLASH_START_ADRESS_128K (STM32_FLASH_START_ADRESS_64K + FLASH_SIZE_GRANULARITY_64K)


#define STM32_EXFLASH_START_ADRESS_16M  ((uint32_t)0x08000000)
#define FLASH_SIZE_GRANULARITY_16M   (4 * 16 * 1024 * 1024)


#define FLASH_SIZE_GRANULARITY_1M  (4 * 1024 * 1024)
#define STM32_FLASH_START_ADRESS_1M STM32_FLASH_START_ADRESS

extern const struct fal_flash_dev stm32_onchip_flash;


extern const struct fal_flash_dev stm32_onchip_flash_16k;
extern const struct fal_flash_dev stm32_onchip_flash_64k;
extern const struct fal_flash_dev stm32_onchip_flash_128k;


extern const struct fal_flash_dev stm32_exchip_flash_16M;


extern struct fal_flash_dev nor_flash0;

/* flash device table */
#define FAL_FLASH_DEV_TABLE                                          \
{                                                                    \
    &stm32_onchip_flash,                                         \
		&nor_flash0,																									\
}
    //&stm32_onchip_flash_16k,                                         \
    //&stm32_onchip_flash_64k,                                         \
    //&stm32_onchip_flash_128k,                                        \
	////&stm32_exchip_flash_16M,																				 \

/* ====================== Partition Configuration ========================== */
#ifdef FAL_PART_HAS_TABLE_CFG

/* partition table */
#define FAL_PART_TABLE                                                                                                     \
{                                                                                                                          \
    {FAL_PART_MAGIC_WROD, "bootloader", "onchip_flash",  0 , FLASH_SIZE_GRANULARITY_16K , 0}, \
    {FAL_PART_MAGIC_WROD, "param",      "onchip_flash",  0+FLASH_SIZE_GRANULARITY_16K , FLASH_SIZE_GRANULARITY_64K , 0}, \
    {FAL_PART_MAGIC_WROD, "app",        "onchip_flash", 0+FLASH_SIZE_GRANULARITY_16K+FLASH_SIZE_GRANULARITY_64K  , FLASH_SIZE_GRANULARITY_128K, 0}, \
    {FAL_PART_MAGIC_WROD, "exchip0", "exchip_flash",  0 , 8 * 1024 * 1024 , 0}, \
    {FAL_PART_MAGIC_WROD, "exchip1", "exchip_flash",  8 * 1024 * 1024 , 8 * 1024 * 1024 , 0}, \
}
//    {FAL_PART_MAGIC_WROD, "app",        "onchip_flash", STM32_FLASH_START_ADRESS_128K  , FLASH_SIZE_GRANULARITY_128K, 0}, \

    // {FAL_PART_MAGIC_WROD, "bootloader", "onchip_flash_16k",  0 , FLASH_SIZE_GRANULARITY_16K , 0}, \
    // {FAL_PART_MAGIC_WROD, "param",      "onchip_flash_64k",  0 , FLASH_SIZE_GRANULARITY_64K , 0}, \
    // {FAL_PART_MAGIC_WROD, "app",        "onchip_flash_128k", 0 , FLASH_SIZE_GRANULARITY_128K, 0}, \
    //{FAL_PART_MAGIC_WROD, "exchip",     "exchip_flash_16M", 0 , FLASH_SIZE_GRANULARITY_16M, 0}, \

#endif /* FAL_PART_HAS_TABLE_CFG */
#endif /* _FAL_CFG_H_ */

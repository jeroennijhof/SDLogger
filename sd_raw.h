/*
 * Copyright (c) 2006-2012 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 *
 * Modified 2012 by Jeroen Nijhof <jeroen@jeroennijhof.nl>
 */

#ifndef SD_RAW_H
#define SD_RAW_H

#include <stdint.h>
#include "sd_raw_config.h"

/**
 * \addtogroup sd_raw
 *
 * @{
 */
/**
 * \file
 * MMC/SD/SDHC raw access header (license: GPLv2 or LGPLv2.1)
 *
 * \author Roland Riegel
 */

/**
 * The card's layout is harddisk-like, which means it contains
 * a master boot record with a partition table.
 */
#define SD_RAW_FORMAT_HARDDISK 0
/**
 * The card contains a single filesystem and no partition table.
 */
#define SD_RAW_FORMAT_SUPERFLOPPY 1
/**
 * The card's layout follows the Universal File Format.
 */
#define SD_RAW_FORMAT_UNIVERSAL 2
/**
 * The card's layout is unknown.
 */
#define SD_RAW_FORMAT_UNKNOWN 3

uint8_t sd_raw_init();
uint8_t sd_raw_read(offset_t offset, uint8_t* buffer, uintptr_t length);
uint8_t sd_raw_write(offset_t offset, const uint8_t* buffer, uintptr_t length);

/**
 * @}
 */

#endif


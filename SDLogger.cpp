/*
 * Copyright (c) 2012 by Jeroen Nijhof <jeroen@jeroennijhof.nl>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 *
 */

#include "SDLogger.h"

SDLogger::SDLogger() {
}

uint8_t SDLogger::init() {
  /* setup pins for the microSD shield */
  pinMode(SS, OUTPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(MISO, INPUT);
  pinMode(SCK, OUTPUT);
  pinMode(10, OUTPUT);

  /* init SD */
  if (!sd_raw_init())
    return 0;

  return 1;
}

uint32_t SDLogger::size() {
  uint32_t len = 0;

  /* read logfile size, first 4 bytes of first block */
  if (sd_raw_read(0, (uint8_t *)&len, 4))
    return len;

  return 0;
}

uint8_t SDLogger::format() {
  uint32_t len = 0;
  uint32_t bk = 0;
  uint8_t zero = 0x00;

  len = size();
  if (len < 4096)
    len = 4096;
  for (bk = 512; bk < len; bk+=512) {
    if(!sd_raw_write(bk, (uint8_t *)&zero, 1))
      return 0;
  }
  
  /* set logfile size to 0, first 4 bytes of first block */
  len = 0;
  return sd_raw_write(0, (uint8_t *)&len, 4);
}

uint8_t SDLogger::log(char *msg) {
  uint32_t len = 0;

  len = size();
  len += 512;

  /* write msg to next available 512 bytes block */
  if (!sd_raw_write(len, (uint8_t *)msg, strlen(msg)+1))
    return 0;

  /* set new logfile size, first 4 bytes of first block */
  if (!sd_raw_write(0, (uint8_t *)&len, 4))
    return 0;

  return 1;
}


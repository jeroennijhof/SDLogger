/*
 * Copyright (c) 2012 by Jeroen Nijhof <jeroen@jeroennijhof.nl>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 *
 * SD Logger for Arduino with a MicroSD shield
 *
 * Check sd_raw_config.h and SDLogger.h for SS/MOSI/MISO/SCK pins,
 * default SS=D8, MOSI=D11, MISO=D12 and SCK=D13.
 *
 * Make sure you format SD with the function format(). This will
 * remove all partitions on SD and create a new layout.
 * 
 * The layout is pretty simple, the blocks are 512 bytes.
 * The first 4 bytes of the first block contains the log size.
 * Then for every log() a new block is used.
 *
 * If you want to clear the current log just use format().
 *
 * If you want to keep the old log data on the SD and append new 
 * log data just comment out the line with sdlog.format() below.
 *
 * The log data can be retrieved from you SD card with tools like dd:
 *     # dd if=/dev/mmcblk0 of=data.log bs=512 skip=1 count=100
 *     # cat data.log
 */

#include <SDLogger.h> 

SDLogger sdlog = SDLogger();

void setup() {
  /* setup serial */
  Serial.begin(9600);
 
  /* setup SD */
  if (!sdlog.init()) {
    Serial.println("MMC/SD initialization failed.");
  } else {
    Serial.println("MMC/SD initialized.");
  }

  /* Format current SD */
  if (!sdlog.format()) {
    Serial.println("Format failed.");
  }
} 
 
void loop() {
  /* Write message to logfile */
  if (!sdlog.log("This is a test.\n")) {
    Serial.println("Write failed.");
  }

  /* Check current logfile size */
  Serial.println(sdlog.size());

  delay(2000);
} 

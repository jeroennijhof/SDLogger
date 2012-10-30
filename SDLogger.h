/*
 * Copyright (c) 2012 by Jeroen Nijhof <jeroen@jeroennijhof.nl>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 *
 */

#ifndef SDLOGGER_H
#define SDLOGGER_H

#include <Arduino.h>
#include "sd_raw.h"

#define SS    8
#define MOSI 11
#define MISO 12
#define SCK  13


class SDLogger {
  private:
  public:
    SDLogger();
    uint8_t init();
    uint32_t size();
    uint8_t format();
    uint8_t log(char *msg);
};

#endif


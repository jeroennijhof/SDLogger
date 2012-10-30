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

#include <string.h>
#include <avr/io.h>
#include "sd_raw.h"

/**
 * \addtogroup sd_raw MMC/SD/SDHC card raw access
 *
 * This module implements read and write access to MMC, SD
 * and SDHC cards. It serves as a low-level driver for the
 * higher level modules such as partition and file system
 * access.
 *
 * @{
 */
/**
 * \file
 * MMC/SD/SDHC raw access implementation (license: GPLv2 or LGPLv2.1)
 *
 * \author Roland Riegel
 */

/**
 * \addtogroup sd_raw_config MMC/SD configuration
 * Preprocessor defines to configure the MMC/SD support.
 */

/**
 * @}
 */

/* commands available in SPI mode */

/* CMD0: response R1 */
#define CMD_GO_IDLE_STATE 0x00
/* CMD1: response R1 */
#define CMD_SEND_OP_COND 0x01
/* CMD8: response R7 */
#define CMD_SEND_IF_COND 0x08
/* CMD9: response R1 */
#define CMD_SEND_CSD 0x09
/* CMD10: response R1 */
#define CMD_SEND_CID 0x0a
/* CMD12: response R1 */
#define CMD_STOP_TRANSMISSION 0x0c
/* CMD13: response R2 */
#define CMD_SEND_STATUS 0x0d
/* CMD16: arg0[31:0]: block length, response R1 */
#define CMD_SET_BLOCKLEN 0x10
/* CMD17: arg0[31:0]: data address, response R1 */
#define CMD_READ_SINGLE_BLOCK 0x11
/* CMD18: arg0[31:0]: data address, response R1 */
#define CMD_READ_MULTIPLE_BLOCK 0x12
/* CMD24: arg0[31:0]: data address, response R1 */
#define CMD_WRITE_SINGLE_BLOCK 0x18
/* CMD25: arg0[31:0]: data address, response R1 */
#define CMD_WRITE_MULTIPLE_BLOCK 0x19
/* CMD27: response R1 */
#define CMD_PROGRAM_CSD 0x1b
/* CMD28: arg0[31:0]: data address, response R1b */
#define CMD_SET_WRITE_PROT 0x1c
/* CMD29: arg0[31:0]: data address, response R1b */
#define CMD_CLR_WRITE_PROT 0x1d
/* CMD30: arg0[31:0]: write protect data address, response R1 */
#define CMD_SEND_WRITE_PROT 0x1e
/* CMD32: arg0[31:0]: data address, response R1 */
#define CMD_TAG_SECTOR_START 0x20
/* CMD33: arg0[31:0]: data address, response R1 */
#define CMD_TAG_SECTOR_END 0x21
/* CMD34: arg0[31:0]: data address, response R1 */
#define CMD_UNTAG_SECTOR 0x22
/* CMD35: arg0[31:0]: data address, response R1 */
#define CMD_TAG_ERASE_GROUP_START 0x23
/* CMD36: arg0[31:0]: data address, response R1 */
#define CMD_TAG_ERASE_GROUP_END 0x24
/* CMD37: arg0[31:0]: data address, response R1 */
#define CMD_UNTAG_ERASE_GROUP 0x25
/* CMD38: arg0[31:0]: stuff bits, response R1b */
#define CMD_ERASE 0x26
/* ACMD41: arg0[31:0]: OCR contents, response R1 */
#define CMD_SD_SEND_OP_COND 0x29
/* CMD42: arg0[31:0]: stuff bits, response R1b */
#define CMD_LOCK_UNLOCK 0x2a
/* CMD55: arg0[31:0]: stuff bits, response R1 */
#define CMD_APP 0x37
/* CMD58: arg0[31:0]: stuff bits, response R3 */
#define CMD_READ_OCR 0x3a
/* CMD59: arg0[31:1]: stuff bits, arg0[0:0]: crc option, response R1 */
#define CMD_CRC_ON_OFF 0x3b

/* command responses */
/* R1: size 1 byte */
#define R1_IDLE_STATE 0
#define R1_ERASE_RESET 1
#define R1_ILL_COMMAND 2
#define R1_COM_CRC_ERR 3
#define R1_ERASE_SEQ_ERR 4
#define R1_ADDR_ERR 5
#define R1_PARAM_ERR 6
/* R1b: equals R1, additional busy bytes */
/* R2: size 2 bytes */
#define R2_CARD_LOCKED 0
#define R2_WP_ERASE_SKIP 1
#define R2_ERR 2
#define R2_CARD_ERR 3
#define R2_CARD_ECC_FAIL 4
#define R2_WP_VIOLATION 5
#define R2_INVAL_ERASE 6
#define R2_OUT_OF_RANGE 7
#define R2_CSD_OVERWRITE 7
#define R2_IDLE_STATE (R1_IDLE_STATE + 8)
#define R2_ERASE_RESET (R1_ERASE_RESET + 8)
#define R2_ILL_COMMAND (R1_ILL_COMMAND + 8)
#define R2_COM_CRC_ERR (R1_COM_CRC_ERR + 8)
#define R2_ERASE_SEQ_ERR (R1_ERASE_SEQ_ERR + 8)
#define R2_ADDR_ERR (R1_ADDR_ERR + 8)
#define R2_PARAM_ERR (R1_PARAM_ERR + 8)
/* R3: size 5 bytes */
#define R3_OCR_MASK (0xffffffffUL)
#define R3_IDLE_STATE (R1_IDLE_STATE + 32)
#define R3_ERASE_RESET (R1_ERASE_RESET + 32)
#define R3_ILL_COMMAND (R1_ILL_COMMAND + 32)
#define R3_COM_CRC_ERR (R1_COM_CRC_ERR + 32)
#define R3_ERASE_SEQ_ERR (R1_ERASE_SEQ_ERR + 32)
#define R3_ADDR_ERR (R1_ADDR_ERR + 32)
#define R3_PARAM_ERR (R1_PARAM_ERR + 32)
/* Data Response: size 1 byte */
#define DR_STATUS_MASK 0x0e
#define DR_STATUS_ACCEPTED 0x05
#define DR_STATUS_CRC_ERR 0x0a
#define DR_STATUS_WRITE_ERR 0x0c

/* status bits for card types */
#define SD_RAW_SPEC_1 0
#define SD_RAW_SPEC_2 1
#define SD_RAW_SPEC_SDHC 2

/* card type state */
static uint8_t sd_raw_card_type;

/* private helper functions */
static void sd_raw_send_byte(uint8_t b);
static uint8_t sd_raw_rec_byte();
static uint8_t sd_raw_send_command(uint8_t command, uint32_t arg);

/**
 * \ingroup sd_raw
 * Initializes memory card communication.
 *
 * \returns 0 on failure, 1 on success.
 */
uint8_t sd_raw_init() {
    /* enable outputs for MOSI, SCK, SS, input for MISO */
    configure_pin_mosi();
    configure_pin_sck();
    configure_pin_ss();
    configure_pin_miso();

    unselect_card();

    /* initialize SPI with lowest frequency; max. 400kHz during identification mode of card */
    SPCR = (0 << SPIE) | /* SPI Interrupt Enable */
           (1 << SPE)  | /* SPI Enable */
           (0 << DORD) | /* Data Order: MSB first */
           (1 << MSTR) | /* Master mode */
           (0 << CPOL) | /* Clock Polarity: SCK low when idle */
           (0 << CPHA) | /* Clock Phase: sample on rising SCK edge */
           (1 << SPR1) | /* Clock Frequency: f_OSC / 128 */
           (1 << SPR0);
    SPSR &= ~(1 << SPI2X); /* No doubled clock frequency */

    /* initialization procedure */
    sd_raw_card_type = 0;
    
    /* card needs 74 cycles minimum to start up */
    uint16_t i;
    for(i = 0; i < 10; ++i) {
        /* wait 8 clock cycles */
        sd_raw_rec_byte();
    }

    /* address card */
    select_card();

    /* reset card */
    uint8_t response;
    for(i = 0; ; ++i) {
        response = sd_raw_send_command(CMD_GO_IDLE_STATE, 0);
        if(response == (1 << R1_IDLE_STATE))
            break;

        if(i == 0x1ff) {
            unselect_card();
            return 0;
        }
    }

#if SD_RAW_SDHC
    /* check for version of SD card specification */
    response = sd_raw_send_command(CMD_SEND_IF_COND, 0x100 /* 2.7V - 3.6V */ | 0xaa /* test pattern */);
    if((response & (1 << R1_ILL_COMMAND)) == 0) {
        sd_raw_rec_byte();
        sd_raw_rec_byte();
        if((sd_raw_rec_byte() & 0x01) == 0)
            return 0; /* card operation voltage range doesn't match */
        if(sd_raw_rec_byte() != 0xaa)
            return 0; /* wrong test pattern */

        /* card conforms to SD 2 card specification */
        sd_raw_card_type |= (1 << SD_RAW_SPEC_2);
    } else
#endif
    {
        /* determine SD/MMC card type */
        sd_raw_send_command(CMD_APP, 0);
        response = sd_raw_send_command(CMD_SD_SEND_OP_COND, 0);
        if((response & (1 << R1_ILL_COMMAND)) == 0) {
            /* card conforms to SD 1 card specification */
            sd_raw_card_type |= (1 << SD_RAW_SPEC_1);
        } else {
            /* MMC card */
        }
    }

    /* wait for card to get ready */
    for(i = 0; ; ++i) {
        if(sd_raw_card_type & ((1 << SD_RAW_SPEC_1) | (1 << SD_RAW_SPEC_2))) {
            uint32_t arg = 0;
#if SD_RAW_SDHC
            if(sd_raw_card_type & (1 << SD_RAW_SPEC_2))
                arg = 0x40000000;
#endif
            sd_raw_send_command(CMD_APP, 0);
            response = sd_raw_send_command(CMD_SD_SEND_OP_COND, arg);
        } else {
            response = sd_raw_send_command(CMD_SEND_OP_COND, 0);
        }

        if((response & (1 << R1_IDLE_STATE)) == 0)
            break;

        if(i == 0x7fff) {
            unselect_card();
            return 0;
        }
    }

#if SD_RAW_SDHC
    if(sd_raw_card_type & (1 << SD_RAW_SPEC_2)) {
        if(sd_raw_send_command(CMD_READ_OCR, 0)) {
            unselect_card();
            return 0;
        }

        if(sd_raw_rec_byte() & 0x40)
            sd_raw_card_type |= (1 << SD_RAW_SPEC_SDHC);

        sd_raw_rec_byte();
        sd_raw_rec_byte();
        sd_raw_rec_byte();
    }
#endif

    /* set block size to 512 bytes */
    if(sd_raw_send_command(CMD_SET_BLOCKLEN, 512)) {
        unselect_card();
        return 0;
    }

    /* deaddress card */
    unselect_card();

    /* switch to highest SPI frequency possible */
    SPCR &= ~((1 << SPR1) | (1 << SPR0)); /* Clock Frequency: f_OSC / 4 */
    SPSR |= (1 << SPI2X); /* Doubled Clock Frequency: f_OSC / 2 */

    return 1;
}

/**
 * \ingroup sd_raw
 * Sends a raw byte to the memory card.
 *
 * \param[in] b The byte to sent.
 * \see sd_raw_rec_byte
 */
void sd_raw_send_byte(uint8_t b) {
    SPDR = b;
    /* wait for byte to be shifted out */
    while(!(SPSR & (1 << SPIF)));
    SPSR &= ~(1 << SPIF);
}

/**
 * \ingroup sd_raw
 * Receives a raw byte from the memory card.
 *
 * \returns The byte which should be read.
 * \see sd_raw_send_byte
 */
uint8_t sd_raw_rec_byte() {
    /* send dummy data for receiving some */
    SPDR = 0xff;
    while(!(SPSR & (1 << SPIF)));
    SPSR &= ~(1 << SPIF);

    return SPDR;
}

/**
 * \ingroup sd_raw
 * Send a command to the memory card which responses with a R1 response (and possibly others).
 *
 * \param[in] command The command to send.
 * \param[in] arg The argument for command.
 * \returns The command answer.
 */
uint8_t sd_raw_send_command(uint8_t command, uint32_t arg) {
    uint8_t response;

    /* wait some clock cycles */
    sd_raw_rec_byte();

    /* send command via SPI */
    sd_raw_send_byte(0x40 | command);
    sd_raw_send_byte((arg >> 24) & 0xff);
    sd_raw_send_byte((arg >> 16) & 0xff);
    sd_raw_send_byte((arg >> 8) & 0xff);
    sd_raw_send_byte((arg >> 0) & 0xff);
    switch(command) {
        case CMD_GO_IDLE_STATE:
           sd_raw_send_byte(0x95);
           break;
        case CMD_SEND_IF_COND:
           sd_raw_send_byte(0x87);
           break;
        default:
           sd_raw_send_byte(0xff);
           break;
    }
    
    /* receive response */
    uint8_t i;
    for(i = 0; i < 10; ++i) {
        response = sd_raw_rec_byte();
        if(response != 0xff)
            break;
    }

    return response;
}

/**
 * \ingroup sd_raw
 * Reads raw data from the card.
 *
 * \param[in] offset The offset from which to read.
 * \param[out] buffer The buffer into which to write the data.
 * \param[in] length The number of bytes to read.
 * \returns 0 on failure, 1 on success.
 * \see sd_raw_read_interval, sd_raw_write, sd_raw_write_interval
 */
uint8_t sd_raw_read(offset_t offset, uint8_t* buffer, uintptr_t length) {
    offset_t block_address;
    uint16_t block_offset;

    /* determine byte count to read at once */
    block_offset = offset & 0x01ff;
    block_address = offset - block_offset;
        
    /* address card */
    select_card();

    /* send single block request */
#if SD_RAW_SDHC
    if(sd_raw_send_command(CMD_READ_SINGLE_BLOCK, (sd_raw_card_type & (1 << SD_RAW_SPEC_SDHC) ? block_address / 512 : block_address)))
#else
    if(sd_raw_send_command(CMD_READ_SINGLE_BLOCK, block_address))
#endif
    {
        unselect_card();
        return 0;
    }

    /* wait for data block (start byte 0xfe) */
    while(sd_raw_rec_byte() != 0xfe);

    /* read byte block */
    uint16_t i;
    for(i = 0; i < 512; ++i) {
        if (i < length) {
            buffer[i] = sd_raw_rec_byte();
        } else {
            sd_raw_rec_byte();
        }
    }

    /* read crc16 */
    sd_raw_rec_byte();
    sd_raw_rec_byte();
            
    /* deaddress card */
    unselect_card();

    /* let card some time to finish */
    sd_raw_rec_byte();

    return 1;
}

/**
 * \ingroup sd_raw
 * Writes raw data to the card.
 *
 * \note If write buffering is enabled, you might have to
 *       call sd_raw_sync() before disconnecting the card
 *       to ensure all remaining data has been written.
 *
 * \param[in] offset The offset where to start writing.
 * \param[in] buffer The buffer containing the data to be written.
 * \param[in] length The number of bytes to write.
 * \returns 0 on failure, 1 on success.
 * \see sd_raw_write_interval, sd_raw_read, sd_raw_read_interval
 */
uint8_t sd_raw_write(offset_t offset, const uint8_t* buffer, uintptr_t length) {
    offset_t block_address;
    uint16_t block_offset;

    /* determine byte count to write at once */
    block_offset = offset & 0x01ff;
    block_address = offset - block_offset;

    /* address card */
    select_card();

    /* send single block request */
#if SD_RAW_SDHC
    if(sd_raw_send_command(CMD_WRITE_SINGLE_BLOCK, (sd_raw_card_type & (1 << SD_RAW_SPEC_SDHC) ? block_address / 512 : block_address)))
#else
    if(sd_raw_send_command(CMD_WRITE_SINGLE_BLOCK, block_address))
#endif
    {
        unselect_card();
        return 0;
    }

    /* send start byte */
    sd_raw_send_byte(0xfe);

    /* write byte block */
    uint16_t i;
    for(i = 0; i < 512; ++i) {
        if (i < length) {
            sd_raw_send_byte(*buffer++);
        } else {
            sd_raw_send_byte(0x00);
        }
    }

    /* write dummy crc16 */
    sd_raw_send_byte(0xff);
    sd_raw_send_byte(0xff);

    /* wait while card is busy */
    while(sd_raw_rec_byte() != 0xff);
    sd_raw_rec_byte();

    /* deaddress card */
    unselect_card();

    return 1;
}


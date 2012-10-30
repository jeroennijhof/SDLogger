SDLogger
========

SDLogger library for Arduino with a MicroSD shield.

I wrote this library because all existing FAT16 libraries uses too
much RAM. Most Arduino's only have 1k RAM.
So instead of reading 512 bytes (1 SD block) in a buffer, change
the buffer and write it back, just take a new block for new data.
That way we don't need to read 512 bytes in a buffer.

Check sd_raw_config.h and SDLogger.h for SS/MOSI/MISO/SCK pins,
default SS=D8, MOSI=D11, MISO=D12 and SCK=D13.

Make sure you format SD with the function format(). This will
remove all partitions on SD and create a new layout.

The layout is pretty simple, the first 4 bytes of the first block
contains the current log size.
Then for every log() a new block is used.

If you want to clear the current log just use format().

If you want to keep the old log data on the SD and append new 
log data just comment out the line with sdlog.format() in the example.

The log data can be retrieved from you SD card with tools like dd:
    # dd if=/dev/mmcblk0 of=data.log bs=512 skip=1 count=100
    # cat data.log


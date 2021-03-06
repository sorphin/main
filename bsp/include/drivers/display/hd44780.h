/*
 * Copyright (c) 2015, Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __DRIVERS_DISPLAY_HD44780_H__
#define __DRIVERS_DISPLAY_HD44780_H__

#include "infra/device.h"

/**
 * @defgroup hd44780 HD44780 Display Driver
 * HD44780 Display Driver API
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "drivers/display/hd44780.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/drivers/display</tt>
 * <tr><th><b>Config flag</b> <td><tt>DISPLAY_HD44780</tt>
 * </table>
 *
 * HD44780 display driver uses a SPI bus in bit-banging mode to interface
 * with an HD44780 connected to a level translator (1.8V to 3.3V)
 *
 * @ingroup ext_drivers
 * @{
 */

#define COMMAND_CLEAR  0x1
#define COMMAND_HOME   0x2
#define COMMAND_MODE   0x4
#define MODE_INC_DEC   0x2
#define MODE_SHIFT     0x1

#define COMMAND_ONOFF  0x8
#define ONOFF_DISPLAY  0x4
#define ONOFF_CURSOR   0x2
#define ONOFF_BLINK    0x1

#define COMMAND_CURSOR 0x10

#define COMMAND_FUNC   0x20
#define FUNC_DATA_LEN  0x10
#define FUNC_NUM_LINES 0x8
#define FUNC_FONT      0x4

#define COMMAND_CGADDR 0x40
#define COMMAND_DDADDR 0x80

/**
 *  HD44780 display driver.
 */
extern struct driver hd44780_driver;

/**
 * Send character to display
 *
 * @param   data  Character to send
 */
void hd44780_data(char data);

/**
 * Send a command to the display
 *
 * @param   data  Command to send
 */
void hd44780_command(char data);

/**
 * Clear the display
 */
void hd44780_clear();

/**
 * Set insertion point on the display
 *
 * @param   line  Line where to set cursor
 * @param   col   Column where to set cursor
 */
void hd44780_goto(int line, int col);

/**
 * Write a char string to the display
 *
 * @param   str   String to write to display
 */
void hd44780_puts(char *str);

/**
 * Enable backlight
 *
 * @param   enable 0 to turn off, !=0 to turn on
 */
void hd44780_backlight(int enable);

/**
 * Return cursor to home
 */
void hd44780_home(void);

/** @} */
#endif /* __DRIVERS_DISPLAY_HD44780_H__ */

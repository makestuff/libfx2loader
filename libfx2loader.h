/* 
 * Copyright (C) 2009-2010 Chris McClelland
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBFX2LOADER_H
#define LIBFX2LOADER_H

#include <stddef.h>
#include <makestuff.h>

#ifdef __cplusplus
extern "C" {
#endif
	// Return codes
	typedef enum {
		FX2_SUCCESS = 0,
		FX2_USB_ERR,
		FX2_BUF_ERR
	} FX2Status;

	// Forward-declaration of the LibUSB handle
	struct usb_dev_handle;

	// Forward-declaration of the Buffer struct
	struct Buffer;

	// Defined in ram.c:
	DLLEXPORT(FX2Status) fx2WriteRAM(
		struct usb_dev_handle *device, const uint8 *bufPtr, int numBytes, const char **error
	) WARN_UNUSED_RESULT;

	// Defined in eeprom.c:
	DLLEXPORT(FX2Status) fx2WriteEEPROM(
		struct usb_dev_handle *device, const uint8 *bufPtr, int numBytes, const char **error
	) WARN_UNUSED_RESULT;

	DLLEXPORT(FX2Status) fx2ReadEEPROM(
		struct usb_dev_handle *device, uint32 numBytes, struct Buffer *i2cBuffer, const char **error
	) WARN_UNUSED_RESULT;

	#define CONFIG_BYTE_DISCON (1<<6)
	#define CONFIG_BYTE_400KHZ (1<<0)

	typedef enum {
		I2C_SUCCESS,
		I2C_BUFFER_ERROR,
		I2C_NOT_INITIALISED,
		I2C_DEST_BUFFER_NOT_EMPTY
	} I2CStatus;

	DLLEXPORT(void) i2cInitialise(
		struct Buffer *buf, uint16 vid, uint16 pid, uint16 did, uint8 configByte
	);

	DLLEXPORT(I2CStatus) i2cWritePromRecords(
		struct Buffer *destination, const struct Buffer *sourceData,
		const struct Buffer *sourceMask, const char **error
	) WARN_UNUSED_RESULT;

	DLLEXPORT(I2CStatus) i2cReadPromRecords(
		struct Buffer *destData, struct Buffer *destMask, const struct Buffer *source,
		const char **error
	) WARN_UNUSED_RESULT;

	DLLEXPORT(I2CStatus) i2cFinalise(
		struct Buffer *buf, const char **error
	) WARN_UNUSED_RESULT;

#ifdef __cplusplus
}
#endif

#endif

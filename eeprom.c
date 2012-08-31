/*
 * Copyright (C) 2009-2012 Chris McClelland
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <makestuff.h>
#include <libusbwrap.h>
#include <liberror.h>
#include <libbuffer.h>
#include "libfx2loader.h"

#define A2_WRITE_ERROR "fx2WriteEEPROM(): This firmware does not seem to support EEPROM operations - try loading an appropriate firmware into RAM first"
#define A2_READ_ERROR "fx2ReadEEPROM(): This firmware does not seem to support EEPROM operations - try loading an appropriate firmware into RAM first"
#define BLOCK_SIZE 4096L

// Write the supplied reader buffer to EEPROM, using the supplied VID/PID.
//
DLLEXPORT(FX2Status) fx2WriteEEPROM(
	struct usb_dev_handle *device, const uint8 *bufPtr, int numBytes, const char **error)
{
	FX2Status returnCode = FX2_SUCCESS;
	int uStatus;
	uint16 address = 0x0000;
	while ( numBytes > BLOCK_SIZE ) {
		uStatus = usbControlWrite(
			device,
			0xA2,        // bRequest: EEPROM access
			address,     // wValue: address to write
			0x0000,      // wIndex: presently unused (will be A16)
			bufPtr,      // data to be written
			BLOCK_SIZE,  // wLength: number of bytes to be written
			5000,        // timeout
			error
		);
		CHECK_STATUS(uStatus, A2_WRITE_ERROR, FX2_USB_ERR);
		numBytes -= BLOCK_SIZE;
		bufPtr += BLOCK_SIZE;
		address += BLOCK_SIZE;
	}
	uStatus = usbControlWrite(
		device,
		0xA2,      // bRequest: EEPROM access
		address,   // wValue: address to write
		0x0000,    // wIndex: presently unused (will be A16)
		bufPtr,    // data to be written
		numBytes,  // wLength: number of bytes to be written
		5000,      // timeout
		error
	);
	CHECK_STATUS(uStatus, A2_WRITE_ERROR, FX2_USB_ERR);
cleanup:
	return returnCode;
}

// Read from the EEPROM into the supplied buffer, using the supplied VID/PID.
//
DLLEXPORT(FX2Status) fx2ReadEEPROM(
	struct usb_dev_handle *device, uint32 numBytes, struct Buffer *i2cBuffer, const char **error)
{
	FX2Status returnCode = FX2_SUCCESS;
	int uStatus;
	uint16 address = 0x0000;
	uint8 *bufPtr;
	if ( bufAppendConst(i2cBuffer, 0x00, numBytes, error) ) {
		errPrefix(error, "fx2ReadEEPROM()");
		FAIL(FX2_BUF_ERR);
	}
	bufPtr = i2cBuffer->data;
	while ( numBytes > BLOCK_SIZE ) {
		uStatus = usbControlRead(
			device,
			0xA2,        // bRequest: EEPROM access
			address,     // wValue: address to read
			0x0000,      // wIndex: presently unused (will be A16)
			bufPtr,      // data to be written
		   BLOCK_SIZE,  // wLength: number of bytes to be written
			5000,        // timeout
			error
		);
		CHECK_STATUS(uStatus, A2_READ_ERROR, FX2_USB_ERR);
		numBytes -= BLOCK_SIZE;
		bufPtr += BLOCK_SIZE;
		address += BLOCK_SIZE;
	}
	uStatus = usbControlRead(
		device,
		0xA2,      // bRequest: EEPROM access
		address,   // wValue: address to read
		0x0000,    // wIndex: presently unused (will be A16)
		bufPtr,    // data to be written
		numBytes,  // wLength: number of bytes to be written
		5000,      // timeout
		error
	);
	CHECK_STATUS(uStatus, A2_READ_ERROR, FX2_USB_ERR);
cleanup:
	return returnCode;
}

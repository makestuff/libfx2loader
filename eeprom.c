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
#include "vendorCommands.h"

#define A2_ERROR ": This firmware does not seem to support EEPROM operations - try loading an appropriate firmware into RAM first"
#define BLOCK_SIZE 4096

// Write the supplied reader buffer to EEPROM, using the supplied VID/PID.
//
DLLEXPORT(FX2Status) fx2WriteEEPROM(
	struct USBDevice *device, const uint8 *bufPtr, uint32 numBytes, const char **error)
{
	FX2Status retVal = FX2_SUCCESS;
	USBStatus uStatus;
	uint16 address = 0x0000;
	uint16 bank = 0x0000;
	while ( numBytes > BLOCK_SIZE ) {
		uStatus = usbControlWrite(
			device,
			CMD_READ_WRITE_EEPROM, // bRequest: EEPROM access
			address,               // wValue: address to write
			bank,                  // wIndex: bank (currently only 0 & 1 supported by firmware)
			bufPtr,                // data to be written
			BLOCK_SIZE,            // wLength: number of bytes to be written
			5000,                  // timeout
			error
		);
		CHECK_STATUS(uStatus, FX2_USB_ERR, cleanup, "fx2WriteEEPROM()"A2_ERROR);
		numBytes -= BLOCK_SIZE;
		bufPtr += BLOCK_SIZE;
		address = (uint16)(address + BLOCK_SIZE);
		if ( !address ) {
			bank++;
		}
	}
	uStatus = usbControlWrite(
		device,
		CMD_READ_WRITE_EEPROM, // bRequest: EEPROM access
		address,               // wValue: address to write
		bank,                  // wIndex: bank (currently only 0 & 1 supported by firmware)
		bufPtr,                // data to be written
		(uint16)numBytes,      // wLength: number of bytes to be written
		5000,                  // timeout
		error
	);
	CHECK_STATUS(uStatus, FX2_USB_ERR, cleanup, "fx2WriteEEPROM()"A2_ERROR);
cleanup:
	return retVal;
}

// Read from the EEPROM into the supplied buffer, using the supplied VID/PID.
//
DLLEXPORT(FX2Status) fx2ReadEEPROM(
	struct USBDevice *device, uint32 numBytes, struct Buffer *i2cBuffer, const char **error)
{
	FX2Status retVal = FX2_SUCCESS;
	USBStatus uStatus;
	BufferStatus bStatus;
	uint16 address = 0x0000;
	uint16 bank = 0x0000;
	uint8 *bufPtr;
	bStatus = bufAppendConst(i2cBuffer, 0x00, numBytes, error);
	CHECK_STATUS(bStatus, FX2_BUF_ERR, cleanup, "fx2ReadEEPROM()");
	bufPtr = i2cBuffer->data;
	while ( numBytes > BLOCK_SIZE ) {
		uStatus = usbControlRead(
			device,
			CMD_READ_WRITE_EEPROM, // bRequest: EEPROM access
			address,               // wValue: address to read
			bank,                  // wIndex: bank (currently only 0 & 1 supported by firmware)
			bufPtr,                // data to be written
		   BLOCK_SIZE,            // wLength: number of bytes to be written
			5000,                  // timeout
			error
		);
		CHECK_STATUS(uStatus, FX2_USB_ERR, cleanup, "fx2WriteEEPROM()"A2_ERROR);
		numBytes -= BLOCK_SIZE;
		bufPtr += BLOCK_SIZE;
		address = (uint16)(address + BLOCK_SIZE);
		if ( !address ) {
			bank++;
		}
	}
	uStatus = usbControlRead(
		device,
		CMD_READ_WRITE_EEPROM, // bRequest: EEPROM access
		address,               // wValue: address to read
		bank,                  // wIndex: bank (currently only 0 & 1 supported by firmware)
		bufPtr,                // data to be written
		(uint16)numBytes,      // wLength: number of bytes to be written
		5000,                  // timeout
		error
	);
	CHECK_STATUS(uStatus, FX2_USB_ERR, cleanup, "fx2WriteEEPROM()"A2_ERROR);
cleanup:
	return retVal;
}

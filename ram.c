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
#include "libfx2loader.h"
#include "vendorCommands.h"

#define BLOCK_SIZE 4096

// Write the supplied reader buffer to RAM, using the supplied VID/PID.
//
DLLEXPORT(FX2Status) fx2WriteRAM(
	struct USBDevice *device, const uint8 *bufPtr, uint32 numBytes, const char **error)
{
	FX2Status retVal = FX2_SUCCESS;
	uint16 address = 0x0000;
	uint8 byte = 0x01;
	USBStatus uStatus = usbControlWrite(
		device,
		CMD_READ_WRITE_RAM, // bRequest: RAM access
		0xE600,             // wValue: address to write (FX2 CPUCS)
		0x0000,             // wIndex: unused
		&byte,              // data = 0x01: hold 8051 in reset
		1,                  // wLength: just one byte
		5000,               // timeout
		error
	);
	CHECK_STATUS(uStatus, FX2_USB_ERR, cleanup, "fx2WriteRAM(): Failed to put the CPU in reset");
	while ( numBytes > BLOCK_SIZE ) {
		uStatus = usbControlWrite(
			device,
			CMD_READ_WRITE_RAM, // bRequest: RAM access
			address,            // wValue: RAM address to write
			0x0000,             // wIndex: unused
			bufPtr,             // data to be written
			BLOCK_SIZE,         // wLength: BLOCK_SIZE block
			5000,               // timeout
			error
		);
		CHECK_STATUS(uStatus, FX2_USB_ERR, cleanup, "fx2WriteRAM(): Failed to write block of bytes");
		numBytes -= BLOCK_SIZE;
		bufPtr += BLOCK_SIZE;
		address = (uint16)(address + BLOCK_SIZE);
	}

	// Write final chunk of data
	uStatus = usbControlWrite(
		device,
		CMD_READ_WRITE_RAM, // bRequest: RAM access
		address,            // wValue: RAM address to write
		0x0000,             // wIndex: unused
		bufPtr,             // data to be written
		(uint16)numBytes,   // wLength: remaining bytes
		5000,               // timeout
		error
	);
	CHECK_STATUS(uStatus, FX2_USB_ERR, cleanup, "fx2WriteRAM(): Failed to write final block");

	// There's an unavoidable race condition here: this command brings the FX2 out of reset, which
	// causes it to drop off the bus for renumeration. It may drop off before or after the host
	// gets its acknowledgement, so we cannot trust the return code. We have no choice but to
	// assume it worked.
	byte = 0x00;
	uStatus = usbControlWrite(
		device,
		CMD_READ_WRITE_RAM, // bRequest: RAM access
		0xE600,             // wValue: address to write (FX2 CPUCS)
		0x0000,             // wIndex: unused
		&byte,              // data = 0x00: bring 8051 out of reset
		1,                  // wLength: just one byte
		5000,               // timeout
		NULL
	);
cleanup:
	return retVal;
}

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

#define BLOCK_SIZE 4096

// Write the supplied reader buffer to RAM, using the supplied VID/PID.
//
DLLEXPORT(FX2Status) fx2WriteRAM(
	struct USBDevice *device, const uint8 *bufPtr, uint32 numBytes, const char **error)
{
	FX2Status returnCode = FX2_SUCCESS;
	int uStatus;
	uint16 address = 0x0000;
	uint8 byte = 0x01;
	uStatus = usbControlWrite(
		device,
		0xA0,    // bRequest: RAM access
		0xE600,  // wValue: address to write (FX2 CPUCS)
		0x0000,  // wIndex: unused
		&byte,   // data = 0x01: hold 8051 in reset
		1,       // wLength: just one byte
		5000,    // timeout
		error
	);
	CHECK_STATUS(uStatus, "fx2WriteRAM(): Failed to put the CPU in reset", FX2_USB_ERR);
	while ( numBytes > BLOCK_SIZE ) {
		uStatus = usbControlWrite(
			device,
			0xA0,     // bRequest: RAM access
			address,  // wValue: RAM address to write
			0x0000,   // wIndex: unused
			bufPtr,   // data to be written
			BLOCK_SIZE,     // wLength: BLOCK_SIZE block
			5000,     // timeout
			error
		);
		CHECK_STATUS(uStatus, "fx2WriteRAM(): Failed to write block of bytes", FX2_USB_ERR);
		numBytes -= BLOCK_SIZE;
		bufPtr += BLOCK_SIZE;
		address += BLOCK_SIZE;
	}
	uStatus = usbControlWrite(
		device,
		0xA0,              // bRequest: RAM access
		address,           // wValue: RAM address to write
		0x0000,            // wIndex: unused
		bufPtr,            // data to be written
		(uint16)numBytes,  // wLength: remaining bytes
		5000,              // timeout
		error
	);
	CHECK_STATUS(uStatus, "fx2WriteRAM(): Failed to write final block", FX2_USB_ERR);

	// Since this brings the FX2 out of reset, the host may get a 'failed' returnCode. We have to
	// assume that it worked nevertheless.
	byte = 0x00;
	uStatus = usbControlWrite(
		device,
		0xA0,    // bRequest: RAM access
		0xE600,  // wValue: address to write (FX2 CPUCS)
		0x0000,  // wIndex: unused
		&byte,   // data = 0x00: bring 8051 out of reset
		1,       // wLength: just one byte
		5000,    // timeout
		NULL
	);
cleanup:
	return returnCode;
}

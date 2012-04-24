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
#ifdef WIN32
	#include <lusb0_usb.h>
#else
	#include <usb.h>
#endif
#include <liberror.h>
#include <libbuffer.h>
#include "libfx2loader.h"

#define A2_WRITE_ERROR "fx2WriteEEPROM(): This firmware does not seem to support EEPROM operations - try loading an appropriate firmware into RAM first\nDiagnostic information: failed writing %lu bytes to 0x%04X returnCode %d: %s"
#define A2_READ_ERROR "fx2ReadEEPROM(): This firmware does not seem to support EEPROM operations - try loading an appropriate firmware into RAM first\nDiagnostic information: failed writing %lu bytes to 0x%04X returnCode %d: %s"
#define BLOCK_SIZE 4096L

// Write the supplied reader buffer to EEPROM, using the supplied VID/PID.
//
DLLEXPORT(FX2Status) fx2WriteEEPROM(
	struct usb_dev_handle *device, const uint8 *bufPtr, int numBytes, const char **error)
{
	FX2Status returnCode;
	int uStatus;
	uint16 address = 0x0000;
	while ( numBytes > BLOCK_SIZE ) {
		uStatus = usb_control_msg(
			device,
			(USB_ENDPOINT_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE),
			0xA2, address, 0x0000, (char*)bufPtr, BLOCK_SIZE, 5000
		);
		if ( uStatus != BLOCK_SIZE ) {
			errRender(error, A2_WRITE_ERROR, BLOCK_SIZE, address, uStatus, usb_strerror());
			FAIL(FX2_USB_ERR);
		}
		numBytes -= BLOCK_SIZE;
		bufPtr += BLOCK_SIZE;
		address += BLOCK_SIZE;
	}
	uStatus = usb_control_msg(
		device,
		(USB_ENDPOINT_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE),
		0xA2, address, 0x0000, (char*)bufPtr, numBytes, 5000
	);
	if ( uStatus != numBytes ) {
		errRender(error, A2_WRITE_ERROR, numBytes, address, uStatus, usb_strerror());
		FAIL(FX2_USB_ERR);
	}
	return FX2_SUCCESS;
cleanup:
	return returnCode;
}

// Read from the EEPROM into the supplied buffer, using the supplied VID/PID.
//
DLLEXPORT(FX2Status) fx2ReadEEPROM(
	struct usb_dev_handle *device, uint32 numBytes, struct Buffer *i2cBuffer, const char **error)
{
	FX2Status returnCode;
	int uStatus;
	uint16 address = 0x0000;
	uint8 *bufPtr;
	if ( bufAppendConst(i2cBuffer, 0x00, numBytes, error) ) {
		errPrefix(error, "fx2ReadEEPROM()");
		FAIL(FX2_BUF_ERR);
	}
	bufPtr = i2cBuffer->data;
	while ( numBytes > BLOCK_SIZE ) {
		uStatus = usb_control_msg(
			device,
			(USB_ENDPOINT_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE),
			0xA2, address, 0x0000, (char*)bufPtr, BLOCK_SIZE, 5000
		);
		if ( uStatus != BLOCK_SIZE ) {
			errRender(error, A2_READ_ERROR, BLOCK_SIZE, address, uStatus, usb_strerror());
			FAIL(FX2_USB_ERR);
		}
		numBytes -= BLOCK_SIZE;
		bufPtr += BLOCK_SIZE;
		address += BLOCK_SIZE;
	}
	uStatus = usb_control_msg(
		device,
		(USB_ENDPOINT_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE),
		0xA2, address, 0x0000, (char*)bufPtr, numBytes, 5000
	);
	if ( uStatus != (int)numBytes ) {
		errRender(error, A2_READ_ERROR, numBytes, address, uStatus, usb_strerror());
		FAIL(FX2_USB_ERR);
	}
	return FX2_SUCCESS;
cleanup:
	return returnCode;
}

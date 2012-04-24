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
#include "libfx2loader.h"

// Write the supplied reader buffer to RAM, using the supplied VID/PID.
//
DLLEXPORT(FX2Status) fx2WriteRAM(
	struct usb_dev_handle *device, const uint8 *bufPtr, int numBytes, const char **error)
{
	FX2Status returnCode;
	int uStatus;
	uint16 address = 0x0000;
	char byte = 0x01;
	uStatus = usb_control_msg(
		device,
		(USB_ENDPOINT_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE),
		0xA0, 0xE600, 0x0000, &byte, 1, 5000
	);
	if ( uStatus != 1 ) {
		errRender(
			error,
			"fx2WriteRAM(): Failed to put the CPU in reset - usb_control_msg() failed returnCode %d: %s",
			uStatus, usb_strerror());
		FAIL(FX2_USB_ERR);
	}
	while ( numBytes > 4096 ) {
		uStatus = usb_control_msg(
			device,
			(USB_ENDPOINT_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE),
			0xA0, address, 0x0000, (char*)bufPtr, 4096, 5000
		);
		if ( uStatus != 4096 ) {
			errRender(
				error,
				"fx2WriteRAM(): Failed to write block of 4096 bytes at 0x%04X - usb_control_msg() failed returnCode %d: %s",
				address, uStatus, usb_strerror());
			FAIL(FX2_USB_ERR);
		}
		numBytes -= 4096;
		bufPtr += 4096;
		address += 4096;
	}
	uStatus = usb_control_msg(
		device,
		(USB_ENDPOINT_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE),
		0xA0, address, 0x0000, (char*)bufPtr, numBytes, 5000
	);
	if ( uStatus != numBytes ) {
		errRender(
			error,
			"fx2WriteRAM(): Failed to write block of %d bytes at 0x%04X - usb_control_msg() failed returnCode %d: %s",
			numBytes, address, uStatus, usb_strerror());
		FAIL(FX2_USB_ERR);
	}

	// Since this brings the FX2 out of reset, the host may get a 'failed' returnCode. We have to
	// assume that it worked nevertheless.
	byte = 0x00;
	usb_control_msg(
		device,
		(USB_ENDPOINT_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE),
		0xA0, 0xE600, 0x0000, &byte, 1, 5000
	);
	return FX2_SUCCESS;
cleanup:
	return returnCode;
}

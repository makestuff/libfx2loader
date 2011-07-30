/* 
 * Copyright (C) 2009 Chris McClelland
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
#include <stddef.h>
#include <makestuff.h>
#include <liberror.h>
#include <libbuffer.h>
#include "libfx2loader.h"

#define LSB(x) ((x) & 0xFF)
#define MSB(x) ((x) >> 8)

// Initialise the buffer as a C2 loader, using supplied values. It seems like for C2 loaders the
// values of vid & pid are never used though. The configByte has only two bits - bit zero selects
// 400kHz I2C bus speed and bit six sets the USBCS.3 bit "DISCON" so the chip comes up disconnected.
//
DLLEXPORT(void) i2cInitialise(
	struct Buffer *buf, uint16 vid, uint16 pid, uint16 did, uint8 configByte)
{
	buf->length = 8;
	buf->data[0] = 0xC2;
	buf->data[1] = LSB(vid);
	buf->data[2] = MSB(vid);
	buf->data[3] = LSB(pid);
	buf->data[4] = MSB(pid);
	buf->data[5] = LSB(did);
	buf->data[6] = MSB(did);
	buf->data[7] = configByte;
}

// Dump the selected range of the HexReader buffers as I2C records to the supplied buffer. This will
// split up large chunks into chunks 1023 bytes or smaller so chunk lengths fit in ten bits.
// (see TRM 3.4.3)
//
static I2CStatus dumpChunk(
	struct Buffer *destination, const struct Buffer *sourceData, const struct Buffer *sourceMask,
	uint16 address, uint16 length, const char **error)
{
	BufferStatus bStatus;
	uint8 *chunkHeader;
	size_t i, startBlock;
	I2CStatus iStatus;
	if ( length == 0 ) {
		return I2C_SUCCESS;
	}
	while ( length > 1023 ) {
		iStatus = dumpChunk(destination, sourceData, sourceMask, address, 1023, error);
		address += 1023;
		length -= 1023;
	}
	bStatus = bufAppendZeros(destination, 4, &chunkHeader, error);
	if ( bStatus != BUF_SUCCESS ) {
		errPrefix(error, "dumpChunk()");
		return I2C_BUFFER_ERROR;
	}
	chunkHeader[0] = MSB(length);
	chunkHeader[1] = LSB(length);
	chunkHeader[2] = MSB(address);
	chunkHeader[3] = LSB(address);
	startBlock = destination->length;
	bStatus = bufAppendBlock(destination, sourceData->data + address, length, error);
	if ( bStatus != BUF_SUCCESS ) {
		errPrefix(error, "dumpChunk()");
		return I2C_BUFFER_ERROR;
	}
	for ( i = 0; i < length; i++ ) {
		if ( sourceMask->data[address + i] == 0x00 ) {
			destination->data[startBlock + i] = 0x00;
		}
	}
	return I2C_SUCCESS;
}

// Build EEPROM records from the data/mask source buffers and write to the destination buffer.
//
DLLEXPORT(I2CStatus) i2cWritePromRecords(
	struct Buffer *destination, const struct Buffer *sourceData, const struct Buffer *sourceMask,
	const char **error)
{
	uint16 i, chunkStart;
	I2CStatus status;
	if ( destination->length != 8 || destination->data[0] != 0xC2 ) {
		errRender(error, "i2cWritePromRecords(): the buffer was not initialised");
		return I2C_NOT_INITIALISED;
	}

	i = 0;
	while ( !sourceMask->data[i] && i < sourceData->length ) {
		i++;
	}
	if ( i == sourceData->length ) {
		return I2C_SUCCESS;  // There are no data
	}

	// There is definitely some data to write
	//
	chunkStart = i;  // keep a record of where this block starts
	do {
		// Find the end of this block of ones
		//
		while ( sourceMask->data[i] && i < sourceData->length ) {
			i++;
		}
		if ( i == sourceData->length ) {
			status = dumpChunk(
				destination, sourceData, sourceMask, chunkStart,
				(uint16)sourceData->length - chunkStart, error);
			if ( status != I2C_SUCCESS ) {
				return status;
			}
			break;
		}

		// Now check: is this run of zeroes worth splitting the block for?
		//
		// There are four bytes of overhead to opening a new record, so on balance it appears that
		// the smallest run of zeros worth breaking a block is FIVE. But since the maximum record
		// length is 1023 bytes, it's actually good to break on FOUR bytes - it costs nothing
		// extra, but it hopefully keeps the number of forced (1023-byte) breaks to a minimum.
		//
		if ( i < sourceData->length-4 ) {
			// We are not within five bytes of the end
			//
			if ( !sourceMask->data[i] && !sourceMask->data[i+1] &&
			     !sourceMask->data[i+2] && !sourceMask->data[i+3] )
			{
				// Yes, let's split it - dump the current block and start a fresh one
				//
				status = dumpChunk(
					destination, sourceData, sourceMask, chunkStart, i - chunkStart, error);
				if ( status != I2C_SUCCESS ) {
					return status;
				}
				
				// Skip these four...we know they're zero
				//
				i += 4;
				
				// Find the next block of ones
				//
				while ( i < sourceMask->length && !sourceMask->data[i] ) {
					i++;
				}
				chunkStart = i;
			} else {
				// This is four or fewer zeros - not worth splitting for so skip over them
				//
				while ( !sourceMask->data[i] ) {
					i++;
				}
			}
		} else {
			// We are within four bytes of the end - include the remainder, whatever it is
			//
			status = dumpChunk(
				destination, sourceData, sourceMask, chunkStart,
				(uint16)sourceMask->length - chunkStart, error);
			if ( status != I2C_SUCCESS ) {
				return status;
			}
			break;
		}
	} while ( i < sourceData->length );
	
	return I2C_SUCCESS;
}

// Read EEPROM records from the source buffer and write the decoded data to the data/mask
// destination buffers.
//
DLLEXPORT(I2CStatus) i2cReadPromRecords(
	struct Buffer *destData, struct Buffer *destMask, const struct Buffer *source,
	const char **error)
{
	uint16 chunkAddress, chunkLength;
	const uint8 *ptr = source->data;
	const uint8 *const ptrEnd = ptr + source->length;
	BufferStatus bStatus;
	if ( source->length < 8+5 || ptr[0] != 0xC2 ) {
		errRender(error, "i2cReadPromRecords(): the EEPROM records appear to be corrupt");
		return I2C_NOT_INITIALISED;
	}
	if ( destData->length != 0 || destMask->length != 0 ) {
		errRender(error, "i2cReadPromRecords(): the destination buffer is not empty");
		return I2C_DEST_BUFFER_NOT_EMPTY;
	}
	ptr += 8;  // skip over the header
	while ( ptr < ptrEnd ) {
		chunkLength = ((ptr[0] << 8) + ptr[1]);
		chunkAddress = (ptr[2] << 8) + ptr[3];
		if ( chunkLength & 0x8000 ) {
			break;
		}
		chunkLength &= 0x03FF;
		ptr += 4;
		bStatus = bufCopyBlock(destData, chunkAddress, ptr, chunkLength, error);
		if ( bStatus != BUF_SUCCESS ) {
			errPrefix(error, "i2cReadPromRecords()");
			return I2C_BUFFER_ERROR;
		}
		bStatus = bufSetBlock(destMask, chunkAddress, 0x01, chunkLength, error);
		if ( bStatus != BUF_SUCCESS ) {
			errPrefix(error, "i2cReadPromRecords()");
			return I2C_BUFFER_ERROR;
		}
		ptr += chunkLength;
	}
	return I2C_SUCCESS;
}

// Finalise the I2C buffers. This involves writing the final record which resets the chip.
//
DLLEXPORT(I2CStatus) i2cFinalise(struct Buffer *buf, const char **error) {
	BufferStatus status;
	uint8 *lastRecord;
	if ( buf->length < 8 || buf->data[0] != 0xC2 ) {
		errRender(error, "i2cFinalise(): the buffer was not initialised");
		return I2C_NOT_INITIALISED;
	}
	status = bufAppendZeros(buf, 5, &lastRecord, error);
	if ( status != BUF_SUCCESS ) {
		errPrefix(error, "i2cFinalise()");
		return I2C_BUFFER_ERROR;
	}
	lastRecord[0] = 0x80;  // This record means "write 0x00 to 0xE600 to reset the chip"
	lastRecord[1] = 0x01;
	lastRecord[2] = 0xe6;
	lastRecord[3] = 0x00;
	lastRecord[4] = 0x00;
	return I2C_SUCCESS;
}

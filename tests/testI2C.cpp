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
#include <string.h>
#include <UnitTest++.h>
#include <makestuff.h>
#include <libbuffer.h>
#include "../libfx2loader.h"
#include "../libfx2loader.h"

#define VID 0x04b4
#define PID 0x8613
#define DID 0x0000

#define LSB(x) (x & 0xFF)
#define MSB(x) (x >> 8)

TEST(I2C_testInitialise) {
	Buffer i2cBuffer;
	I2CStatus iStatus;
	BufferStatus bStatus;
	const uint8 expected[] = {0xC2, LSB(VID), MSB(VID), LSB(PID), MSB(PID), LSB(DID), MSB(DID), CONFIG_BYTE_400KHZ, 0x80, 0x01, 0xE6, 0x00, 0x00};
	bStatus = bufInitialise(&i2cBuffer, 1024, 0x00, NULL);
	CHECK_EQUAL(BUF_SUCCESS, bStatus);
	i2cInitialise(&i2cBuffer, VID, PID, DID, CONFIG_BYTE_400KHZ);
	iStatus = i2cFinalise(&i2cBuffer, NULL);
	CHECK_EQUAL(I2C_SUCCESS, iStatus);
	CHECK_EQUAL(8UL+5UL, i2cBuffer.length);
	CHECK_EQUAL(1024UL, i2cBuffer.capacity);
	CHECK_ARRAY_EQUAL(expected, i2cBuffer.data, 8+5);
	bufDestroy(&i2cBuffer);
}

TEST(I2C_testRoundTrip) {
	uint32 i, j;
	Buffer i2cBuffer, srcData, srcMask, dstData, dstMask;
	BufferStatus bStatus;
	I2CStatus iStatus;
	const uint8 data[] = {0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, 0xF0};
	uint8 mask[16];
	uint8 expected[16];
	for ( i = 0; i < 65536; i++ ) {   // all possible 16-bit mask patterns
		for ( j = 0; j < 16; j++ ) {
			if ( i & (1<<j) ) {
				mask[j] = 0x01;
				expected[j] = data[j];
			} else {
				mask[j] = 0x00;
				expected[j] = 0x00;
			}
		}
		bStatus = bufInitialise(&i2cBuffer, 1024, 0x00, NULL);
		CHECK_EQUAL(BUF_SUCCESS, bStatus);
		i2cInitialise(&i2cBuffer, VID, PID, DID, CONFIG_BYTE_400KHZ);
		bStatus = bufInitialise(&srcData, 16, 0x00, NULL);
		CHECK_EQUAL(BUF_SUCCESS, bStatus);
		bStatus = bufInitialise(&dstData, 16, 0x00, NULL);
		CHECK_EQUAL(BUF_SUCCESS, bStatus);
		bStatus = bufInitialise(&srcMask, 16, 0x00, NULL);
		CHECK_EQUAL(BUF_SUCCESS, bStatus);
		bStatus = bufInitialise(&dstMask, 16, 0x00, NULL);
		CHECK_EQUAL(BUF_SUCCESS, bStatus);
		bStatus = bufWriteBlock(&srcData, 0x00000000, expected, 16, NULL);
		CHECK_EQUAL(BUF_SUCCESS, bStatus);
		bStatus = bufWriteBlock(&srcMask, 0x00000000, mask, 16, NULL);
		CHECK_EQUAL(BUF_SUCCESS, bStatus);
		iStatus = i2cWritePromRecords(&i2cBuffer, &srcData, &srcMask, NULL);
		CHECK_EQUAL(I2C_SUCCESS, iStatus);
		iStatus = i2cFinalise(&i2cBuffer, NULL);
		CHECK_EQUAL(I2C_SUCCESS, iStatus);
		iStatus = i2cReadPromRecords(&dstData, &dstMask, &i2cBuffer, NULL);
		CHECK_EQUAL(I2C_SUCCESS, iStatus);
		CHECK_ARRAY_EQUAL(expected, dstData.data, 16);
		bufDestroy(&dstMask);
		bufDestroy(&srcMask);
		bufDestroy(&dstData);
		bufDestroy(&srcData);
		bufDestroy(&i2cBuffer);
	}
}

TEST(I2C_testWPRNoInit) {
	Buffer i2cBuffer;
	I2CStatus iStatus;
	BufferStatus bStatus;
	bStatus = bufInitialise(&i2cBuffer, 8, 0x00, NULL);
	CHECK_EQUAL(BUF_SUCCESS, bStatus);
	iStatus = i2cWritePromRecords(&i2cBuffer, NULL, NULL, NULL);
	CHECK_EQUAL(I2C_NOT_INITIALISED, iStatus);
	bufDestroy(&i2cBuffer);
}

TEST(I2C_testFinaliseNoInit) {
	Buffer buf;
	I2CStatus iStatus;
	BufferStatus bStatus;
	bStatus = bufInitialise(&buf, 8, 0x00, NULL);
	CHECK_EQUAL(BUF_SUCCESS, bStatus);
	iStatus = i2cFinalise(&buf, NULL);
	CHECK_EQUAL(I2C_NOT_INITIALISED, iStatus);
	bufDestroy(&buf);
}

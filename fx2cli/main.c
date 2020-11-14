/* 
 * Copyright (C) 2009-2011 Chris McClelland
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sheitmann/libargtable2.h>
#include <makestuff/libusbwrap.h>
#include <makestuff/libfx2loader.h>
#include <makestuff/liberror.h>
#include <makestuff/libbuffer.h>

typedef enum {
	SRC_BAD,
	SRC_EEPROM,
	SRC_HEXFILE,
	SRC_BIXFILE,
	SRC_IICFILE
} Source;

typedef enum {
	DST_RAM,
	DST_EEPROM,
	DST_HEXFILE,
	DST_IICFILE,
	DST_BIXFILE
} Destination;

#define INDENT "                              "

int main(int argc, char *argv[]) {
	struct arg_str *vpOpt   = arg_str0("v", "vidpid", "<VID:PID>", " vendor ID and product ID (e.g 04B4:8613)");
	struct arg_lit *helpOpt = arg_lit0("h", "help", "             print this help and exit");
	struct arg_str *srcOpt = arg_str1(
		NULL, NULL, "<source>",
		"             where to read from:\n"
		INDENT"eeprom:<size>: external EEPROM (size in kbits)\n"
		INDENT"fileName.hex: I8HEX-format .hex or .ihx file\n"
		INDENT"fileName.bix: binary .bix file\n"
		INDENT"fileName.iic: Cypress .iic-format file");
	struct arg_str *dstOpt = arg_str0(
		NULL, NULL, "<destination>", "          where to write to:\n"
		INDENT"ram: internal RAM (default)\n"
		INDENT"eeprom: external EEPROM\n"
		INDENT"fileName.hex: I8HEX-format .hex or .ihx file\n"
		INDENT"fileName.bix: binary .bix file\n"
		INDENT"fileName.iic: Cypress .iic-format file");
	struct arg_end *endOpt = arg_end(20);
	void* argTable[] = {vpOpt, helpOpt, srcOpt, dstOpt, endOpt};
	const char *progName = "fx2loader";
	int retVal = 0;
	int numErrors;
	Source src = SRC_BAD;
	Destination dst;
	struct Buffer sourceData = {0};
	struct Buffer sourceMask = {0};
	struct Buffer i2cBuffer = {0};
	const char *srcExt, *dstExt;
	uint32 eepromSize = 0;
	struct USBDevice *device = NULL;
	const char *error = NULL;

	// Parse arguments...
	//
	if ( arg_nullcheck(argTable) != 0 ) {
		printf("%s: insufficient memory\n", progName);
		FAIL_RET(1, cleanup);
	}

	numErrors = arg_parse(argc, argv, argTable);

	if ( helpOpt->count > 0 ) {
		printf("FX2Loader Command-Line Interface Copyright (C) 2009-2011 Chris McClelland\n\nUsage: %s", progName);
		arg_print_syntax(stdout, argTable, "\n");
		printf("\nUpload code to the Cypress FX2LP.\n\n");
		arg_print_glossary(stdout, argTable,"  %-10s %s\n");
		FAIL_RET(0, cleanup);
	}

	if ( numErrors > 0 ) {
		arg_print_errors(stdout, endOpt, progName);
		printf("Try '%s --help' for more information.\n", progName);
		FAIL_RET(1, cleanup);
	}

	srcExt = srcOpt->sval[0] + strlen(srcOpt->sval[0]) - 4;
	if ( !strcmp(".hex", srcExt) || !strcmp(".ihx", srcExt) ) {
		src = SRC_HEXFILE;
	} else if ( !strcmp(".bix", srcExt) ) {
		src = SRC_BIXFILE;
	} else if ( !strcmp(".iic", srcExt) ) {
		src = SRC_IICFILE;
	} else if ( !strncmp("eeprom:", srcOpt->sval[0], 7) ) {
		const char *const eepromSizeString = srcOpt->sval[0] + 7;
		eepromSize = (uint32)atoi(eepromSizeString) * 128;  // size in bytes
		if ( eepromSize == 0 ) {
			fprintf(stderr, "You need to supply an EEPROM size in kilobits (e.g -s eeprom:128)\n");
			FAIL_RET(2, cleanup);
		}
		src = SRC_EEPROM;
	}

	if ( src == SRC_BAD ) {
		fprintf(stderr, "Unrecognised source: %s\n", srcOpt->sval[0]);
		FAIL_RET(3, cleanup);
	}

	if ( dstOpt->count ) {
		dstExt = dstOpt->sval[0] + strlen(dstOpt->sval[0]) - 4;
		if ( !strcmp(".hex", dstExt) || !strcmp(".ihx", dstExt) ) {
			dst = DST_HEXFILE;
		} else if ( !strcmp(".bix", dstExt) ) {
			dst = DST_BIXFILE;
		} else if ( !strcmp(".iic", dstExt) ) {
			dst = DST_IICFILE;
		} else if ( !strcmp("ram", dstOpt->sval[0]) ) {
			dst = DST_RAM;
		} else if ( !strcmp("eeprom", dstOpt->sval[0]) ) {
			dst = DST_EEPROM;
		} else {
			fprintf(stderr, "Unrecognised destination: %s\n", srcOpt->sval[0]);
			FAIL_RET(4, cleanup);
		}
	} else {
		dst = DST_RAM;
	}

	if ( src == SRC_EEPROM || dst == DST_EEPROM || dst == DST_RAM ) {
		if ( !vpOpt->count ) {
			fprintf(stderr, "Missing VID:PID - try something like \"-v 04b4:8613\"\n");
			FAIL_RET(5, cleanup);
		}
		CHECK_STATUS(usbInitialise(0, &error), 6, cleanup);
		CHECK_STATUS(usbOpenDevice(vpOpt->sval[0], 1, 0, 0, &device, &error), 7, cleanup);
	}

	// Initialise buffers...
	//
	CHECK_STATUS(bufInitialise(&sourceData, 1024, 0x00, &error), 8, cleanup);
	CHECK_STATUS(bufInitialise(&sourceMask, 1024, 0x00, &error), 9, cleanup);
	CHECK_STATUS(bufInitialise(&i2cBuffer, 1024, 0x00, &error), 10, cleanup);

	// Read from source...
	//
	if ( src == SRC_HEXFILE ) {
		CHECK_STATUS(bufReadFromIntelHexFile(&sourceData, &sourceMask, srcOpt->sval[0], &error), 11, cleanup);
	} else if ( src == SRC_BIXFILE ) {
		CHECK_STATUS(bufAppendFromBinaryFile(&sourceData, srcOpt->sval[0], &error), 12, cleanup);
		CHECK_STATUS(bufAppendConst(&sourceMask, 0x01, sourceData.length, &error), 13, cleanup);
	} else if ( src == SRC_IICFILE ) {
		CHECK_STATUS(bufAppendFromBinaryFile(&i2cBuffer, srcOpt->sval[0], &error), 14, cleanup);
	} else if ( src == SRC_EEPROM ) {
		CHECK_STATUS(fx2ReadEEPROM(device, eepromSize, &i2cBuffer, &error), 15, cleanup);
	} else {
		fprintf(stderr, "Internal error UNHANDLED_SRC\n");
		FAIL_RET(16, cleanup);
	}

	// Write to destination...
	//
	if ( dst == DST_RAM ) {
		// If the source data was I2C, write it to data/mask buffers
		//
		if ( i2cBuffer.length > 0 ) {
			CHECK_STATUS(i2cReadPromRecords(&sourceData, &sourceMask, &i2cBuffer, &error), 17, cleanup);
		}

		// Write the data to RAM
		//
		CHECK_STATUS(fx2WriteRAM(device, sourceData.data, (uint32)sourceData.length, &error), 18, cleanup);
	} else if ( dst == DST_EEPROM ) {
		// If the source data was *not* I2C, construct I2C data from the raw data/mask buffers
		//
		if ( i2cBuffer.length == 0 ) {
			i2cInitialise(&i2cBuffer, 0x0000, 0x0000, 0x0000, CONFIG_BYTE_400KHZ);
			CHECK_STATUS(i2cWritePromRecords(&i2cBuffer, &sourceData, &sourceMask, &error), 19, cleanup);
			CHECK_STATUS(i2cFinalise(&i2cBuffer, &error), 20, cleanup);
		}

		// Write the I2C data to the EEPROM
		//
		CHECK_STATUS(fx2WriteEEPROM(device, i2cBuffer.data, (uint32)i2cBuffer.length, &error), 21, cleanup);
	} else if ( dst == DST_HEXFILE ) {
		// If the source data was I2C, write it to data/mask buffers
		//
		if ( i2cBuffer.length > 0 ) {
			CHECK_STATUS(i2cReadPromRecords(&sourceData, &sourceMask, &i2cBuffer, &error), 22, cleanup);
		}

		// Write the data/mask buffers out as an I8HEX file
		//
		//dump(0x00000000, sourceMask.data, sourceMask.length);
		CHECK_STATUS(
			bufWriteToIntelHexFile(&sourceData, &sourceMask, dstOpt->sval[0], 16, false, &error),
			23, cleanup);
	} else if ( dst == DST_BIXFILE ) {
		// If the source data was I2C, write it to data/mask buffers
		//
		if ( i2cBuffer.length > 0 ) {
			CHECK_STATUS(i2cReadPromRecords(&sourceData, &sourceMask, &i2cBuffer, &error), 24, cleanup);
		}

		// Write the data buffer out as a binary file
		//
		CHECK_STATUS(
			bufWriteBinaryFile(&sourceData, dstOpt->sval[0], 0x00000000, sourceData.length, &error),
			25, cleanup);
	} else if ( dst == DST_IICFILE ) {
		// If the source data was *not* I2C, construct I2C data from the raw data/mask buffers
		//
		if ( i2cBuffer.length == 0 ) {
			i2cInitialise(&i2cBuffer, 0x0000, 0x0000, 0x0000, CONFIG_BYTE_400KHZ);
			CHECK_STATUS(i2cWritePromRecords(&i2cBuffer, &sourceData, &sourceMask, &error), 26, cleanup);
			CHECK_STATUS(i2cFinalise(&i2cBuffer, &error), 27, cleanup);
		}

		// Write the I2C data out as a binary file
		//
		CHECK_STATUS(
			bufWriteBinaryFile(&i2cBuffer, dstOpt->sval[0], 0x00000000, i2cBuffer.length, &error),
			28, cleanup);
	} else {
		fprintf(stderr, "Internal error UNHANDLED_DST\n");
		FAIL_RET(29, cleanup);
	}

cleanup:
	if ( error ) {
		fprintf(stderr, "%s: %s\n", argv[0], error);
		errFree(error);
	}
	usbCloseDevice(device, 0);
	usbShutdown();
	if ( i2cBuffer.data ) {
		bufDestroy(&i2cBuffer);
	}
	if ( sourceMask.data ) {
		bufDestroy(&sourceMask);
	}
	if ( sourceData.data ) {
		bufDestroy(&sourceData);
	}
	arg_freetable(argTable, sizeof(argTable)/sizeof(*argTable));
	return retVal;
}

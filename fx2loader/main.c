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
#include <argtable2.h>
#include <libusbwrap.h>
#include <usb.h>
#include <libfx2loader.h>
#include <liberror.h>
#include <libbuffer.h>

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

int main(int argc, char *argv[]) {

	struct arg_str *vpOpt = arg_str1("v", "vidpid", "<VID:PID>", " vendor ID and product ID (e.g 04B4:8613)");
	struct arg_lit *helpOpt  = arg_lit0("h", "help", "             print this help and exit");
	struct arg_str *srcOpt = arg_str1(NULL, NULL, "<source>", "             where to read from (<eeprom:<kbitSize> | fileName.hex | fileName.bix | fileName.iic>)");
	struct arg_str *dstOpt = arg_str0(NULL, NULL, "<destination>", "          where to write to (<ram | eeprom | fileName.hex | fileName.bix | fileName.iic> - defaults to \"ram\")");
	struct arg_end *endOpt   = arg_end(20);
	void* argTable[] = {vpOpt, helpOpt, srcOpt, dstOpt, endOpt};
	const char *progName = "fx2loader";
	int returnCode = 0;
	int numErrors;

	Source src = SRC_BAD;
	Destination dst;
	struct Buffer sourceData = {0};
	struct Buffer sourceMask = {0};
	struct Buffer i2cBuffer = {0};
	const char *srcExt, *dstExt;
	int eepromSize = 0;
	struct usb_dev_handle *device = NULL;
	const char *error = NULL;

	// Parse arguments...
	//
	if ( arg_nullcheck(argTable) != 0 ) {
		printf("%s: insufficient memory\n", progName);
		returnCode = 1;
		goto cleanup;
	}

	numErrors = arg_parse(argc, argv, argTable);

	if ( helpOpt->count > 0 ) {
		printf("FX2Loader Copyright (C) 2009-2011 Chris McClelland\n\nUsage: %s", progName);
		arg_print_syntax(stdout, argTable, "\n");
		printf("\nUpload code to the Cypress FX2LP.\n\n");
		arg_print_glossary(stdout, argTable,"  %-10s %s\n");
		FAIL(0);
	}

	if ( numErrors > 0 ) {
		arg_print_errors(stdout, endOpt, progName);
		printf("Try '%s --help' for more information.\n", progName);
		FAIL(1);
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
		eepromSize = atoi(eepromSizeString) * 128;  // size in bytes
		if ( eepromSize == 0 ) {
			fprintf(stderr, "You need to supply an EEPROM size in kilobytes (e.g -s eeprom:128)\n");
			FAIL(2);
		}
		src = SRC_EEPROM;
	}

	if ( src == SRC_BAD ) {
		fprintf(stderr, "Unrecognised source: %s\n", srcOpt->sval[0]);
		FAIL(3);
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
			FAIL(4);
		}
	} else {
		dst = DST_RAM;
	}

	if ( src == SRC_EEPROM || dst == DST_EEPROM || dst == DST_RAM ) {
		usbInitialise();
		if ( usbOpenDeviceVP(vpOpt->sval[0], 1, 0, 0, &device, &error) ) {
			fprintf(stderr, "%s\n", error);
			FAIL(5);
		}
	}

	// Initialise buffers...
	//
	if ( bufInitialise(&sourceData, 1024, 0x00, &error) ) {
		fprintf(stderr, "%s\n", error);
		FAIL(6);
	}
	if ( bufInitialise(&sourceMask, 1024, 0x00, &error) ) {
		fprintf(stderr, "%s\n", error);
		FAIL(7);
	}
	if ( bufInitialise(&i2cBuffer, 1024, 0x00, &error) ) {
		fprintf(stderr, "%s\n", error);
		FAIL(8);
	}

	// Read from source...
	//
	if ( src == SRC_HEXFILE ) {
		if ( bufReadFromIntelHexFile(&sourceData, &sourceMask, srcOpt->sval[0], &error) ) {
			fprintf(stderr, "%s\n", error);
			FAIL(9);
		}
	} else if ( src == SRC_BIXFILE ) {
		if ( bufAppendFromBinaryFile(&sourceData, srcOpt->sval[0], &error) ) {
			fprintf(stderr, "%s\n", error);
			FAIL(10);
		}
		if ( bufAppendConst(&sourceMask, sourceData.length, 0x01, NULL, &error) ) {
			fprintf(stderr, "%s\n", error);
			FAIL(11);
		}
	} else if ( src == SRC_IICFILE ) {
		if ( bufAppendFromBinaryFile(&i2cBuffer, srcOpt->sval[0], &error) ) {
			fprintf(stderr, "%s\n", error);
			FAIL(12);
		}
	} else if ( src == SRC_EEPROM ) {
		if ( fx2ReadEEPROM(device, eepromSize, &i2cBuffer, &error) ) {
			fprintf(stderr, "%s\n", error);
			FAIL(13);
		}
	} else {
		fprintf(stderr, "Internal error UNHANDLED_SRC\n");
		FAIL(14);
	}

	// Write to destination...
	//
	if ( dst == DST_RAM ) {
		// If the source data was I2C, write it to data/mask buffers
		//
		if ( i2cBuffer.length > 0 ) {
			if ( i2cReadPromRecords(&sourceData, &sourceMask, &i2cBuffer, &error) ) {
				fprintf(stderr, "%s\n", error);
				FAIL(15);
			}
		}

		// Write the data to RAM
		//
		if ( fx2WriteRAM(device, sourceData.data, sourceData.length, &error) ) {
			fprintf(stderr, "%s\n", error);
			FAIL(16);
		}
	} else if ( dst == DST_EEPROM ) {
		// If the source data was *not* I2C, construct I2C data from the raw data/mask buffers
		//
		if ( i2cBuffer.length == 0 ) {
			i2cInitialise(&i2cBuffer, 0x0000, 0x0000, 0x0000, CONFIG_BYTE_400KHZ);
			if ( i2cWritePromRecords(&i2cBuffer, &sourceData, &sourceMask, &error) ) {
				fprintf(stderr, "%s\n", error);
				FAIL(17);
			}
			if ( i2cFinalise(&i2cBuffer, &error) ) {
				fprintf(stderr, "%s\n", error);
				FAIL(18);
			}
		}

		// Write the I2C data to the EEPROM
		//
		if ( fx2WriteEEPROM(device, i2cBuffer.data, i2cBuffer.length, &error) ) {
			fprintf(stderr, "%s\n", error);
			FAIL(19);
		}
	} else if ( dst == DST_HEXFILE ) {
		// If the source data was I2C, write it to data/mask buffers
		//
		if ( i2cBuffer.length > 0 ) {
			if ( i2cReadPromRecords(&sourceData, &sourceMask, &i2cBuffer, &error) ) {
				fprintf(stderr, "%s\n", error);
				FAIL(20);
			}
		}

		// Write the data/mask buffers out as an I8HEX file
		//
		//dump(0x00000000, sourceMask.data, sourceMask.length);
		if ( bufWriteToIntelHexFile(&sourceData, &sourceMask, dstOpt->sval[0], 16, false, &error) ) {
			fprintf(stderr, "%s\n", error);
			FAIL(21);
		}
	} else if ( dst == DST_BIXFILE ) {
		// If the source data was I2C, write it to data/mask buffers
		//
		if ( i2cBuffer.length > 0 ) {
			if ( i2cReadPromRecords(&sourceData, &sourceMask, &i2cBuffer, &error) ) {
				fprintf(stderr, "%s\n", error);
				FAIL(22);
			}
		}

		// Write the data buffer out as a binary file
		//
		if ( bufWriteBinaryFile(&sourceData, dstOpt->sval[0], 0x00000000, sourceData.length, &error) ) {
			fprintf(stderr, "%s\n", error);
			FAIL(23);
		}
	} else if ( dst == DST_IICFILE ) {
		// If the source data was *not* I2C, construct I2C data from the raw data/mask buffers
		//
		if ( i2cBuffer.length == 0 ) {
			i2cInitialise(&i2cBuffer, 0x0000, 0x0000, 0x0000, CONFIG_BYTE_400KHZ);
			if ( i2cWritePromRecords(&i2cBuffer, &sourceData, &sourceMask, &error) ) {
				fprintf(stderr, "%s\n", error);
				FAIL(24);
			}
			if ( i2cFinalise(&i2cBuffer, &error) ) {
				fprintf(stderr, "%s\n", error);
				FAIL(25);
			}
		}

		// Write the I2C data out as a binary file
		//
		if ( bufWriteBinaryFile(&i2cBuffer, dstOpt->sval[0], 0x00000000, i2cBuffer.length, &error) ) {
			fprintf(stderr, "%s\n", error);
			FAIL(26);
		}
	} else {
		fprintf(stderr, "Internal error UNHANDLED_DST\n");
		FAIL(27);
	}

cleanup:
	if ( device ) {
		usb_release_interface(device, 0);
		usb_close(device);
	}
	if ( error ) {
		fx2FreeError(error);
	}
	if ( i2cBuffer.data ) {
		bufDestroy(&i2cBuffer);
	}
	if ( sourceMask.data ) {
		bufDestroy(&sourceMask);
	}
	if ( sourceData.data ) {
		bufDestroy(&sourceData);
	}
	arg_freetable(argTable, sizeof(argTable)/sizeof(argTable[0]));
	return returnCode;
}

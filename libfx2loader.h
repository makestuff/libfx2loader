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

/**
 * @file libfx2loader.h
 *
 * The <b>FX2Loader</b> library makes it easier to load and save an FX2LP's RAM and EEPROM firmware.
 * Its functions fall into two categories:
 * - <b>FX2 Operations</b>: Write raw bytes to RAM and read/write raw bytes to/from EEPROM.
 * - <b>I2C Operations</b>: Of course you can write arbitrary data to EEPROM if you like, but the
 * EEPROM is usually used to bootstrap the FX2LP firmware on power-on. To do this you must write
 * bytes in a specific format to EEPROM. This format is described in the <a href="http://www.cypress.com/?rID=38232">TRM section 3.4.3</a>,
 * and you can use these functions to prepare a buffer in the I2C format, or decode a buffer that is
 * already in the I2C format.
 */
#ifndef LIBFX2LOADER_H
#define LIBFX2LOADER_H

#include <makestuff.h>

#ifdef __cplusplus
extern "C" {
#endif

	// ---------------------------------------------------------------------------------------------
	// Type declarations
	// ---------------------------------------------------------------------------------------------
	/**
	 * @name Enumerations
	 * @{
	 */
	/**
	 * Return codes from the FX2 functions.
	 */
	typedef enum {
		FX2_SUCCESS = 0,  ///< The operation completed successfully.
		FX2_USB_ERR,      ///< A USB error occurred.
		FX2_BUF_ERR       ///< A buffer error occurred, probably an allocation error.
	} FX2Status;

	/**
	 * Return codes from the I2C buffer and conversion functions.
	 */
	typedef enum {
		I2C_SUCCESS = 0,           ///< The operation completed successfully.
		I2C_BUFFER_ERROR,          ///< A buffer error occurred, probably an allocation error.
		I2C_NOT_INITIALISED,       ///< The operation expected an initialised I2C buffer.
		I2C_DEST_BUFFER_NOT_EMPTY  ///< The destination buffer already has some data in it.
	} I2CStatus;
	//@}

	/**
	 * The C2 loader's DISCON flag. See TRM section 3.5.
	 */
	#define CONFIG_BYTE_DISCON (1<<6)
	/**
	 * The C2 loader's 400KHZ flag. See TRM section 3.5.
	 */
	#define CONFIG_BYTE_400KHZ (1<<0)

	// Forward-declaration of the LibUSB handle
	struct USBDevice;

	// Forward-declaration of the Buffer struct
	struct Buffer;

	// ---------------------------------------------------------------------------------------------
	// Firmware Operations
	// ---------------------------------------------------------------------------------------------
	/**
	 * @name Firmware Operations
	 * @{
	 */
	/**
	 * @brief Write a new firmware to the FX2LP's RAM and begin execution.
	 *
	 * Write a new firmware to the FX2LP's RAM. The supplied data must be valid 8051 code, and there
	 * must be less than 16KiB of it (since that's how much onboard RAM the FX2LP has).
	 *
	 * @param device The FX2LP device, previously opened using <a href="http://www.swaton.ukfsn.org/apidocs/libusbwrap_8h.html">libusbwrap</a>.
	 * @param bufPtr A pointer to the block of bytes to write to RAM.
	 * @param numBytes The number of bytes to write to RAM.
	 * @param error A pointer to a <code>char*</code> which will be set on exit to an allocated
	 *            error message if something goes wrong. Responsibility for this allocated memory
	 *            passes to the caller and must be freed with \c fx2FreeError(). If \c error is
	 *            \c NULL, no allocation is done and no message is returned, but the return code
	 *            will still be valid.
	 * @returns
	 *     - \c FX2_SUCCESS if the operation completed successfully.
	 *     - \c FX2_USB_ERR if a USB error occurred.
	 */
	DLLEXPORT(FX2Status) fx2WriteRAM(
		struct USBDevice *device, const uint8 *bufPtr, uint32 numBytes, const char **error
	) WARN_UNUSED_RESULT;

	/**
	 * @brief Write a block of data to the FX2LP's external EEPROM.
	 *
	 * Write a block of raw bytes to the FX2LP's external EEPROM (if present). Note that if you want
	 * to write bootable code, it must conform to the FX2LP's C2 loader format. You can prepare such
	 * an I2C buffer using \c i2cWritePromRecords().
	 *
	 * @param device The FX2LP device, previously opened using <a href="http://www.swaton.ukfsn.org/apidocs/libusbwrap_8h.html">libusbwrap</a>.
	 * @param bufPtr A pointer to the block of bytes to write to EEPROM.
	 * @param numBytes The number of bytes to write to EEPROM.
	 * @param error A pointer to a <code>char*</code> which will be set on exit to an allocated
	 *            error message if something goes wrong. Responsibility for this allocated memory
	 *            passes to the caller and must be freed with \c fx2FreeError(). If \c error is
	 *            \c NULL, no allocation is done and no message is returned, but the return code
	 *            will still be valid.
	 * @returns
	 *     - \c FX2_SUCCESS if the operation completed successfully.
	 *     - \c FX2_USB_ERR if a USB error occurred.
	 */
	DLLEXPORT(FX2Status) fx2WriteEEPROM(
		struct USBDevice *device, const uint8 *bufPtr, uint32 numBytes, const char **error
	) WARN_UNUSED_RESULT;

	/**
	 * @brief Read a block of data from the FX2LP's external EEPROM.
	 *
	 * Read a block of raw bytes from the FX2LP's external EEPROM (if present) into the supplied
	 * buffer. If the data on the EEPROM represents an FX2LP C2 loader, you may decode it into
	 * contiguous data and mask buffers using \c i2cReadPromRecords().
	 *
	 * @param device The FX2LP device, previously opened using <a href="http://www.swaton.ukfsn.org/apidocs/libusbwrap_8h.html">libusbwrap</a>.
	 * @param numBytes The number of bytes to read from EEPROM.
	 * @param i2cBuffer A <code><a href="http://www.swaton.ukfsn.org/apidocs/libbuffer_8h.html">Buffer</a></code>
	 *            to be populated with the data read from EEPROM.
	 * @param error A pointer to a <code>char*</code> which will be set on exit to an allocated
	 *            error message if something goes wrong. Responsibility for this allocated memory
	 *            passes to the caller and must be freed with \c fx2FreeError(). If \c error is
	 *            \c NULL, no allocation is done and no message is returned, but the return code
	 *            will still be valid.
	 * @returns
	 *     - \c FX2_SUCCESS if the operation completed successfully.
	 *     - \c FX2_USB_ERR if a USB error occurred.
	 *     - \c FX2_BUF_ERR if an allocation error occurred.
	 */
	DLLEXPORT(FX2Status) fx2ReadEEPROM(
		struct USBDevice *device, uint32 numBytes, struct Buffer *i2cBuffer, const char **error
	) WARN_UNUSED_RESULT;
	//@}

	// ---------------------------------------------------------------------------------------------
	// I2C Operations
	// ---------------------------------------------------------------------------------------------
	/**
	 * @name I2C Operations
	 * @{
	 */
	/**
	 * @brief Initialise an I2C buffer.
	 *
	 * Prepare a <code><a href="http://www.swaton.ukfsn.org/apidocs/libbuffer_8h.html">Buffer</a></code>
	 * with the FX2LP \c 0xC2 header. See TRM section 3.4.3.
	 *
	 * @param buf The <code><a href="http://www.swaton.ukfsn.org/apidocs/libbuffer_8h.html">Buffer</a></code>
	 *            to be initialised.
	 * @param vid The Vendor ID to use in the header (usually \c 0x0000 for C2 loaders).
	 * @param pid the Product ID to use in the header (usually \c 0x0000 for C2 loaders).
	 * @param did the Device ID to use in the header (usually \c 0x0000 for C2 loaders).
	 * @param configByte The configuration byte to use. See TRM section 3.5.
	 */
	DLLEXPORT(void) i2cInitialise(
		struct Buffer *buf, uint16 vid, uint16 pid, uint16 did, uint8 configByte
	);

	/**
	 * @brief Populate an I2C buffer with supplied data, respecting the supplied mask.
	 *
	 * Given a data buffer and a mask buffer, this function will write I2C records to the
	 * destination buffer ready for writing to EEPROM or to an i2c file. The mask buffer contains
	 * \c 0x00 where there is useful data at the corresponding offset into the data buffer and
	 * \c 0x01 where there is a corresponding "hole" (i.e undefined bytes) in the data buffer.
	 *
	 * The destination buffer should have been initialised with \c i2cInitialise() and should be
	 * finalised with \c i2cFinalise() after this function completes.
	 *
	 * @param destination The <code><a href="http://www.swaton.ukfsn.org/apidocs/libbuffer_8h.html">Buffer</a></code>
	 *            to write the I2C records to.
	 * @param sourceData The <code><a href="http://www.swaton.ukfsn.org/apidocs/libbuffer_8h.html">Buffer</a></code>
	 *            to read the data from.
	 * @param sourceMask The <code><a href="http://www.swaton.ukfsn.org/apidocs/libbuffer_8h.html">Buffer</a></code>
	 *            from which to read the mask for the source data.
	 * @param error A pointer to a <code>char*</code> which will be set on exit to an allocated
	 *            error message if something goes wrong. Responsibility for this allocated memory
	 *            passes to the caller and must be freed with \c fx2FreeError(). If \c error is
	 *            \c NULL, no allocation is done and no message is returned, but the return code
	 *            will still be valid.
	 * @returns
	 *     - \c I2C_SUCCESS if the operation completed successfully.
	 *     - \c I2C_NOT_INITIALISED if the supplied destination buffer was uninitialised.
	 *     - \c I2C_BUFFER_ERROR if an allocation error occurred.
	 */
	DLLEXPORT(I2CStatus) i2cWritePromRecords(
		struct Buffer *destination, const struct Buffer *sourceData,
		const struct Buffer *sourceMask, const char **error
	) WARN_UNUSED_RESULT;

	/**
	 * @brief Extract linear data and mask buffers from a supplied I2C buffer.
	 *
	 * Given a buffer containing I2C records (e.g from an i2c file or read from EEPROM), this
	 * function will populate two buffers, a data buffer and a mask buffer.  The mask buffer
	 * contains \c 0x00 where there is useful data at the corresponding offset into the data buffer
	 * and \c 0x01 where there is a corresponding "hole" (i.e undefined bytes) in the data buffer.
	 *
	 * @param destData The <code><a href="http://www.swaton.ukfsn.org/apidocs/libbuffer_8h.html">Buffer</a></code>
	 *            to write the data bytes to.
	 * @param destMask The <code><a href="http://www.swaton.ukfsn.org/apidocs/libbuffer_8h.html">Buffer</a></code>
	 *            to write the mask bytes to.
	 * @param source The <code><a href="http://www.swaton.ukfsn.org/apidocs/libbuffer_8h.html">Buffer</a></code>
	 *            from which to read the I2C records.
	 * @param error A pointer to a <code>char*</code> which will be set on exit to an allocated
	 *            error message if something goes wrong. Responsibility for this allocated memory
	 *            passes to the caller and must be freed with \c fx2FreeError(). If \c error is
	 *            \c NULL, no allocation is done and no message is returned, but the return code
	 *            will still be valid.
	 * @returns
	 *     - \c I2C_SUCCESS if the operation completed successfully.
	 *     - \c I2C_NOT_INITIALISED if the supplied I2C buffer was invalid.
	 *     - \c I2C_BUFFER_ERROR if an allocation error occurred.
	 */
	DLLEXPORT(I2CStatus) i2cReadPromRecords(
		struct Buffer *destData, struct Buffer *destMask, const struct Buffer *source,
		const char **error
	) WARN_UNUSED_RESULT;

	/**
	 * @brief Append a termination record to the end of the supplied I2C buffer.
	 *
	 * The TRM (section 3.4.3) defines a specific record terminator for the \c 0xC2 loader format,
	 * which executes a write of \c 0x00 to the CPU reset register at -c 0xE600. This function
	 * writes that record terminator, thus marking the end of the I2C records to be read on startup.
	 *
	 * @param buf The I2C <code><a href="http://www.swaton.ukfsn.org/apidocs/libbuffer_8h.html">Buffer</a></code>
	 *            to finalise.
	 * @param error A pointer to a <code>char*</code> which will be set on exit to an allocated
	 *            error message if something goes wrong. Responsibility for this allocated memory
	 *            passes to the caller and must be freed with \c fx2FreeError(). If \c error is
	 *            \c NULL, no allocation is done and no message is returned, but the return code
	 *            will still be valid.
	 * @returns
	 *     - \c I2C_SUCCESS if the operation completed successfully.
	 *     - \c I2C_NOT_INITIALISED if the supplied I2C buffer was invalid.
	 *     - \c I2C_BUFFER_ERROR if an allocation error occurred.
	 */
	DLLEXPORT(I2CStatus) i2cFinalise(
		struct Buffer *buf, const char **error
	) WARN_UNUSED_RESULT;
	//@}

	// ---------------------------------------------------------------------------------------------
	// Miscellaneous functions
	// ---------------------------------------------------------------------------------------------
	/**
	 * @name Miscellaneous Functions
	 * @{
	 */
	/**
	 * @brief Free an error allocated when one of the other functions fails.
	 *
	 * @param err An error message previously allocated by one of the other library functions.
	 */
	DLLEXPORT(void) fx2FreeError(const char *err);
	//@}

#ifdef __cplusplus
}
#endif

#endif

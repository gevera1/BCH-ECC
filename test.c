/*
 * bch_main.c
 *
 *  Created on: Feb 16, 2021
 *      Author: gevera
 */
#include "bch_functions.h"
#include "flash_functions.h"
#include "stdio.h"
#include "stdlib.h"

#include <errno.h>
//=======================================STATIC DECLARATIONS==========================================//
static XSpi Spi;
INTC InterruptController;

static int SpiFlashWaitForFlashReady(void);
static int SetupInterruptSystem(XSpi *SpiPtr);

/*
 * The instances to support the device drivers are global such that they
 * are initialized to zero each time the program runs. They could be local
 * but should at least be static so they are zeroed.
 */
static XSpi Spi;
INTC InterruptController;

/*
 * The following variables are shared between non-interrupt processing and
 * interrupt processing such that they must be global.
 */
volatile static int TransferInProgress;

/*
 * The following variable tracks any errors that occur during interrupt
 * processing.
 */
static int ErrorCount;

/*
 * Buffers used during read and write transactions.
 */
static u8 ReadBuffer[PAGE_SIZE + READ_WRITE_EXTRA_BYTES + 4];
static u8 WriteBuffer[PAGE_SIZE + READ_WRITE_EXTRA_BYTES];

static u8 test_mesg[256] = {
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12
};

static u8 err_mesg[256] = {
		0x02, 0x92, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12, 0x93, 0xA9, 0x46, 0xB0,
		0x12, 0x93, 0xA9, 0x46, 0xB0, 0x12
};

/*
 * Byte offset value written to Flash. This needs to be redefined for writing
 * different patterns of data to the Flash device.
 */
//static u8 TestByte = 0x20;

//===================================MAIN FUNCTION======================================//

int main( void )
{
	int Status, Index, ParSize;

	struct bch_control* 	BCHptr;
	unsigned int 			*err_loc;
	u8	corr_mesg[256];
	u8	*parity_array, n_err;

	err_loc = (unsigned int *) malloc( 3 * sizeof(unsigned int));

	printf("\n==================Beginning Flash Test=====================\n");
	if (INIT_FLASH(SPI_DEVICE_ID, &Spi) != XST_SUCCESS)	{
		printf("INIT ERROR\n");
		return XST_FAILURE;
	}

	printf("INIT SUCCESS!\n");

	Status = CLEAR(&Spi, 0x00);
	if (Status != XST_SUCCESS) 	{ printf("CLEAR() ERR\n"); }
	printf("CLEAR() SUCCESS!\n");

	Status = WRITE(&Spi, 0x00, 20, test_mesg);
	if (Status != XST_SUCCESS) 	{ printf("WRITE() ERR\n"); }
	printf("WRITE() SUCCESS!\n");

	Status = READ (&Spi, 0x00, 20);
	if (Status != XST_SUCCESS) 	{ printf("READ() ERR\n");  }
	printf("READ() SUCCESS!\n");

	printf("\nBegin Comparison!\n");
	for (Index = 0; Index < 20; Index++) {
		if ((u8)test_mesg[Index] != (u8) ReadBuffer[Index + 4]) {
			printf("Comparison at Index %d Failed!\n", Index);
			return XST_FAILURE;
		}
		printf("%d %d\n", test_mesg[Index], (u8) ReadBuffer[Index+READ_WRITE_EXTRA_BYTES]);
	}

	printf("\n===================Begin BCH Testing==========================\n");
	printf("Initializing BCH Control structure...\n");

	BCHptr = init_bch(M, T, 0);
	if ( BCHptr == NULL ) {
		printf("BCH INIT ERROR!\n");
		return XST_FAILURE;
	}

	printf("ECC Parity array should be of size: %d\n", BCHptr->ecc_bytes);
	ParSize = BCHptr->ecc_bytes;

	parity_array = (u8 *) malloc(ParSize);

	printf("Parity Array Before Encoding:\n");
	for (Index = 0; Index < ParSize; Index++) 	{
		parity_array[Index] = 0x00;
		printf("\t%d\n", parity_array[Index]);
	}

	encode_bch(BCHptr, test_mesg, 256, parity_array);

	printf("Parity Array After Encoding:\n");
	for (Index = 0; Index < ParSize; Index++) 	{
		printf("\t%d\n", parity_array[Index]);
	}

	n_err = decode_bch(BCHptr, err_mesg, 256, parity_array, 0, 0, err_loc);
	if (n_err == -EINVAL || n_err == -EBADMSG) {
		printf("Decode Error!\n");
		return XST_FAILURE;
	} else {
		printf("There are %d errors detected\n", n_err);
	}

	printf("\n\tMessage Original:\n");
	for (Index = 0; Index < 256; Index++) { printf("%d ", test_mesg[Index]); }

	memcpy(corr_mesg, err_mesg, 256);
	printf("Correcting Error...\n\tMessage with Error:\n");
	for (Index = 0; Index < 256; Index++) { printf("%d ", corr_mesg[Index]); }

	correct_bch(BCHptr, corr_mesg, 256, err_loc, n_err);

	printf("\n\tMessage After Correction:\n");
	for (Index = 0; Index < 256; Index++) { printf("%d ", corr_mesg[Index]); }


	printf("\n===============Tests completed Successfully!===================\n");

	/*
	* Clear the read Buffer.
	*/
	for(Index = 0; Index < PAGE_SIZE + READ_WRITE_EXTRA_BYTES; Index++) {
		ReadBuffer[Index] = 0x0;
	}

	if (CLEAR(&Spi, 0x00) != XST_SUCCESS) {
		printf("CLEAR() ERROR!\n");
		return XST_FAILURE;
	}

	free_bch(BCHptr);

	return XST_SUCCESS;
}

//==================================FLASH FUNCTIONS=====================================//

/***************************************************************************/
/**
 *
 * Performs functions that initialize the Quad SPI Flash.
 *
 * @param	DevID	the device's unique 16-bit Identification number
 * @param	spi		is a pointer to the Memory Location of the SPI device
 *
 * @return	XST_SUCCESS if successful, XST_FAILURE or XST_DEVICE_NOT_FOUND otherwise
 *
 * @note	This function should be run once
 */
int INIT_FLASH	( u16 DevID, XSpi *SpiPtr )
{
	int Status;
	XSpi_Config *ConfigPtr;

	// Initialize SPI Driver from Device ID
	ConfigPtr = XSpi_LookupConfig(SPI_DEVICE_ID);
	if (ConfigPtr == NULL)		{ return XST_DEVICE_NOT_FOUND; }

	Status = XSpi_CfgInitialize(SpiPtr, ConfigPtr, ConfigPtr->BaseAddress);
	if (Status != XST_SUCCESS) 	{ return XST_FAILURE; }

	// Connect SPI Driver to interrupt subsystem
	Status = SetupInterruptSystem(SpiPtr);
	if (Status != XST_SUCCESS) 	{ return XST_FAILURE; }

	// Setup handler for SPI in context of interrupts
	XSpi_SetStatusHandler(SpiPtr, SpiPtr, (XSpi_StatusHandler)SpiHandler);

	// Set SPI device as master and in manual slave select mode
	Status = XSpi_SetOptions(SpiPtr, XSP_MASTER_OPTION | XSP_MANUAL_SSELECT_OPTION);
	if (Status != XST_SUCCESS) 	{ return XST_FAILURE; }

	// Select the QUAD flash device on SPI bus
	Status = XSpi_SetSlaveSelect(SpiPtr, SPI_SELECT);
	if (Status != XST_SUCCESS) 	{ return XST_FAILURE; }

	// Start SPI Driver
	XSpi_Start(SpiPtr);

	return XST_SUCCESS;
}

int CLEAR	( XSpi *SpiPtr, u32 Addr )
{
	if (SpiFlashWriteEnable(SpiPtr) != XST_SUCCESS) {
		printf("Write Enable Error!\n");
		return XST_FAILURE;
	}

	if (SpiFlashSectorErase(SpiPtr, Addr) != XST_SUCCESS) {
		printf("Sector Erase Error!\n");
		return XST_FAILURE;
	}
	return XST_SUCCESS;
}

/***************************************************************************/
/**
 *
 * Wrapper function to perform Write operation on SPI Flash memory
 *
 * @param	SpiPtr		the device's unique 16-bit Identification number
 * @param	Addr		is a pointer to the Memory Location of the SPI device
 * @param 	ByteCount	is the number of Bytes being written into Flash
 *
 * @return	XST_SUCCESS if successful, XST_FAILURE otherwise
 *
 * @note	encapsulates SpiFlashWriteEnable() function
 */
int WRITE	( XSpi *SpiPtr, u32 Addr, u32 ByteCount, u8 *mesg )
{
	int Status;

	/* Sanity Check */
	if (ByteCount > 0xFFFFFFFF)	{
		printf("Incorrect ByteCount!\n");
		return XST_FAILURE;
	}

	Status = SpiFlashWriteEnable(SpiPtr);
	if (Status != XST_SUCCESS) {
		printf("Write Enable Error!\n");
		return XST_FAILURE;
	}

	Status = SpiFlashWrite(SpiPtr, Addr, ByteCount, COMMAND_QUAD_WRITE, mesg);
	if (Status != XST_SUCCESS) {
		printf("Flash Write Error!\n");
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/***************************************************************************/
/**
 *
 * Wrapper function to perform Read operation on SPI Flash memory
 *
 * @param	SpiPtr		the device's unique 16-bit Identification number
 * @param	Addr		is a pointer to the Memory Location of the SPI device
 * @param 	ByteCount	is the number of Bytes being written into Flash
 *
 * @return	XST_SUCCESS if successful, XST_FAILURE otherwise
 *
 * @note	None
 */
int READ	( XSpi *SpiPtr, u32 Addr, u32 ByteCount )
{
	int Status, Index;

	/* Sanity Check */
	if (ByteCount > 0xFFFFFFFF)	{
		printf("Incorrect ByteCount!\n");
		return XST_FAILURE;
	}

	for (Index = 0; Index < ByteCount + READ_WRITE_EXTRA_BYTES; Index++) {
		ReadBuffer[Index] = 0x00;
	}

	Status = SpiFlashRead(SpiPtr, Addr, ByteCount, COMMAND_RANDOM_READ);
	if (Status != XST_SUCCESS) {
		printf("Flash Read Error!\n");
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function enables writes to the Numonyx Serial Flash memory.
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		None
*
******************************************************************************/
int SpiFlashWriteEnable(XSpi *SpiPtr)
{
	int Status;

	/*
	 * Wait while the Flash is busy.
	 */
	Status = SpiFlashWaitForFlashReady();
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Prepare the WriteBuffer.
	 */
	WriteBuffer[BYTE1] = COMMAND_WRITE_ENABLE;

	/*
	 * Initiate the Transfer.
	 */
	TransferInProgress = TRUE;
	Status = XSpi_Transfer(SpiPtr, WriteBuffer, NULL,
				WRITE_ENABLE_BYTES);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait till the Transfer is complete and check if there are any errors
	 * in the transaction..
	 */
	while(TransferInProgress);
	if(ErrorCount != 0) {
		ErrorCount = 0;
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function writes the data to the specified locations in the Numonyx Serial
* Flash memory.
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
* @param	Addr is the address in the Buffer, where to write the data.
* @param	ByteCount is the number of bytes to be written.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		None
*
******************************************************************************/
int SpiFlashWrite(XSpi *SpiPtr, u32 Addr, u32 ByteCount, u8 WriteCmd, u8 *mesg )
{
	u32 Index;
	int Status;

	/* Sanity Check */
	if (sizeof(mesg) > ByteCount) 	{ return XST_FAILURE; }

	Status = SpiFlashWaitForFlashReady();
	if (Status != XST_SUCCESS)		{ return XST_FAILURE; }

	WriteBuffer[BYTE1]	= WriteCmd;
	WriteBuffer[BYTE2]	= (u8) (Addr >> 16);
	WriteBuffer[BYTE3]	= (u8) (Addr >> 8);
	WriteBuffer[BYTE4]	= (u8) (Addr);


	for (Index = 4; Index < ByteCount + READ_WRITE_EXTRA_BYTES; Index++) {
		WriteBuffer[Index] = (u8) mesg[Index - 4];
	}

	/*
	 * Initiate the Transfer.
	 */
	TransferInProgress = TRUE;
	Status = XSpi_Transfer(SpiPtr, WriteBuffer, NULL,
				(ByteCount + READ_WRITE_EXTRA_BYTES));
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	while(TransferInProgress);
	if(ErrorCount != 0) {
		ErrorCount = 0;
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function reads the data from the Numonyx Serial Flash Memory
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
* @param	Addr is the starting address in the Flash Memory from which the
*		data is to be read.
* @param	ByteCount is the number of bytes to be read.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		None
*
******************************************************************************/
int SpiFlashRead(XSpi *SpiPtr, u32 Addr, u32 ByteCount, u8 ReadCmd)
{
	int Status;

	/*
	 * Wait while the Flash is busy.
	 */
	Status = SpiFlashWaitForFlashReady();
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Prepare the WriteBuffer.
	 */
	WriteBuffer[BYTE1] = ReadCmd;
	WriteBuffer[BYTE2] = (u8) (Addr >> 16);
	WriteBuffer[BYTE3] = (u8) (Addr >> 8);
	WriteBuffer[BYTE4] = (u8) Addr;

	if (ReadCmd == COMMAND_DUAL_READ) {
		ByteCount += DUAL_READ_DUMMY_BYTES;
	} else if (ReadCmd == COMMAND_DUAL_IO_READ) {
		ByteCount += DUAL_READ_DUMMY_BYTES;
	} else if (ReadCmd == COMMAND_QUAD_IO_READ) {
		ByteCount += QUAD_IO_READ_DUMMY_BYTES;
	} else if (ReadCmd==COMMAND_QUAD_READ) {
		ByteCount += QUAD_READ_DUMMY_BYTES;
	}

	/*
	 * Initiate the Transfer.
	 */
	TransferInProgress = TRUE;
	Status = XSpi_Transfer( SpiPtr, WriteBuffer, ReadBuffer,
				(ByteCount + READ_WRITE_EXTRA_BYTES));
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait till the Transfer is complete and check if there are any errors
	 * in the transaction.
	 */
	while(TransferInProgress);
	if(ErrorCount != 0) {
		ErrorCount = 0;
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function erases the entire contents of the Numonyx Serial Flash device.
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		The erased bytes will read as 0xFF.
*
******************************************************************************/
int SpiFlashBulkErase(XSpi *SpiPtr)
{
	int Status;

	/*
	 * Wait while the Flash is busy.
	 */
	Status = SpiFlashWaitForFlashReady();
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Prepare the WriteBuffer.
	 */
	WriteBuffer[BYTE1] = COMMAND_BULK_ERASE;

	/*
	 * Initiate the Transfer.
	 */
	TransferInProgress = TRUE;
	Status = XSpi_Transfer(SpiPtr, WriteBuffer, NULL,
						BULK_ERASE_BYTES);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait till the Transfer is complete and check if there are any errors
	 * in the transaction..
	 */
	while(TransferInProgress);
	if(ErrorCount != 0) {
		ErrorCount = 0;
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function erases the contents of the specified Sector in the Numonyx
* Serial Flash device.
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
* @param	Addr is the address within a sector of the Buffer, which is to
*		be erased.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		The erased bytes will be read back as 0xFF.
*
******************************************************************************/
int SpiFlashSectorErase(XSpi *SpiPtr, u32 Addr)
{
	int Status;

	/*
	 * Wait while the Flash is busy.
	 */
	Status = SpiFlashWaitForFlashReady();
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Prepare the WriteBuffer.
	 */
	WriteBuffer[BYTE1] = COMMAND_SECTOR_ERASE;
	WriteBuffer[BYTE2] = (u8) (Addr >> 16);
	WriteBuffer[BYTE3] = (u8) (Addr >> 8);
	WriteBuffer[BYTE4] = (u8) (Addr);

	/*
	 * Initiate the Transfer.
	 */
	TransferInProgress = TRUE;
	Status = XSpi_Transfer(SpiPtr, WriteBuffer, NULL,
					SECTOR_ERASE_BYTES);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait till the Transfer is complete and check if there are any errors
	 * in the transaction..
	 */
	while(TransferInProgress);
	if(ErrorCount != 0) {
		ErrorCount = 0;
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function reads the Status register of the Numonyx Flash.
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		The status register content is stored at the second byte
*		pointed by the ReadBuffer.
*
******************************************************************************/
int SpiFlashGetStatus(XSpi *SpiPtr)
{
	int Status;

	/*
	 * Prepare the Write Buffer.
	 */
	WriteBuffer[BYTE1] = COMMAND_STATUSREG_READ;

	/*
	 * Initiate the Transfer.
	 */
	TransferInProgress = TRUE;
	Status = XSpi_Transfer(SpiPtr, WriteBuffer, ReadBuffer,
						STATUS_READ_BYTES);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait till the Transfer is complete and check if there are any errors
	 * in the transaction..
	 */
	while(TransferInProgress);
	if(ErrorCount != 0) {
		ErrorCount = 0;
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}



/*****************************************************************************/
/**
*
* This function waits till the Numonyx serial Flash is ready to accept next
* command.
*
* @param	None
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		This function reads the status register of the Buffer and waits
*.		till the WIP bit of the status register becomes 0.
*
******************************************************************************/
int SpiFlashWaitForFlashReady(void)
{
	int Status;
	u8 StatusReg;

	while(1) {

		/*
		 * Get the Status Register. The status register content is
		 * stored at the second byte pointed by the ReadBuffer.
		 */
		Status = SpiFlashGetStatus(&Spi);
		if(Status != XST_SUCCESS) {
			return XST_FAILURE;
		}

		/*
		 * Check if the flash is ready to accept the next command.
		 * If so break.
		 */
		StatusReg = ReadBuffer[1];
		if((StatusReg & FLASH_SR_IS_READY_MASK) == 0) {
			break;
		}
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function is the handler which performs processing for the SPI driver.
* It is called from an interrupt context such that the amount of processing
* performed should be minimized. It is called when a transfer of SPI data
* completes or an error occurs.
*
* This handler provides an example of how to handle SPI interrupts and
* is application specific.
*
* @param	CallBackRef is the upper layer callback reference passed back
*		when the callback function is invoked.
* @param	StatusEvent is the event that just occurred.
* @param	ByteCount is the number of bytes transferred up until the event
*		occurred.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void SpiHandler(void *CallBackRef, u32 StatusEvent, unsigned int ByteCount)
{
	/*
	 * Indicate the transfer on the SPI bus is no longer in progress
	 * regardless of the status event.
	 */
	TransferInProgress = FALSE;

	/*
	 * If the event was not transfer done, then track it as an error.
	 */
	if (StatusEvent != XST_SPI_TRANSFER_DONE) {
		ErrorCount++;
	}
}

/*****************************************************************************/
/**
*
* This function setups the interrupt system such that interrupts can occur
* for the Spi device. This function is application specific since the actual
* system may or may not have an interrupt controller. The Spi device could be
* directly connected to a processor without an interrupt controller.  The
* user should modify this function to fit the application.
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
*
* @return	XST_SUCCESS if successful, otherwise XST_FAILURE.
*
* @note		None
*
******************************************************************************/
static int SetupInterruptSystem(XSpi *SpiPtr)
{

	int Status;

#ifdef XPAR_INTC_0_DEVICE_ID
	/*
	 * Initialize the interrupt controller driver so that
	 * it's ready to use, specify the device ID that is generated in
	 * xparameters.h
	 */
	Status = XIntc_Initialize(&InterruptController, INTC_DEVICE_ID);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Connect a device driver handler that will be called when an
	 * interrupt for the device occurs, the device driver handler
	 * performs the specific interrupt processing for the device
	 */
	Status = XIntc_Connect(&InterruptController,
				SPI_INTR_ID,
				(XInterruptHandler)XSpi_InterruptHandler,
				(void *)SpiPtr);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Start the interrupt controller such that interrupts are enabled for
	 * all devices that cause interrupts, specific real mode so that
	 * the SPI can cause interrupts thru the interrupt controller.
	 */
	Status = XIntc_Start(&InterruptController, XIN_REAL_MODE);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Enable the interrupt for the SPI.
	 */
	XIntc_Enable(&InterruptController, SPI_INTR_ID);

#else /* SCUGIC */
	XScuGic_Config *IntcConfig;

	/*
	 * Initialize the interrupt controller driver so that it is ready to
	 * use.
	 */
	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL == IntcConfig) {
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(&InterruptController, IntcConfig,
					IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}


	XScuGic_SetPriorityTriggerType(&InterruptController, SPI_INTR_ID,
					0xA0, 0x3);


	/*
	 * Connect the interrupt handler that will be called when an
	 * interrupt occurs for the device.
	 */
	Status = XScuGic_Connect(&InterruptController, SPI_INTR_ID,
				 (Xil_InterruptHandler)XSpi_InterruptHandler,
					 (void *)SpiPtr);
	if (Status != XST_SUCCESS) {
		return Status;
	}

	/*
	 * Enable the interrupt for the SPI device.
	 */
	XScuGic_Enable(&InterruptController, SPI_INTR_ID);


#endif

	/*
	 * Initialize the exception table.
	 */
	Xil_ExceptionInit();

	/*
	 * Register the interrupt controller handler with the exception table.
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
				(Xil_ExceptionHandler)INTC_HANDLER,
				&InterruptController);

	/*
	 * Enable non-critical exceptions.
	 */
	Xil_ExceptionEnable();

	return XST_SUCCESS;
}



/*
 * flash_functions.h
 *
 *  Created on: Feb 16, 2021
 *      Author: gevera
 */

#ifndef SRC_FLASH_FUNCTIONS_H_
#define SRC_FLASH_FUNCTIONS_H_

/***************************** Include Files *********************************/

#include "xparameters.h"	/* EDK generated parameters */
#include "xspi.h"		/* SPI device driver */
#include "xil_exception.h"

#ifdef XPAR_INTC_0_DEVICE_ID
#include "xintc.h"
#include <stdio.h>
#else  /* SCU GIC */
#include "xscugic.h"
#include "xil_printf.h"
#endif


/************************** Constant Definitions *****************************/

/*
 * The following constants map to the XPAR parameters created in the
 * xparameters.h file. They are defined here such that a user can easily
 * change all the needed parameters in one place.
 */
#define SPI_DEVICE_ID			XPAR_SPI_0_DEVICE_ID

#ifdef XPAR_INTC_0_DEVICE_ID
 #define INTC_DEVICE_ID		XPAR_INTC_0_DEVICE_ID
 #define SPI_INTR_ID		XPAR_INTC_0_SPI_0_VEC_ID
#else
#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID
 #define SPI_INTR_ID		XPAR_FABRIC_SPI_0_VEC_ID
#endif /* XPAR_INTC_0_DEVICE_ID */


#ifdef XPAR_INTC_0_DEVICE_ID
#define INTC		    static XIntc
#define INTC_HANDLER	XIntc_InterruptHandler
#else
#define INTC			static XScuGic
#define INTC_HANDLER	XScuGic_InterruptHandler
#endif /* XPAR_INTC_0_DEVICE_ID */

/*
 * The following constant defines the slave select signal that is used to
 * to select the Flash device on the SPI bus, this signal is typically
 * connected to the chip select of the device.
 */
#define SPI_SELECT 			0x01

/*
 * Definitions of the commands shown in this example.
 */
#define COMMAND_PAGE_PROGRAM		0x02 /* Page Program command */
#define COMMAND_QUAD_WRITE		0x32 /* Quad Input Fast Program */
#define COMMAND_RANDOM_READ		0x03 /* Random read command */
#define COMMAND_DUAL_READ		0x3B /* Dual Output Fast Read */
#define COMMAND_DUAL_IO_READ		0xBB /* Dual IO Fast Read */
#define COMMAND_QUAD_READ		0x6B /* Quad Output Fast Read */
#define COMMAND_QUAD_IO_READ		0xEB /* Quad IO Fast Read */
#define	COMMAND_WRITE_ENABLE		0x06 /* Write Enable command */
#define COMMAND_SECTOR_ERASE		0xD8 /* Sector Erase command */
#define COMMAND_BULK_ERASE		0xC7 /* Bulk Erase command */
#define COMMAND_STATUSREG_READ		0x05 /* Status read command */

/**
 * This definitions specify the EXTRA bytes in each of the command
 * transactions. This count includes Command byte, address bytes and any
 * don't care bytes needed.
 */
#define READ_WRITE_EXTRA_BYTES		4 /* Read/Write extra bytes */
#define	WRITE_ENABLE_BYTES		1 /* Write Enable bytes */
#define SECTOR_ERASE_BYTES		4 /* Sector erase extra bytes */
#define BULK_ERASE_BYTES		1 /* Bulk erase extra bytes */
#define STATUS_READ_BYTES		2 /* Status read bytes count */
#define STATUS_WRITE_BYTES		2 /* Status write bytes count */

/*
 * Flash not busy mask in the status register of the flash device.
 */
#define FLASH_SR_IS_READY_MASK		0x01 /* Ready mask */

/*
 * Number of bytes per page in the flash device.
 */
#define PAGE_SIZE			256

/*
 * Address of the page to perform Erase, Write and Read operations.
 */
#define FLASH_TEST_ADDRESS		0x00

/*
 * Byte Positions.
 */
#define BYTE1				0 /* Byte 1 position */
#define BYTE2				1 /* Byte 2 position */
#define BYTE3				2 /* Byte 3 position */
#define BYTE4				3 /* Byte 4 position */
#define BYTE5				4 /* Byte 5 position */
#define BYTE6				5 /* Byte 6 position */
#define BYTE7				6 /* Byte 7 position */
#define BYTE8				7 /* Byte 8 position */

/*
 * The following definitions specify the number of dummy bytes to ignore in the
 * data read from the flash, through various Read commands. This is apart from
 * the dummy bytes returned in reponse to the command and address transmitted.
 */
/*
 * After transmitting Dual Read command and address on DIO0,the quad spi device
 * configures DIO0 and DIO1 in input mode and receives data on both DIO0 and
 * DIO1 for 8 dummy clock cycles. So we end up with 16 dummy bits in DRR. The
 * same logic applies Quad read command, so we end up with 4 dummy bytes in
 * that case.
 */
#define DUAL_READ_DUMMY_BYTES		2
#define QUAD_READ_DUMMY_BYTES		4

#define DUAL_IO_READ_DUMMY_BYTES	2
#define QUAD_IO_READ_DUMMY_BYTES	5


/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

int SpiFlashWriteEnable(XSpi *SpiPtr);
int SpiFlashWrite(XSpi *SpiPtr, u32 Addr, u32 ByteCount, u8 WriteCmd, u8 *mesg);
int SpiFlashRead(XSpi *SpiPtr, u32 Addr, u32 ByteCount, u8 ReadCmd);
int SpiFlashBulkErase(XSpi *SpiPtr);
int SpiFlashSectorErase(XSpi *SpiPtr, u32 Addr);
int SpiFlashGetStatus(XSpi *SpiPtr);
int SpiFlashQuadEnable(XSpi *SpiPtr);
int SpiFlashEnableHPM(XSpi *SpiPtr);
void SpiHandler(void *CallBackRef, u32 StatusEvent, unsigned int ByteCount);

/* User-Defined functions */
int INIT_FLASH	( u16 DevID, XSpi *SpiPtr );
int CLEAR		( XSpi *SpiPtr, u32 Addr );
int WRITE	( XSpi *SpiPtr, u32 Addr, u32 ByteCount, u8 *mesg );
int READ	( XSpi *SpiPtr, u32 Addr, u32 ByteCount );
  
/************************** Variable Definitions *****************************/


#endif /* SRC_FLASH_FUNCTIONS_H_ */

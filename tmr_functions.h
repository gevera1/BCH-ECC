/*
 * tmr_functions.h
 *
 *  Created on: Feb 22, 2021
 *      Author: gevera
 */

#ifndef TMR_FUNCTIONS_H_
#define TMR_FUNCTIONS_H_

/***************************** Include Files *********************************/

#include "xparameters.h"
#include "xtmrctr.h"
#include "xintc.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "stdio.h"

/************************** Constant Definitions *****************************/

/*
 * The following constants map to the XPAR parameters created in the
 * xparameters.h file. They are only defined here such that a user can easily
 * change all the needed parameters in one place.
 */
#define TMRCTR_DEVICE_ID	XPAR_TMRCTR_0_DEVICE_ID
#define INTC_DEVICE_ID		XPAR_INTC_0_DEVICE_ID
#define TMRCTR_INTERRUPT_ID	XPAR_INTC_0_TMRCTR_0_VEC_ID

/*
 * The following constant determines which timer counter of the device that is
 * used for this example, there are currently 2 timer counters in a device
 * and this example uses the first one, 0, the timer numbers are 0 based
 */
#define TIMER_CNTR_0	 0

/**************************** Type Definitions *******************************/


/***************** Macros (Inline Functions) Definitions *********************/


/************************** Function Prototypes ******************************/

void TimerCounterHandler(void *CallBackRef, u8 TmrCtrNumber);

void TmrCtrDisableIntr(XIntc* IntcInstancePtr, u16 IntrId);

void TmrCtr_FastHandler(void) __attribute__ ((fast_interrupt));

int TimerInit ( XIntc* IntcInstancePtr,
		XTmrCtr* TmrCtrInstancePtr,
		u16 DeviceId,
		u16 IntrId,
		u8 TmrCtrNumber );
int TimerExit ( XIntc* IntcInstancePtr,
		XTmrCtr* TmrCtrInstancePtr,
		u16 DeviceId,
		u8 TmrCtrNumber );

#define RESET_VALUE	 0xE0000000	/* 43 seconds from 0x0 */

#endif /* TMR_FUNCTIONS_H_ */

/*******************************************************************************
  Filename:       PGA450.c
  Revised:        $Date: 2016-03-01 16:46:59
  Revision:       $Revision: 44594 $

  Description:    This file contains the PGA450 driver
*******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include <string.h>

#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Queue.h>

#include "hci_tl.h"
#include "gatt.h"
#include "gapgattserver.h"
#include "gattservapp.h"
#include "devinfoservice.h"
#include "simpleGATTprofile.h"

#include "peripheral.h"
#include "gapbondmgr.h"

#include "osal_snv.h"
#include "ICallBleAPIMSG.h"

#include "util.h"
#include "board_lcd.h"
#include "board_key.h"
#include "Board.h"

#include "PGA450.h"

#include <ti/drivers/lcd/LCDDogm1286.h>

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * PUBLIC VARIABLES
 */
PGA450_W_Package PGA450_Write;
PGA450_R_Package PGA450_Read;
SPI_Handle SPI_PGA450_Handle;
/*********************************************************************
 * LOCAL FUNCTIONS
 */
const Bandpass_Coefficient Bandpass_Coefficient_Table[32][4] = {
		//         BW=4k               BW=5k                   BW=6k                  BW=7k
	    {{0xF54A,0xF9A5,0x032D},{0xF48B,0xF815,0x03F6},{0xF3CD,0xF687,0x04BD},{0xF311,0xF4FB,0x0582}},//39k
		{{0xF4E6,0xF9A5,0x032D},{0xF427,0xF815,0x03F6},{0xF36A,0xF687,0x04BD},{0xF2AE,0xF4FB,0x0582}},//40k
		{{0xF480,0xF9A5,0x032D},{0xF3C1,0xF815,0x03F6},{0xF304,0xF687,0x04BD},{0xF249,0xF4FB,0x0582}},//41k
		{{0xF417,0xF9A5,0x032D},{0xF358,0xF815,0x03F6},{0xF29C,0xF687,0x04BD},{0xF1E1,0xF4FB,0x0582}},//42k
		{{0xF3AC,0xF9A5,0x032D},{0xF2ED,0xF815,0x03F6},{0xF231,0xF687,0x04BD},{0xF176,0xF4FB,0x0582}},//43k
		{{0xF33E,0xF9A5,0x032D},{0xF280,0xF815,0x03F6},{0xF1C4,0xF687,0x04BD},{0xF10A,0xF4FB,0x0582}},//44k
		{{0xF2CE,0xF9A5,0x032D},{0xF210,0xF815,0x03F6},{0xF154,0xF687,0x04BD},{0xF09A,0xF4FB,0x0582}},//45k
		{{0xF25B,0xF9A5,0x032D},{0xF19E,0xF815,0x03F6},{0xF0E2,0xF687,0x04BD},{0xF029,0xF4FB,0x0582}},//46k
		{{0xF1E6,0xF9A5,0x032D},{0xF129,0xF815,0x03F6},{0xF06E,0xF687,0x04BD},{0xEFB5,0xF4FB,0x0582}},//47k
		{{0xF16E,0xF9A5,0x032D},{0xF0B2,0xF815,0x03F6},{0xEFF7,0xF687,0x04BD},{0xEF3E,0xF4FB,0x0582}},//48k
	    {{0xF0F4,0xF9A5,0x032D},{0xF038,0xF815,0x03F6},{0xEF7E,0xF687,0x04BD},{0xEEC5,0xF4FB,0x0582}},//49k
		{{0xF078,0xF9A5,0x032D},{0xEFBC,0xF815,0x03F6},{0xEF02,0xF687,0x04BD},{0xEE4A,0xF4FB,0x0582}},//50k
		{{0xEFF9,0xF9A5,0x032D},{0xEF3E,0xF815,0x03F6},{0xEE84,0xF687,0x04BD},{0xEDCC,0xF4FB,0x0582}},//51k
		{{0xEF78,0xF9A5,0x032D},{0xEEBD,0xF815,0x03F6},{0xEE03,0xF687,0x04BD},{0xED4C,0xF4FB,0x0582}},//52k
		{{0xEEF4,0xF9A5,0x032D},{0xEE39,0xF815,0x03F6},{0xED80,0xF687,0x04BD},{0xECC9,0xF4FB,0x0582}},//53k
		{{0xEE6E,0xF9A5,0x032D},{0xEDB4,0xF815,0x03F6},{0xECFB,0xF687,0x04BD},{0xEC44,0xF4FB,0x0582}},//54k
		{{0xEDE5,0xF9A5,0x032D},{0xED2B,0xF815,0x03F6},{0xEC73,0xF687,0x04BD},{0xEBBD,0xF4FB,0x0582}},//55k
		{{0xED5A,0xF9A5,0x032D},{0xECA1,0xF815,0x03F6},{0xEBE9,0xF687,0x04BD},{0xEB33,0xF4FB,0x0582}},//56k
		{{0xECCD,0xF9A5,0x032D},{0xEC14,0xF815,0x03F6},{0xEB5D,0xF687,0x04BD},{0xEAA7,0xF4FB,0x0582}},//57k
		{{0xEC3D,0xF9A5,0x032D},{0xEB85,0xF815,0x03F6},{0xEACE,0xF687,0x04BD},{0xEA19,0xF4FB,0x0582}},//58k
	    {{0xEBAB,0xF9A5,0x032D},{0xEAF3,0xF815,0x03F6},{0xEA3D,0xF687,0x04BD},{0xE988,0xF4FB,0x0582}},//59k
		{{0xEB16,0xF9A5,0x032D},{0xEA5F,0xF815,0x03F6},{0xE9A9,0xF687,0x04BD},{0xE8F5,0xF4FB,0x0582}},//60k
		{{0xEA7F,0xF9A5,0x032D},{0xE9C8,0xF815,0x03F6},{0xE913,0xF687,0x04BD},{0xE85F,0xF4FB,0x0582}},//61k
		{{0xE9E6,0xF9A5,0x032D},{0xE930,0xF815,0x03F6},{0xE87B,0xF687,0x04BD},{0xE7C7,0xF4FB,0x0582}},//62k
		{{0xE94B,0xF9A5,0x032D},{0xE894,0xF815,0x03F6},{0xE7E0,0xF687,0x04BD},{0xE72D,0xF4FB,0x0582}},//63k
		{{0xE8AD,0xF9A5,0x032D},{0xE7F7,0xF815,0x03F6},{0xE743,0xF687,0x04BD},{0xE691,0xF4FB,0x0582}},//64k
		{{0xE80C,0xF9A5,0x032D},{0xE757,0xF815,0x03F6},{0xE6A4,0xF687,0x04BD},{0xE5F2,0xF4FB,0x0582}},//65k
		{{0xE769,0xF9A5,0x032D},{0xE6B5,0xF815,0x03F6},{0xE602,0xF687,0x04BD},{0xE551,0xF4FB,0x0582}},//66k
		{{0xE6C4,0xF9A5,0x032D},{0xE610,0xF815,0x03F6},{0xE55E,0xF687,0x04BD},{0xE4AD,0xF4FB,0x0582}},//67k
		{{0xE61D,0xF9A5,0x032D},{0xE569,0xF815,0x03F6},{0xE4B8,0xF687,0x04BD},{0xE407,0xF4FB,0x0582}},//68k
		{{0xE573,0xF9A5,0x032D},{0xE4C0,0xF815,0x03F6},{0xE40F,0xF687,0x04BD},{0xE35F,0xF4FB,0x0582}},//69k
		{{0xE4C7,0xF9A5,0x032D},{0xE415,0xF815,0x03F6},{0xE364,0xF687,0x04BD},{0xE2B5,0xF4FB,0x0582}} //70k
	};


/*********************************************************************
 * PROFILE CALLBACKS
 */


/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SimpleBLEPeripheral_createTask
 *
 * @brief   Task creation function for the Simple BLE Peripheral.
 *
 * @param   None.
 *
 * @return  None.
 */
void PGA450_Transfer(SPI_Handle Handle, PGA450_W_Package *txbuf, PGA450_R_Package *rxbuf)
{
  // SPI_Transaction spiTransaction;
  uint8 tbuf[4];
  uint8 rbuf[4];
  tbuf[0] = txbuf->cmd;
  tbuf[1] = txbuf->address;
  tbuf[2] = txbuf->data0;
  tbuf[3] = txbuf->data1;

  SPI_Transaction spiTransaction;
  spiTransaction.arg = NULL;
  spiTransaction.txBuf = tbuf;
  spiTransaction.rxBuf = rbuf;
  if(txbuf->cmd==CMD_OTP_write ||
     txbuf->cmd==CMD_OTP_read ||
	 txbuf->cmd==CMD_External_RAM_write ||
	 txbuf->cmd==CMD_DEV_RAM_write ||
	 txbuf->cmd==CMD_DEV_RAM_read
	 )
  {
	  spiTransaction.count = 4;
  }
  else
  {
	  spiTransaction.count = 3;
  }

  SPI_transfer(Handle, &spiTransaction);

  rxbuf->CheckByte = rbuf[0];
  rxbuf->data = rbuf[1];
}



void PGA450_Reset(void)
{
	PGA450_Write.cmd = CMD_TEST_write;
	PGA450_Write.address = 0x2F;
	PGA450_Write.data0 = 1;
	PGA450_Transfer(SPI_PGA450_Handle, &PGA450_Write, &PGA450_Read);
}

void PGA450_Release(void)
{

	PGA450_Write.cmd = CMD_TEST_write;
	PGA450_Write.address = 0x2F;
	PGA450_Write.data0 = 0;
	PGA450_Transfer(SPI_PGA450_Handle, &PGA450_Write, &PGA450_Read);
}

uint8 Read_ESFR(uint8 addr, uint8* data)
{
	PGA450_W_Package Write;
	PGA450_R_Package Read;
	uint8 status = 1;

	Write.cmd = CMD_ESFR_read;
	Write.address = addr;
	PGA450_Transfer(SPI_PGA450_Handle, &Write, &Read);

	Write.cmd = CMD_ESFR_read;
	Write.address = addr;
	PGA450_Transfer(SPI_PGA450_Handle, &Write, &Read);
	if(Read.CheckByte == 0x02)
	{
		*data = Read.data;
		status = 0;
	}
	else
		status = 1;
	return status;
}

uint8 Write_ESFR(uint8 addr, uint8 data)
{
	PGA450_W_Package Write;
	PGA450_R_Package Read;
	uint8 temp;
	uint8 status;

	Write.cmd = CMD_ESFR_write;
	Write.address = addr;
	Write.data0 = data;
	PGA450_Transfer(SPI_PGA450_Handle, &Write, &Read);

	if(Read_ESFR(addr,&temp) == 0)
	{
		if(temp == data) status = 0;
		else status =1;
	}
	else if(Read_ESFR(addr,&temp) == 0)
	{
		if(temp == data) status = 0;
		else status =1;
	}
	else
		status =1;
	return status;
}

void Set_Bandpass_Coefficient(uint8 CF, uint8 BW )
{
	uint16 A2,A3,B1;

	A2 = Bandpass_Coefficient_Table[CF-39][BW-4].A2;
	A3 = Bandpass_Coefficient_Table[CF-39][BW-4].A3;
	B1 = Bandpass_Coefficient_Table[CF-39][BW-4].B1;
	//write A2
	Write_ESFR(ADDR_BPF_A2_MSB,A2 >> 8);
	Write_ESFR(ADDR_BPF_A2_LSB,A2 & 0xFF);
	//write A3
	Write_ESFR(ADDR_BPF_A3_MSB,A3 >> 8);
	Write_ESFR(ADDR_BPF_A3_LSB,A3 & 0xFF);
	//write B1
	Write_ESFR(ADDR_BPF_B1_MSB,B1 >> 8);
	Write_ESFR(ADDR_BPF_B1_LSB,B1 & 0xFF);
}

/*********************************************************************
*********************************************************************/

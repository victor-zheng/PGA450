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

volatile uint8 FIFO_Buffer[768];

PGA450_Parameter PAG450_Para = {
		{PARA_MODE_PUSH_PULL,PARA_VOLTAGE_5P0V,10,10,40000,0,0,0,0,0},
		PARA_LNA_GAIN_64dB,
		PARA_FREQ_40kHZ,
		PARA_BPF_BW_4kHz,
		30,
		PARA_LPF_4P0kHz,
		PARA_FIFO_8BIT_UP,
		100
};
/*********************************************************************
 * LOCAL FUNCTIONS
 */
const Lowpass_Coefficient Lowpass_Coefficient_Table[26][8] = {
		//   F=0.5k          F=1.0k         F=1.5k         F=2.0k          F=2.5k          F=3.0k          F=3.5k          F=4.0k
	    {{0x7652,0x4D7},{0x6D53,0x957},{0x64E8,0xD8C},{0x5CFF,0x1180},{0x5587,0x153D},{0x4E70,0x18C8},{0x47AF,0x1C29},{0x4138,0x1F64}},//0x19
		{{0x75F3,0x506},{0x6CA1,0x9B0},{0x63EF,0xE09},{0x5BC6,0x121D},{0x5415,0x15F6},{0x4CCA,0x199B},{0x45D9,0x1D14},{0x3F35,0x2066}},//0x1A
		{{0x7594,0x536},{0x6BF0,0xA08},{0x62F7,0xE84},{0x5A90,0x12B8},{0x52A6,0x16AD},{0x4B28,0x1A6C},{0x4408,0x1DFC},{0x3D38,0x2164}},//0x1B
		{{0x7535,0x565},{0x6B41,0xA60},{0x6201,0xEFF},{0x595C,0x1352},{0x513B,0x1762},{0x498B,0x1B3A},{0x423C,0x1EE2},{0x3B41,0x2260}},//0x1C
		{{0x74D7,0x595},{0x6A92,0xAB7},{0x610D,0xF79},{0x582B,0x13EA},{0x4FD4,0x1816},{0x47F3,0x1C07},{0x4076,0x1FC5},{0x3950,0x2358}},//0x1D
		{{0x7479,0x5C4},{0x69E5,0xB0E},{0x601B,0xFF3},{0x56FD,0x1481},{0x4E70,0x18C8},{0x465E,0x1CD1},{0x3EB5,0x20A6},{0x3764,0x244E}},//0x1E
		{{0x741B,0x5F3},{0x6937,0xB64},{0x5F2A,0x106B},{0x55D1,0x1517},{0x4D10,0x1978},{0x44CE,0x1D99},{0x3CF8,0x2184},{0x357E,0x2541}},//0x1F
		{{0x73BD,0x622},{0x688B,0xBBA},{0x5E3B,0x1000},{0x54A8,0x15AC},{0x4BB3,0x1A27},{0x4342,0x1E5F},{0x3B41,0x2260},{0x339D,0x2632}},//0x20
		{{0x7360,0x650},{0x67E0,0xC10},{0x5D4E,0x1159},{0x5381,0x163F},{0x4A59,0x1AD3},{0x41BA,0x1F23},{0x398D,0x2339},{0x31C0,0x2720}},//0x21
		{{0x7302,0x67F},{0x6736,0xC65},{0x5C62,0x11CF},{0x525D,0x16D1},{0x4903,0x1B7F},{0x4036,0x1FE5},{0x37DE,0x2411},{0x2FE8,0x280C}},//0x22
		{{0x72A5,0x6AD},{0x668C,0xCBA},{0x5B78,0x1244},{0x513B,0x1762},{0x47AF,0x1C29},{0x3EB5,0x20A6},{0x3633,0x24E6},{0x2E15,0x28F5}},//0x23
		{{0x7249,0x6DC},{0x65E4,0xD0E},{0x5A90,0x12B8},{0x501C,0x17F2},{0x465E,0x1CD1},{0x3D38,0x2164},{0x348D,0x25BA},{0x2C46,0x29DD}},//0x24
		{{0x71EC,0x70A},{0x653C,0xD62},{0x59A9,0x132C},{0x4EFE,0x1881},{0x4511,0x1D78},{0x3BBE,0x2221},{0x32E9,0x268B},{0x2A7B,0x2AC2}},//0x25
		{{0x7190,0x738},{0x6495,0xDB6},{0x58C4,0x139E},{0x4DE3,0x190F},{0x43C6,0x1E1D},{0x3A47,0x22DC},{0x314A,0x275B},{0x28B4,0x2BA6}},//0x26
		{{0x7134,0x766},{0x63EF,0xE09},{0x57E0,0x1410},{0x4CCA,0x199B},{0x427E,0x1EC1},{0x38D4,0x2396},{0x2FAE,0x2829},{0x26F0,0x2C88}},//0x27
		{{0x70D9,0x794},{0x6349,0xE5B},{0x56FD,0x1481},{0x4BB3,0x1A27},{0x4138,0x1F64},{0x3764,0x244E},{0x2E15,0x28F5},{0x2530,0x2D68}},//0x28
		{{0x707E,0x7C1},{0x62A5,0xEAE},{0x561C,0x14F2},{0x4A9E,0x1AB1},{0x3FF5,0x2005},{0x35F7,0x2505},{0x2C80,0x29C0},{0x2373,0x2E46}},//0x29
		{{0x7022,0x7EF},{0x6201,0xEFF},{0x553D,0x1562},{0x498B,0x1B3A},{0x3EB5,0x20A6},{0x348D,0x25BA},{0x2AED,0x2A89},{0x21B9,0x2F23}},//0x2A
		{{0x6FC8,0x81C},{0x615E,0xF51},{0x545E,0x15D1},{0x487A,0x1BC3},{0x3D77,0x2145},{0x3325,0x266E},{0x295E,0x2B51},{0x2002,0x2FFF}},//0x2B
		{{0x6F6D,0x84A},{0x60BC,0xFA2},{0x5381,0x163F},{0x476B,0x1C4A},{0x3C3B,0x21E2},{0x31C0,0x2720},{0x27D2,0x2C17},{0x1E4E,0x30D9}},//0x2C
		{{0x6F13,0x877},{0x601B,0xFF3},{0x52A6,0x16AD},{0x465E,0x1CD1},{0x3B02,0x227F},{0x305E,0x27D1},{0x2648,0x2CDC},{0x1C9D,0x31B2}},//0x2D
		{{0x6EB9,0x8A4},{0x5F7A,0x1043},{0x51CC,0x171A},{0x4553,0x1D56},{0x39CB,0x231A},{0x2EFE,0x2881},{0x24C0,0x2DA0},{0x1AED,0x3289}},//0x2E
		{{0x6E5F,0x8D1},{0x5EDA,0x1093},{0x50F3,0x1786},{0x444A,0x1DDB},{0x3897,0x23B5},{0x2DA1,0x292F},{0x233C,0x2E62},{0x1940,0x3360}},//0x2F
		{{0x6E05,0x8FD},{0x5E3B,0x10E2},{0x501C,0x17F2},{0x4342,0x1E5F},{0x3764,0x244E},{0x2C46,0x29DD},{0x21B9,0x2F23},{0x1796,0x3435}},//0x30
		{{0x6DAC,0x92A},{0x5D9D,0x1132},{0x4F45,0x185D},{0x423C,0x1EE2},{0x3633,0x24E6},{0x2AED,0x2A89},{0x2039,0x2FE4},{0x15ED,0x350A}},//0x31
		{{0x6D53,0x957},{0x5CFF,0x1180},{0x4E70,0x18C8},{0x4138,0x1F64},{0x3505,0x257E},{0x2997,0x2B35},{0x1EBB,0x30A3},{0x1446,0x35DD}} //0x32
	};

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

uint8 Read_EXT_RAM(uint16 addr, uint8* data)
{
	PGA450_W_Package Write;
	PGA450_R_Package Read;
	uint8 status = 1;

	Write.cmd = CMD_External_RAM_read;
	Write.address = addr >> 8;
	Write.data0 = addr & 0xFF;
	PGA450_Transfer(SPI_PGA450_Handle, &Write, &Read);
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

uint8 Read_ESFR(uint8 addr, uint8* data)
{
	PGA450_W_Package Write;
	PGA450_R_Package Read;
	uint8 status = 1;

	Write.cmd = CMD_ESFR_read;
	Write.address = addr;
	PGA450_Transfer(SPI_PGA450_Handle, &Write, &Read);
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

void Set_LNA_Gain(uint8 gain)
{
	Write_ESFR(ADDR_CONTROL_1,gain << 2);
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

void Set_Lowpass_Coefficient(uint16 cutoff, uint16 downsample)
{
	uint16 A2,B1;
	A2 = Lowpass_Coefficient_Table[downsample-25][cutoff].A2;
	B1 = Lowpass_Coefficient_Table[downsample-25][cutoff].B1;
	//write A2
	Write_ESFR(ADDR_BPF_A2_MSB,A2 >> 8);
	Write_ESFR(ADDR_BPF_A2_LSB,A2 & 0xFF);
	//write B1
	Write_ESFR(ADDR_BPF_B1_MSB,B1 >> 8);
	Write_ESFR(ADDR_BPF_B1_LSB,B1 & 0xFF);
}

void Set_Data_Store(PGA450_Parameter* para)
{
	Write_ESFR(ADDR_DOWNSAMPLE,para->downsample);
	Write_ESFR(ADDR_FIFO_CTRL,para->FIFO_mode | 0x4);
	Write_ESFR(ADDR_BLANKING_TIME,para->blink_time);
}

void Set_Transducer_Driver(Transducer_Driver_Parameter* driver)
{
	uint32 temp;
	//write burst_mode
	Write_ESFR(ADDR_BURST_MODE,driver->burst_mode);

	//write output voltage
	Write_ESFR(ADDR_VREG_SEL,driver->drive_voltage);

	//write pulse number
	Write_ESFR(ADDR_PULSE_CNTA,driver->outA_pulse_num);
	Write_ESFR(ADDR_PULSE_CNTB,driver->outB_pulse_num);

	//wrtie frequency
	temp = 16000000/(driver->frequency*2);
	if(temp > 0x7FF) temp = 0x7FF;
	driver->outA_on_time = temp;   //ON_A = dec2hex(FOSC / fburst / 2)
	Write_ESFR(ADDR_ONA_MSB,driver->outA_on_time >> 8);
	Write_ESFR(ADDR_ONA_LSB,driver->outA_on_time & 0xFF);

	driver->outA_off_time = temp;
	Write_ESFR(ADDR_OFFA_MSB,driver->outA_off_time >> 8);
	Write_ESFR(ADDR_OFFA_LSB,driver->outA_off_time& 0xFF);

	driver->outB_on_time = temp;   //ON_A = dec2hex(FOSC / fburst / 2)
	Write_ESFR(ADDR_ONB_MSB,driver->outB_on_time >> 8);
	Write_ESFR(ADDR_ONB_LSB,driver->outB_on_time & 0xFF);

	driver->outB_off_time = temp;
	Write_ESFR(ADDR_OFFB_MSB,driver->outB_off_time >> 8);
	Write_ESFR(ADDR_OFFB_LSB,driver->outB_off_time& 0xFF);

	driver->dead_time = (float)temp*0.05;  //5% deadtime
	if(driver->dead_time < 3) driver->dead_time = 3;
	Write_ESFR(ADDR_DEADTIME,driver->dead_time);
}

void Turn_no_Sample(void)
{
	Write_ESFR(ADDR_EN_CTRL,0x09);
}

void Turn_off_Sample(void)
{
    Write_ESFR(ADDR_EN_CTRL,0x00);
}
void Read_PGA450_FIFO(uint8* pbuf)
{
	uint16 i;
	for(i=0;i<768;i++)
	{
		Read_EXT_RAM(i,pbuf);
		pbuf++;
	}
}
void Power_Enable(void)
{
	Write_ESFR(ADDR_PWR_MODE,0x03);
}

void Initial_PGA450(void)
{
	Power_Enable();
	Set_Transducer_Driver(&PAG450_Para.driver);
	Set_LNA_Gain(PAG450_Para.LNA_gain);
	Set_Bandpass_Coefficient(PAG450_Para.BPF_CF,PAG450_Para.BPF_BW);
	Set_Lowpass_Coefficient(PAG450_Para.LPF_CUT,PAG450_Para.downsample);
	Set_Data_Store(&PAG450_Para);
	Write_ESFR(ADDR_ANALOG_MUX,0x04);
	Write_ESFR(ADDR_DIGITAL_MUX,0x08);
}
/*********************************************************************
*********************************************************************/

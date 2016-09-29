/******************************************************************************
 *
 * Copyright(c) 2007 - 2011 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/

#include "../mp_precomp.h"
#include "../phydm_precomp.h"



/*---------------------------Define Local Constant---------------------------*/
// 2010/04/25 MH Define the max tx power tracking tx agc power.
#define		ODM_TXPWRTRACK_MAX_IDX8812A		6

/*---------------------------Define Local Constant---------------------------*/


//3============================================================
//3 Tx Power Tracking
//3============================================================

#if 0



	//new element A = element D x X

		//new element C = element D x Y


void DoIQK_8812A(
	PVOID		pDM_VOID,
	u1Byte 		DeltaThermalIndex,
	u1Byte		ThermalValue,	
	u1Byte 		Threshold
	)
{
	PDM_ODM_T	pDM_Odm = (PDM_ODM_T)pDM_VOID;
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	PADAPTER 		Adapter = pDM_Odm->Adapter;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
#endif

	ODM_ResetIQKResult(pDM_Odm);		

#if(DM_ODM_SUPPORT_TYPE  & ODM_WIN)
#if (DEV_BUS_TYPE == RT_PCI_INTERFACE)	
#if USE_WORKITEM
	PlatformAcquireMutex(&pHalData->mxChnlBwControl);
#else
	PlatformAcquireSpinLock(Adapter, RT_CHANNEL_AND_BANDWIDTH_SPINLOCK);
#endif
#elif((DEV_BUS_TYPE == RT_USB_INTERFACE) || (DEV_BUS_TYPE == RT_SDIO_INTERFACE))
	PlatformAcquireMutex(&pHalData->mxChnlBwControl);
#endif
#endif			


	pDM_Odm->RFCalibrateInfo.ThermalValue_IQK= ThermalValue;
	PHY_IQCalibrate_8812A(Adapter, FALSE);


#if(DM_ODM_SUPPORT_TYPE  & ODM_WIN)
#if (DEV_BUS_TYPE == RT_PCI_INTERFACE)	
#if USE_WORKITEM
	PlatformReleaseMutex(&pHalData->mxChnlBwControl);
#else
	PlatformReleaseSpinLock(Adapter, RT_CHANNEL_AND_BANDWIDTH_SPINLOCK);
#endif
#elif((DEV_BUS_TYPE == RT_USB_INTERFACE) || (DEV_BUS_TYPE == RT_SDIO_INTERFACE))
	PlatformReleaseMutex(&pHalData->mxChnlBwControl);
#endif
#endif
}

/*-----------------------------------------------------------------------------
 * Function:	odm_TxPwrTrackSetPwr88E()
 *
 * Overview:	88E change all channel tx power accordign to flag.
 *				OFDM & CCK are all different.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	04/23/2012	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
VOID
ODM_TxPwrTrackSetPwr8812A(
		PDM_ODM_T			pDM_Odm,
		PWRTRACK_METHOD 	Method,
		u1Byte 				RFPath,
		u1Byte 				ChannelMappedIndex
		)
{
	if (Method == TXAGC) 
	{
		u1Byte	cckPowerLevel[MAX_TX_COUNT], ofdmPowerLevel[MAX_TX_COUNT];
		u1Byte	BW20PowerLevel[MAX_TX_COUNT], BW40PowerLevel[MAX_TX_COUNT];
		u1Byte	rf = 0;
		u4Byte 	pwr = 0, TxAGC = 0;
		PADAPTER Adapter = pDM_Odm->Adapter;

		ODM_RT_TRACE(pDM_Odm, ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD, ("odm_TxPwrTrackSetPwr88E CH=%d\n", *(pDM_Odm->pChannel)));
#if (DM_ODM_SUPPORT_TYPE & (ODM_WIN|ODM_CE ))

		pwr = PHY_QueryBBReg(Adapter, rTxAGC_A_Rate18_06, 0xFF);
		pwr += (pDM_Odm->BbSwingIdxCck - pDM_Odm->BbSwingIdxCckBase);
		TxAGC = (pwr<<16)|(pwr<<8)|(pwr);
		PHY_SetBBReg(Adapter, rTxAGC_A_CCK1_Mcs32, bMaskByte1, TxAGC);
		PHY_SetBBReg(Adapter, rTxAGC_B_CCK11_A_CCK2_11, 0xffffff00, TxAGC);
		RTPRINT(FPHY, PHY_TXPWR, ("ODM_TxPwrTrackSetPwr88E: CCK Tx-rf(A) Power = 0x%x\n", TxAGC));		

		pwr = PHY_QueryBBReg(Adapter, rTxAGC_A_Rate18_06, 0xFF);
		pwr += (pDM_Odm->BbSwingIdxOfdm[RF_PATH_A] - pDM_Odm->BbSwingIdxOfdmBase);
		TxAGC |= ((pwr<<24)|(pwr<<16)|(pwr<<8)|pwr);
		PHY_SetBBReg(Adapter, rTxAGC_A_Rate18_06, bMaskDWord, TxAGC);
		PHY_SetBBReg(Adapter, rTxAGC_A_Rate54_24, bMaskDWord, TxAGC);
		PHY_SetBBReg(Adapter, rTxAGC_A_Mcs03_Mcs00, bMaskDWord, TxAGC);
		PHY_SetBBReg(Adapter, rTxAGC_A_Mcs07_Mcs04, bMaskDWord, TxAGC);
		PHY_SetBBReg(Adapter, rTxAGC_A_Mcs11_Mcs08, bMaskDWord, TxAGC);
		PHY_SetBBReg(Adapter, rTxAGC_A_Mcs15_Mcs12, bMaskDWord, TxAGC);
		RTPRINT(FPHY, PHY_TXPWR, ("ODM_TxPwrTrackSetPwr88E: OFDM Tx-rf(A) Power = 0x%x\n", TxAGC));		
#endif
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
		PHY_RF6052SetCCKTxPower(pDM_Odm->priv, *(pDM_Odm->pChannel));
		PHY_RF6052SetOFDMTxPower(pDM_Odm->priv, *(pDM_Odm->pChannel));
#endif

	} 
	else if (Method == BBSWING)
	{
		// Adjust BB swing by OFDM IQ matrix
		if (RFPath == RF_PATH_A)
			ODM_SetBBReg(pDM_Odm, rA_TxScale_Jaguar, bMaskDWord, pDM_Odm->BbSwingIdxOfdm[RF_PATH_A]);
		else if (RFPath == RF_PATH_B)
			ODM_SetBBReg(pDM_Odm, rB_TxScale_Jaguar, bMaskDWord, pDM_Odm->BbSwingIdxOfdm[RF_PATH_B]);
	}
	else
	{
		return;
	}
}	// odm_TxPwrTrackSetPwr88E

void ConfigureTxpowerTrack_8812A(
		PTXPWRTRACK_CFG	pConfig
		)
{
	pConfig->SwingTableSize_CCK = CCK_TABLE_SIZE;
	pConfig->SwingTableSize_OFDM = OFDM_TABLE_SIZE;
	pConfig->Threshold_IQK = 8;
	pConfig->AverageThermalNum = AVG_THERMAL_NUM_8812A;
	pConfig->RfPathCount = 2;
	pConfig->ThermalRegAddr = RF_T_METER_8812A;

	pConfig->ODM_TxPwrTrackSetPwr = ODM_TxPwrTrackSetPwr8812A;
	pConfig->DoIQK = DoIQK_8812A;
	pConfig->PHY_LCCalibrate = PHY_LCCalibrate_8812A;
}

#endif

//1 7.	IQK
#define MAX_TOLERANCE		5
#define IQK_DELAY_TIME		1		//ms

u1Byte			//bit0 = 1 => Tx OK, bit1 = 1 => Rx OK
	phy_PathA_IQK_8812A(
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
			IN PDM_ODM_T		pDM_Odm,
#else
			IN	PADAPTER	pAdapter,
#endif
			IN	BOOLEAN		configPathB
			)
{
	u4Byte regEAC, regE94, regE9C, regEA4;
	u1Byte result = 0x00;
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);	
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
#endif
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;
#endif
#endif	
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Path A IQK!\n"));

	//1 Tx IQK
	//path-A IQK setting
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Path-A IQK setting!\n"));
	ODM_SetBBReg(pDM_Odm, rTx_IQK_Tone_A, bMaskDWord, 0x10008c1c);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_Tone_A, bMaskDWord, 0x30008c1c);
	ODM_SetBBReg(pDM_Odm, rTx_IQK_PI_A, bMaskDWord, 0x8214032a);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_PI_A, bMaskDWord, 0x28160000);

	//LO calibration setting
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("LO calibration setting!\n"));
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Rsp, bMaskDWord, 0x00462911);

	//One shot, path A LOK & IQK
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("One shot, path A LOK & IQK!\n"));
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Pts, bMaskDWord, 0xf9000000);
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Pts, bMaskDWord, 0xf8000000);

	// delay x ms
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Delay %d ms for One shot, path A LOK & IQK.\n", IQK_DELAY_TIME_8812A));
	//PlatformStallExecution(IQK_DELAY_TIME_8812A*1000);
	ODM_delay_ms(IQK_DELAY_TIME_8812A);

	// Check failed
	regEAC = ODM_GetBBReg(pDM_Odm, rRx_Power_After_IQK_A_2, bMaskDWord);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("0xeac = 0x%x\n", regEAC));
	regE94 = ODM_GetBBReg(pDM_Odm, rTx_Power_Before_IQK_A, bMaskDWord);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("0xe94 = 0x%x\n", regE94));
	regE9C= ODM_GetBBReg(pDM_Odm, rTx_Power_After_IQK_A, bMaskDWord);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("0xe9c = 0x%x\n", regE9C));
	regEA4= ODM_GetBBReg(pDM_Odm, rRx_Power_Before_IQK_A_2, bMaskDWord);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("0xea4 = 0x%x\n", regEA4));

	if(!(regEAC & BIT28) &&		
			(((regE94 & 0x03FF0000)>>16) != 0x142) &&
			(((regE9C & 0x03FF0000)>>16) != 0x42) )
		result |= 0x01;
	else							//if Tx not OK, ignore Rx
		return result;

#if 0
	if(!(regEAC & BIT27) &&		//if Tx is OK, check whether Rx is OK
			(((regEA4 & 0x03FF0000)>>16) != 0x132) &&
			(((regEAC & 0x03FF0000)>>16) != 0x36))
		result |= 0x02;
	else
		RTPRINT(FINIT, INIT_IQK, ("Path A Rx IQK fail!!\n"));
#endif	

	return result;


}

u1Byte			//bit0 = 1 => Tx OK, bit1 = 1 => Rx OK
	phy_PathA_RxIQK_8812A(
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
			IN PDM_ODM_T		pDM_Odm,
#else
			IN	PADAPTER	pAdapter,
#endif
			IN	BOOLEAN		configPathB
			)
{
	u4Byte regEAC, regE94, regE9C, regEA4, u4tmp;
	u1Byte result = 0x00;
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
#endif
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;
#endif
#endif	
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Path A Rx IQK!\n"));

	//1 Get TXIMR setting
	//modify RXIQK mode table
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Path-A Rx IQK modify RXIQK mode table!\n"));
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, 0xffffff00, 0);	
	ODM_SetRFReg(pDM_Odm, RF_PATH_A, RF_WE_LUT, bRFRegOffsetMask, 0x800a0 );
	ODM_SetRFReg(pDM_Odm, RF_PATH_A, RF_RCK_OS, bRFRegOffsetMask, 0x30000 );
	ODM_SetRFReg(pDM_Odm, RF_PATH_A, RF_TXPA_G1, bRFRegOffsetMask, 0x0000f );
	ODM_SetRFReg(pDM_Odm, RF_PATH_A, RF_TXPA_G2, bRFRegOffsetMask, 0xf117B );
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, 0xffffff00, 0x808000);

	//IQK setting
	ODM_SetBBReg(pDM_Odm, rTx_IQK, bMaskDWord, 0x01007c00);
	ODM_SetBBReg(pDM_Odm, rRx_IQK, bMaskDWord, 0x81004800);

	//path-A IQK setting
	ODM_SetBBReg(pDM_Odm, rTx_IQK_Tone_A, bMaskDWord, 0x10008c1c);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_Tone_A, bMaskDWord, 0x30008c1c);
	ODM_SetBBReg(pDM_Odm, rTx_IQK_PI_A, bMaskDWord, 0x82160804);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_PI_A, bMaskDWord, 0x28160000);	

	//LO calibration setting
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("LO calibration setting!\n"));
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Rsp, bMaskDWord, 0x0046a911);

	//One shot, path A LOK & IQK
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("One shot, path A LOK & IQK!\n"));
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Pts, bMaskDWord, 0xf9000000);
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Pts, bMaskDWord, 0xf8000000);

	// delay x ms
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Delay %d ms for One shot, path A LOK & IQK.\n", IQK_DELAY_TIME_8812A));
	//PlatformStallExecution(IQK_DELAY_TIME_8812A*1000);
	ODM_delay_ms(IQK_DELAY_TIME_8812A);


	// Check failed
	regEAC = ODM_GetBBReg(pDM_Odm, rRx_Power_After_IQK_A_2, bMaskDWord);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("0xeac = 0x%x\n", regEAC));	
	regE94 = ODM_GetBBReg(pDM_Odm, rTx_Power_Before_IQK_A, bMaskDWord);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("0xe94 = 0x%x\n", regE94));
	regE9C= ODM_GetBBReg(pDM_Odm, rTx_Power_After_IQK_A, bMaskDWord);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("0xe9c = 0x%x\n", regE9C));

	if(!(regEAC & BIT28) &&		
			(((regE94 & 0x03FF0000)>>16) != 0x142) &&
			(((regE9C & 0x03FF0000)>>16) != 0x42) )
		result |= 0x01;
	else							//if Tx not OK, ignore Rx
		return result;

	u4tmp = 0x80007C00 | (regE94&0x3FF0000)  | ((regE9C&0x3FF0000) >> 16);	
	ODM_SetBBReg(pDM_Odm, rTx_IQK, bMaskDWord, u4tmp);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("0xe40 = 0x%x u4tmp = 0x%x \n", ODM_GetBBReg(pDM_Odm, rTx_IQK, bMaskDWord), u4tmp));	


	//1 RX IQK
	//modify RXIQK mode table
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Path-A Rx IQK modify RXIQK mode table 2!\n"));
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, 0xffffff00, 0);		
	ODM_SetRFReg(pDM_Odm, RF_PATH_A, RF_WE_LUT, bRFRegOffsetMask, 0x800a0 );
	ODM_SetRFReg(pDM_Odm, RF_PATH_A, RF_RCK_OS, bRFRegOffsetMask, 0x30000 );
	ODM_SetRFReg(pDM_Odm, RF_PATH_A, RF_TXPA_G1, bRFRegOffsetMask, 0x0000f );
	ODM_SetRFReg(pDM_Odm, RF_PATH_A, RF_TXPA_G2, bRFRegOffsetMask, 0xf7ffa );
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, 0xffffff00, 0x808000);

	//IQK setting
	ODM_SetBBReg(pDM_Odm, rRx_IQK, bMaskDWord, 0x01004800);

	//path-A IQK setting
	ODM_SetBBReg(pDM_Odm, rTx_IQK_Tone_A, bMaskDWord, 0x30008c1c);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_Tone_A, bMaskDWord, 0x10008c1c);
	ODM_SetBBReg(pDM_Odm, rTx_IQK_PI_A, bMaskDWord, 0x82160c05);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_PI_A, bMaskDWord, 0x28160c05);	

	//LO calibration setting
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("LO calibration setting!\n"));
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Rsp, bMaskDWord, 0x0046a911);

	//One shot, path A LOK & IQK
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("One shot, path A LOK & IQK!\n"));
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Pts, bMaskDWord, 0xf9000000);
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Pts, bMaskDWord, 0xf8000000);

	// delay x ms
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Delay %d ms for One shot, path A LOK & IQK.\n", IQK_DELAY_TIME_8812A));
	//PlatformStallExecution(IQK_DELAY_TIME_8812A*1000);
	ODM_delay_ms(IQK_DELAY_TIME_8812A);

	// Check failed
	regEAC = ODM_GetBBReg(pDM_Odm, rRx_Power_After_IQK_A_2, bMaskDWord);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("0xeac = 0x%x\n", regEAC));
	regE94 = ODM_GetBBReg(pDM_Odm, rTx_Power_Before_IQK_A, bMaskDWord);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("0xe94 = 0x%x\n", regE94));
	regE9C= ODM_GetBBReg(pDM_Odm, rTx_Power_After_IQK_A, bMaskDWord);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("0xe9c = 0x%x\n", regE9C));
	regEA4= ODM_GetBBReg(pDM_Odm, rRx_Power_Before_IQK_A_2, bMaskDWord);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("0xea4 = 0x%x\n", regEA4));

#if 0	
	if(!(regEAC & BIT28) &&		
			(((regE94 & 0x03FF0000)>>16) != 0x142) &&
			(((regE9C & 0x03FF0000)>>16) != 0x42) )
		result |= 0x01;
	else							//if Tx not OK, ignore Rx
		return result;
#endif	

	if(!(regEAC & BIT27) &&		//if Tx is OK, check whether Rx is OK
			(((regEA4 & 0x03FF0000)>>16) != 0x132) &&
			(((regEAC & 0x03FF0000)>>16) != 0x36))
		result |= 0x02;
	else
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("Path A Rx IQK fail!!\n"));

	return result;


}

u1Byte				//bit0 = 1 => Tx OK, bit1 = 1 => Rx OK
	phy_PathB_IQK_8812A(
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
			IN PDM_ODM_T		pDM_Odm
#else
			IN	PADAPTER	pAdapter
#endif
			)
{
	u4Byte regEAC, regEB4, regEBC, regEC4, regECC;
	u1Byte	result = 0x00;
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
#endif
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;
#endif
#endif	
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("Path B IQK!\n"));

	//One shot, path B LOK & IQK
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("One shot, path A LOK & IQK!\n"));
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Cont, bMaskDWord, 0x00000002);
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Cont, bMaskDWord, 0x00000000);

	// delay x ms
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Delay %d ms for One shot, path B LOK & IQK.\n", IQK_DELAY_TIME_8812A));
	//PlatformStallExecution(IQK_DELAY_TIME_8812A*1000);
	ODM_delay_ms(IQK_DELAY_TIME_8812A);

	// Check failed
	regEAC = ODM_GetBBReg(pDM_Odm, rRx_Power_After_IQK_A_2, bMaskDWord);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("0xeac = 0x%x\n", regEAC));
	regEB4 = ODM_GetBBReg(pDM_Odm, rTx_Power_Before_IQK_B, bMaskDWord);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("0xeb4 = 0x%x\n", regEB4));
	regEBC= ODM_GetBBReg(pDM_Odm, rTx_Power_After_IQK_B, bMaskDWord);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("0xebc = 0x%x\n", regEBC));
	regEC4= ODM_GetBBReg(pDM_Odm, rRx_Power_Before_IQK_B_2, bMaskDWord);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("0xec4 = 0x%x\n", regEC4));
	regECC= ODM_GetBBReg(pDM_Odm, rRx_Power_After_IQK_B_2, bMaskDWord);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("0xecc = 0x%x\n", regECC));

	if(!(regEAC & BIT31) &&
			(((regEB4 & 0x03FF0000)>>16) != 0x142) &&
			(((regEBC & 0x03FF0000)>>16) != 0x42))
		result |= 0x01;
	else
		return result;

	if(!(regEAC & BIT30) &&
			(((regEC4 & 0x03FF0000)>>16) != 0x132) &&
			(((regECC & 0x03FF0000)>>16) != 0x36))
		result |= 0x02;
	else
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("Path B Rx IQK fail!!\n"));


	return result;

}

VOID
	_PHY_PathAFillIQKMatrix_8812A(
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
			IN PDM_ODM_T		pDM_Odm,
#else
			IN	PADAPTER	pAdapter,
#endif
			IN  BOOLEAN    	bIQKOK,
			IN	s4Byte		result[][8],
			IN	u1Byte		final_candidate,
			IN  BOOLEAN		bTxOnly
			)
{
	u4Byte	Oldval_0, X, TX0_A, reg;
	s4Byte	Y, TX0_C;
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);		
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
#endif
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;
#endif
#endif	
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("Path A IQ Calibration %s !\n",(bIQKOK)?"Success":"Failed"));

	if(final_candidate == 0xFF)
		return;

	else if(bIQKOK)
	{
		Oldval_0 = (ODM_GetBBReg(pDM_Odm, rOFDM0_XATxIQImbalance, bMaskDWord) >> 22) & 0x3FF;

		X = result[final_candidate][0];
		if ((X & 0x00000200) != 0)
			X = X | 0xFFFFFC00;				
		TX0_A = (X * Oldval_0) >> 8;
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("X = 0x%x, TX0_A = 0x%x, Oldval_0 0x%x\n", X, TX0_A, Oldval_0));
		ODM_SetBBReg(pDM_Odm, rOFDM0_XATxIQImbalance, 0x3FF, TX0_A);

		ODM_SetBBReg(pDM_Odm, rOFDM0_ECCAThreshold, BIT(31), ((X* Oldval_0>>7) & 0x1));

		Y = result[final_candidate][1];
		if ((Y & 0x00000200) != 0)
			Y = Y | 0xFFFFFC00;		


		TX0_C = (Y * Oldval_0) >> 8;
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("Y = 0x%x, TX = 0x%x\n", (u4Byte)Y, (u4Byte)TX0_C));
		ODM_SetBBReg(pDM_Odm, rOFDM0_XCTxAFE, 0xF0000000, ((TX0_C&0x3C0)>>6));
		ODM_SetBBReg(pDM_Odm, rOFDM0_XATxIQImbalance, 0x003F0000, (TX0_C&0x3F));

		ODM_SetBBReg(pDM_Odm, rOFDM0_ECCAThreshold, BIT(29), ((Y* Oldval_0>>7) & 0x1));

		if(bTxOnly)
		{
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("_PHY_PathAFillIQKMatrix_8812A only Tx OK\n"));		
			return;
		}

		reg = result[final_candidate][2];
#if (DM_ODM_SUPPORT_TYPE==ODM_AP)		
		if( RTL_ABS(reg ,0x100) >= 16) 
			reg = 0x100;
#endif
		ODM_SetBBReg(pDM_Odm, rOFDM0_XARxIQImbalance, 0x3FF, reg);

		reg = result[final_candidate][3] & 0x3F;
		ODM_SetBBReg(pDM_Odm, rOFDM0_XARxIQImbalance, 0xFC00, reg);

		reg = (result[final_candidate][3] >> 6) & 0xF;
		ODM_SetBBReg(pDM_Odm, rOFDM0_RxIQExtAnta, 0xF0000000, reg);
	}
}

VOID
	_PHY_PathBFillIQKMatrix_8812A(
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
			IN PDM_ODM_T		pDM_Odm,
#else
			IN	PADAPTER	pAdapter,
#endif
			IN  BOOLEAN   	bIQKOK,
			IN	s4Byte		result[][8],
			IN	u1Byte		final_candidate,
			IN	BOOLEAN		bTxOnly			//do Tx only
			)
{
	u4Byte	Oldval_1, X, TX1_A, reg;
	s4Byte	Y, TX1_C;
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);		
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
#endif
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;
#endif
#endif	
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Path B IQ Calibration %s !\n",(bIQKOK)?"Success":"Failed"));

	if(final_candidate == 0xFF)
		return;

	else if(bIQKOK)
	{
		Oldval_1 = (ODM_GetBBReg(pDM_Odm, rOFDM0_XBTxIQImbalance, bMaskDWord) >> 22) & 0x3FF;

		X = result[final_candidate][4];
		if ((X & 0x00000200) != 0)
			X = X | 0xFFFFFC00;		
		TX1_A = (X * Oldval_1) >> 8;
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("X = 0x%x, TX1_A = 0x%x\n", X, TX1_A));
		ODM_SetBBReg(pDM_Odm, rOFDM0_XBTxIQImbalance, 0x3FF, TX1_A);

		ODM_SetBBReg(pDM_Odm, rOFDM0_ECCAThreshold, BIT(27), ((X* Oldval_1>>7) & 0x1));

		Y = result[final_candidate][5];
		if ((Y & 0x00000200) != 0)
			Y = Y | 0xFFFFFC00;		

		TX1_C = (Y * Oldval_1) >> 8;
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("Y = 0x%x, TX1_C = 0x%x\n", (u4Byte)Y, (u4Byte)TX1_C));
		ODM_SetBBReg(pDM_Odm, rOFDM0_XDTxAFE, 0xF0000000, ((TX1_C&0x3C0)>>6));
		ODM_SetBBReg(pDM_Odm, rOFDM0_XBTxIQImbalance, 0x003F0000, (TX1_C&0x3F));

		ODM_SetBBReg(pDM_Odm, rOFDM0_ECCAThreshold, BIT(25), ((Y* Oldval_1>>7) & 0x1));

		if(bTxOnly)
			return;

		reg = result[final_candidate][6];
		ODM_SetBBReg(pDM_Odm, rOFDM0_XBRxIQImbalance, 0x3FF, reg);

		reg = result[final_candidate][7] & 0x3F;
		ODM_SetBBReg(pDM_Odm, rOFDM0_XBRxIQImbalance, 0xFC00, reg);

		reg = (result[final_candidate][7] >> 6) & 0xF;
		ODM_SetBBReg(pDM_Odm, rOFDM0_AGCRSSITable, 0x0000F000, reg);
	}
}

//
// 2011/07/26 MH Add an API for testing IQK fail case.
//
// MP Already declare in odm.c 
#if 0	//!(DM_ODM_SUPPORT_TYPE & ODM_WIN) 
BOOLEAN
ODM_CheckPowerStatus(
		IN	PADAPTER		Adapter)
{
	/*
	   HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	   PDM_ODM_T			pDM_Odm = &pHalData->DM_OutSrc;
	   RT_RF_POWER_STATE 	rtState;
	   PMGNT_INFO			pMgntInfo	= &(Adapter->MgntInfo);

	// 2011/07/27 MH We are not testing ready~~!! We may fail to get correct value when init sequence.
	if (pMgntInfo->init_adpt_in_progress == TRUE)
	{
	ODM_RT_TRACE(pDM_Odm,COMP_INIT, DBG_LOUD, ("ODM_CheckPowerStatus Return TRUE, due to initadapter"));
	return	TRUE;
	}

	//
	//	2011/07/19 MH We can not execute tx pwoer tracking/ LLC calibrate or IQK.
	//
	Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_RF_STATE, (pu1Byte)(&rtState));	
	if(Adapter->bDriverStopped || Adapter->bDriverIsGoingToPnpSetPowerSleep || rtState == eRfOff)
	{
	ODM_RT_TRACE(pDM_Odm,COMP_INIT, DBG_LOUD, ("ODM_CheckPowerStatus Return FALSE, due to %d/%d/%d\n", 
	Adapter->bDriverStopped, Adapter->bDriverIsGoingToPnpSetPowerSleep, rtState));
	return	FALSE;
	}
	 */
	return	TRUE;
}
#endif

VOID
	_PHY_SaveADDARegisters_8812A(
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
			IN PDM_ODM_T		pDM_Odm,
#else
			IN	PADAPTER	pAdapter,
#endif
			IN	pu4Byte		ADDAReg,
			IN	pu4Byte		ADDABackup,
			IN	u4Byte		RegisterNum
			)
{
	u4Byte	i;
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
#endif
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;
#endif

	if (ODM_CheckPowerStatus(pAdapter) == FALSE)
		return;
#endif

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Save ADDA parameters.\n"));
	for( i = 0 ; i < RegisterNum ; i++){
		ADDABackup[i] = ODM_GetBBReg(pDM_Odm, ADDAReg[i], bMaskDWord);
	}
}


VOID
	_PHY_SaveMACRegisters_8812A(
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
			IN PDM_ODM_T		pDM_Odm,
#else
			IN	PADAPTER	pAdapter,
#endif
			IN	pu4Byte		MACReg,
			IN	pu4Byte		MACBackup
			)
{
	u4Byte	i;
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
#endif
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;
#endif
#endif	
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Save MAC parameters.\n"));
	for( i = 0 ; i < (IQK_MAC_REG_NUM - 1); i++){
		MACBackup[i] = ODM_Read1Byte(pDM_Odm, MACReg[i]);		
	}
	MACBackup[i] = ODM_Read4Byte(pDM_Odm, MACReg[i]);		

}


VOID
	_PHY_ReloadADDARegisters_8812A(
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
			IN PDM_ODM_T		pDM_Odm,
#else
			IN	PADAPTER	pAdapter,
#endif
			IN	pu4Byte		ADDAReg,
			IN	pu4Byte		ADDABackup,
			IN	u4Byte		RegiesterNum
			)
{
	u4Byte	i;
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
#endif
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;
#endif
#endif

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Reload ADDA power saving parameters !\n"));
	for(i = 0 ; i < RegiesterNum; i++)
	{
		ODM_SetBBReg(pDM_Odm, ADDAReg[i], bMaskDWord, ADDABackup[i]);
	}
}

VOID
	_PHY_ReloadMACRegisters_8812A(
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
			IN PDM_ODM_T		pDM_Odm,
#else
			IN	PADAPTER	pAdapter,
#endif
			IN	pu4Byte		MACReg,
			IN	pu4Byte		MACBackup
			)
{
	u4Byte	i;
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
#endif
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;
#endif
#endif
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("Reload MAC parameters !\n"));
	for(i = 0 ; i < (IQK_MAC_REG_NUM - 1); i++){
		ODM_Write1Byte(pDM_Odm, MACReg[i], (u1Byte)MACBackup[i]);
	}
	ODM_Write4Byte(pDM_Odm, MACReg[i], MACBackup[i]);	
}


VOID
	_PHY_PathADDAOn_8812A(
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
			IN PDM_ODM_T		pDM_Odm,
#else
			IN	PADAPTER	pAdapter,
#endif
			IN	pu4Byte		ADDAReg,
			IN	BOOLEAN		isPathAOn,
			IN	BOOLEAN		is2T
			)
{
	u4Byte	pathOn;
	u4Byte	i;
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
#endif
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;
#endif
#endif
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("ADDA ON.\n"));

	pathOn = isPathAOn ? 0x04db25a4 : 0x0b1b25a4;
	if(FALSE == is2T){
		pathOn = 0x0bdb25a0;
		ODM_SetBBReg(pDM_Odm, ADDAReg[0], bMaskDWord, 0x0b1b25a0);
	}
	else{
		ODM_SetBBReg(pDM_Odm,ADDAReg[0], bMaskDWord, pathOn);
	}

	for( i = 1 ; i < IQK_ADDA_REG_NUM ; i++){
		ODM_SetBBReg(pDM_Odm,ADDAReg[i], bMaskDWord, pathOn);
	}

}

VOID
	_PHY_MACSettingCalibration_8812A(
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
			IN PDM_ODM_T		pDM_Odm,
#else
			IN	PADAPTER	pAdapter,
#endif
			IN	pu4Byte		MACReg,
			IN	pu4Byte		MACBackup	
			)
{
	u4Byte	i = 0;
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
#endif
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;
#endif
#endif	
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("MAC settings for Calibration.\n"));

	ODM_Write1Byte(pDM_Odm, MACReg[i], 0x3F);

	for(i = 1 ; i < (IQK_MAC_REG_NUM - 1); i++){
		ODM_Write1Byte(pDM_Odm, MACReg[i], (u1Byte)(MACBackup[i]&(~BIT3)));
	}
	ODM_Write1Byte(pDM_Odm, MACReg[i], (u1Byte)(MACBackup[i]&(~BIT5)));	

}

VOID
	_PHY_PathAStandBy_8812A(
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
			IN PDM_ODM_T		pDM_Odm
#else
			IN PADAPTER	pAdapter
#endif
			)
{
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
#endif
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;
#endif
#endif	
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("Path-A standby mode!\n"));

	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, 0xffffff00, 0x0);
	ODM_SetBBReg(pDM_Odm, 0x840, bMaskDWord, 0x00010000);
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, 0xffffff00, 0x808000);
}

VOID
	_PHY_PIModeSwitch_8812A(
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
			IN PDM_ODM_T		pDM_Odm,
#else
			IN	PADAPTER	pAdapter,
#endif
			IN	BOOLEAN		PIMode
			)
{
	u4Byte	mode;
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
#endif
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;
#endif
#endif	
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("BB Switch to %s mode!\n", (PIMode ? "PI" : "SI")));

	mode = PIMode ? 0x01000100 : 0x01000000;
	ODM_SetBBReg(pDM_Odm, rFPGA0_XA_HSSIParameter1, bMaskDWord, mode);
	ODM_SetBBReg(pDM_Odm, rFPGA0_XB_HSSIParameter1, bMaskDWord, mode);
}

BOOLEAN							
	phy_SimularityCompare_8812A(
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
			IN PDM_ODM_T		pDM_Odm,
#else
			IN	PADAPTER	pAdapter,
#endif
			IN	s4Byte 		result[][8],
			IN	u1Byte		 c1,
			IN	u1Byte		 c2
			)
{
	u4Byte		i, j, diff, SimularityBitMap, bound = 0;
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);	
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
#endif
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;
#endif
#endif	
	u1Byte		final_candidate[2] = {0xFF, 0xFF};	//for path A and path B
	BOOLEAN		bResult = TRUE;
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	BOOLEAN		is2T = IS_92C_SERIAL( pHalData->VersionID);
#else
	BOOLEAN		is2T = 0;
#endif

	if(is2T)
		bound = 8;
	else
		bound = 4;

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("===> IQK:phy_SimularityCompare_8812A c1 %d c2 %d!!!\n", c1, c2));


	SimularityBitMap = 0;

	for( i = 0; i < bound; i++ )
	{
		diff = (result[c1][i] > result[c2][i]) ? (result[c1][i] - result[c2][i]) : (result[c2][i] - result[c1][i]);
		if (diff > MAX_TOLERANCE)
		{
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("IQK:phy_SimularityCompare_8812A differnece overflow index %d compare1 0x%x compare2 0x%x!!!\n",  i, (u4Byte)result[c1][i], (u4Byte)result[c2][i]));

			if((i == 2 || i == 6) && !SimularityBitMap)
			{
				if(result[c1][i]+result[c1][i+1] == 0)
					final_candidate[(i/4)] = c2;
				else if (result[c2][i]+result[c2][i+1] == 0)
					final_candidate[(i/4)] = c1;
				else
					SimularityBitMap = SimularityBitMap|(1<<i);					
			}
			else
				SimularityBitMap = SimularityBitMap|(1<<i);
		}
	}

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("IQK:phy_SimularityCompare_8812A SimularityBitMap   %d !!!\n", SimularityBitMap));

	if ( SimularityBitMap == 0)
	{
		for( i = 0; i < (bound/4); i++ )
		{
			if(final_candidate[i] != 0xFF)
			{
				for( j = i*4; j < (i+1)*4-2; j++)
					result[3][j] = result[final_candidate[i]][j];
				bResult = FALSE;
			}
		}
		return bResult;
	}
	else if (!(SimularityBitMap & 0x0F))			//path A OK
	{
		for(i = 0; i < 4; i++)
			result[3][i] = result[c1][i];
		return FALSE;
	}
	else if (!(SimularityBitMap & 0xF0) && is2T)	//path B OK
	{
		for(i = 4; i < 8; i++)
			result[3][i] = result[c1][i];
		return FALSE;
	}	
	else		
		return FALSE;

}
#if 0
#define BW_20M 	0
#define	BW_40M  1
#define	BW_80M	2
#endif

void _IQK_RX_FillIQC_8812A(
		IN PDM_ODM_T			pDM_Odm,
		IN ODM_RF_RADIO_PATH_E 	Path,
		IN unsigned int			RX_X,
		IN unsigned int			RX_Y
		) 
{
	switch (Path) {
		case ODM_RF_PATH_A:
			{
				ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
				ODM_SetBBReg(pDM_Odm, 0xc10, 0x000003ff, RX_X>>1);
				ODM_SetBBReg(pDM_Odm, 0xc10, 0x03ff0000, RX_Y>>1);
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RX_X = %x;;RX_Y = %x ====>fill to IQC\n", RX_X>>1&0x000003ff, RX_Y>>1&0x000003ff));
				ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("0xc10 = %x ====>fill to IQC\n", ODM_Read4Byte(pDM_Odm, 0xc10)));
			}
			break;
		case ODM_RF_PATH_B:
			{
				ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
				ODM_SetBBReg(pDM_Odm, 0xe10, 0x000003ff, RX_X>>1);
				ODM_SetBBReg(pDM_Odm, 0xe10, 0x03ff0000, RX_Y>>1);
			ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RX_X = %x;;RX_Y = %x====>fill to IQC\n ", RX_X>>1&0x000003ff, RX_Y>>1&0x000003ff));
				ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("0xe10 = %x====>fill to IQC\n", ODM_Read4Byte(pDM_Odm, 0xe10)));
			}
			break;		
		default:
			break;					
	};	
}

void _IQK_TX_FillIQC_8812A(
		IN PDM_ODM_T			pDM_Odm,
		IN ODM_RF_RADIO_PATH_E 	Path,
		IN unsigned int			TX_X,
		IN unsigned int			TX_Y
		) 
{
	switch (Path) {
		case ODM_RF_PATH_A:
			{
				ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 1 --> Page C1
				ODM_Write4Byte(pDM_Odm, 0xc90, 0x00000080);
				ODM_Write4Byte(pDM_Odm, 0xcc4, 0x20040000);
				ODM_Write4Byte(pDM_Odm, 0xcc8, 0x20000000);
				ODM_SetBBReg(pDM_Odm, 0xccc, 0x000007ff, TX_Y);
				ODM_SetBBReg(pDM_Odm, 0xcd4, 0x000007ff, TX_X);
			ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("TX_X = %x;;TX_Y = %x =====> fill to IQC\n", TX_X&0x000007ff, TX_Y&0x000007ff));
				ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("0xcd4 = %x;;0xccc = %x ====>fill to IQC\n", ODM_GetBBReg(pDM_Odm, 0xcd4, 0x000007ff), ODM_GetBBReg(pDM_Odm, 0xccc, 0x000007ff)));
			}
			break;
		case ODM_RF_PATH_B:
			{
				ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 1 --> Page C1
				ODM_Write4Byte(pDM_Odm, 0xe90, 0x00000080);
				ODM_Write4Byte(pDM_Odm, 0xec4, 0x20040000);
				ODM_Write4Byte(pDM_Odm, 0xec8, 0x20000000);
				ODM_SetBBReg(pDM_Odm, 0xecc, 0x000007ff, TX_Y);
				ODM_SetBBReg(pDM_Odm, 0xed4, 0x000007ff, TX_X);
			ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("TX_X = %x;;TX_Y = %x =====> fill to IQC\n", TX_X&0x000007ff, TX_Y&0x000007ff));
				ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("0xed4 = %x;;0xecc = %x ====>fill to IQC\n", ODM_GetBBReg(pDM_Odm, 0xed4, 0x000007ff), ODM_GetBBReg(pDM_Odm, 0xecc, 0x000007ff)));
			}
			break;		
		default:
			break;					
	};	
}

void _IQK_BackupMacBB_8812A(
		IN PDM_ODM_T		pDM_Odm,
	IN pu4Byte		MACBB_backup,
	IN pu4Byte		Backup_MACBB_REG, 
	IN u4Byte		MACBB_NUM
		)
{
	u4Byte i;
	ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
	//save MACBB default value
	for (i = 0; i < MACBB_NUM; i++){
		MACBB_backup[i] = ODM_Read4Byte(pDM_Odm, Backup_MACBB_REG[i]);
	}

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("BackupMacBB Success!!!!\n"));
}
void _IQK_BackupRF_8812A(
		IN PDM_ODM_T		pDM_Odm,
	IN pu4Byte		RFA_backup,
	IN pu4Byte		RFB_backup, 
	IN pu4Byte		Backup_RF_REG, 
	IN u4Byte		RF_NUM
		)	
{

	u4Byte i;
	ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
	//Save RF Parameters
	for (i = 0; i < RF_NUM; i++){
		RFA_backup[i] = ODM_GetRFReg(pDM_Odm, RF_PATH_A, Backup_RF_REG[i], bMaskDWord);
		RFB_backup[i] = ODM_GetRFReg(pDM_Odm, RF_PATH_B, Backup_RF_REG[i], bMaskDWord);
	}
	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("BackupRF Success!!!!\n"));
}
void _IQK_BackupAFE_8812A(
		IN PDM_ODM_T		pDM_Odm,
	IN pu4Byte		AFE_backup,
	IN pu4Byte		Backup_AFE_REG, 
	IN u4Byte		AFE_NUM
		)
{
	u4Byte i;
	ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
	//Save AFE Parameters 
    	for (i = 0; i < AFE_NUM; i++){
		AFE_backup[i] = ODM_Read4Byte(pDM_Odm, Backup_AFE_REG[i]);	
	}
	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("BackupAFE Success!!!!\n"));
}
void _IQK_RestoreMacBB_8812A(
		IN PDM_ODM_T		pDM_Odm,
	IN pu4Byte		MACBB_backup,
	IN pu4Byte		Backup_MACBB_REG, 
	IN u4Byte		MACBB_NUM
		)	
{
	u4Byte i;
	ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
	//Reload MacBB Parameters 
    	for (i = 0; i < MACBB_NUM; i++){
		ODM_Write4Byte(pDM_Odm, Backup_MACBB_REG[i], MACBB_backup[i]);
	}
	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RestoreMacBB Success!!!!\n"));
}
void _IQK_RestoreRF_8812A(
		IN PDM_ODM_T			pDM_Odm,
		IN ODM_RF_RADIO_PATH_E 	Path,
	IN pu4Byte			Backup_RF_REG,
	IN pu4Byte 			RF_backup,
	IN u4Byte			RF_REG_NUM
		)
{	
	u4Byte i;

	ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
	for (i = 0; i < RF_REG_NUM; i++)
		ODM_SetRFReg(pDM_Odm, Path, Backup_RF_REG[i], bRFRegOffsetMask, RF_backup[i]);
	ODM_SetRFReg(pDM_Odm, Path, 0xef, bRFRegOffsetMask, 0x0);

	switch(Path){
		case ODM_RF_PATH_A:
			{
				ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RestoreRF Path A Success!!!!\n"));
			}
			break;
		case ODM_RF_PATH_B:
			{
				ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RestoreRF Path B Success!!!!\n"));
			}
			break;
		default:
			break;
	}
}
void _IQK_RestoreAFE_8812A(
		IN PDM_ODM_T		pDM_Odm,
	IN pu4Byte		AFE_backup,
	IN pu4Byte		Backup_AFE_REG, 
	IN u4Byte		AFE_NUM
		)
{
	u4Byte i;
	ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
	//Reload AFE Parameters 
    	for (i = 0; i < AFE_NUM; i++){
		ODM_Write4Byte(pDM_Odm, Backup_AFE_REG[i], AFE_backup[i]);
	}
	ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 1 --> Page C1
	ODM_Write4Byte(pDM_Odm, 0xc80, 0x0);
	ODM_Write4Byte(pDM_Odm, 0xc84, 0x0);
	ODM_Write4Byte(pDM_Odm, 0xc88, 0x0);
	ODM_Write4Byte(pDM_Odm, 0xc8c, 0x3c000000);
	ODM_Write4Byte(pDM_Odm, 0xcb8, 0x0);
	ODM_Write4Byte(pDM_Odm, 0xe80, 0x0);
	ODM_Write4Byte(pDM_Odm, 0xe84, 0x0);
	ODM_Write4Byte(pDM_Odm, 0xe88, 0x0);
	ODM_Write4Byte(pDM_Odm, 0xe8c, 0x3c000000);
	ODM_Write4Byte(pDM_Odm, 0xeb8, 0x0);
	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RestoreAFE Success!!!!\n"));
}


void _IQK_ConfigureMAC_8812A(
		IN PDM_ODM_T		pDM_Odm
		)
{
	// ========MAC register setting========
	ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
	ODM_Write1Byte(pDM_Odm, 0x522, 0x3f);
	ODM_SetBBReg(pDM_Odm, 0x550, BIT(11)|BIT(3), 0x0);
	ODM_SetBBReg(pDM_Odm, 0x808, BIT(28), 0x0);	//		CCK Off
	ODM_Write1Byte(pDM_Odm, 0x808, 0x00);		//		RX ante off
	ODM_SetBBReg(pDM_Odm, 0x838, 0xf, 0xc);		//		CCA off
}

#define cal_num 3

void _IQK_Tx_8812A(
	IN PDM_ODM_T		pDM_Odm,
	IN ODM_RF_RADIO_PATH_E Path,
	IN u1Byte chnlIdx
	)
{
	u4Byte 		TX_fail,RX_fail, delay_count, IQK_ready, cal_retry, cal = 0, temp_reg65;
    int 			TX_X = 0, TX_Y = 0, RX_X = 0, RX_Y = 0, TX_Average = 0, RX_Average = 0;
    int 			TX_X0[cal_num], TX_Y0[cal_num], RX_X0[cal_num], RX_Y0[cal_num];
    	BOOLEAN 	TX0IQKOK = FALSE, RX0IQKOK = FALSE;
	int 			TX_X1[cal_num], TX_Y1[cal_num], RX_X1[cal_num], RX_Y1[cal_num];
	BOOLEAN  	TX1IQKOK = FALSE, RX1IQKOK = FALSE, VDF_enable = FALSE;
	int 			i, k, VDF_Y[3], VDF_X[3], Tx_dt[3], Rx_dt[3], ii, dx = 0, dy = 0, TX_finish = 0, RX_finish = 0;
	PODM_RF_CAL_T  pRFCalibrateInfo = &(pDM_Odm->RFCalibrateInfo);
	prtl8192cd_priv		priv = pDM_Odm->priv;

	pDM_Odm->priv->pshare->IQK_total_cnt++;

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("BandWidth = %d ExtPA = %d pBand = %d\n", *pDM_Odm->pBandWidth, pDM_Odm->ExtPA, *pDM_Odm->pBandType));
	if (*pDM_Odm->pBandWidth == 2){
		VDF_enable = TRUE;
	}
	temp_reg65 = ODM_GetRFReg(pDM_Odm, Path, 0x65, bMaskDWord);
		switch (Path) {
		case ODM_RF_PATH_A:
			{
			ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
				// ========Path-A AFE all on========
				// Port 0 DAC/ADC on
				ODM_Write4Byte(pDM_Odm, 0xc60, 0x77777777);
				ODM_Write4Byte(pDM_Odm, 0xc64, 0x77777777);
				 
				// Port 1 DAC/ADC off
				ODM_Write4Byte(pDM_Odm, 0xe60, 0x00000000);
				ODM_Write4Byte(pDM_Odm, 0xe64, 0x00000000);

					ODM_Write4Byte(pDM_Odm, 0xc68, 0x19791979);

					ODM_SetBBReg(pDM_Odm, 0xc00, 0xf, 0x4);// 	hardware 3-wire off

					// IQK Setting
					//====== TX IQK ======
					// 1. DAC/ADC sampling rate (160 MHz)
					ODM_SetBBReg(pDM_Odm, 0xc5c, BIT(26)|BIT(25)|BIT(24), 0x7);
			ODM_SetBBReg(pDM_Odm, 0x8c4, BIT(30), 0x1);
			//ODM_SetBBReg(pDM_Odm, 0xcb0, 0x000000f0, 0x7);
			ODM_SetBBReg(pDM_Odm, 0xcb0, 0x00ff0000, 0x77);
			ODM_SetBBReg(pDM_Odm, 0xcb4, 0x03000000, 0x0);
			}
			break;
		case ODM_RF_PATH_B:
			{ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
			// Port 0 DAC/ADC off
			ODM_Write4Byte(pDM_Odm, 0xc60, 0x00000000);
			ODM_Write4Byte(pDM_Odm, 0xc64, 0x00000000);
			 
			// Port 1 DAC/ADC on
			ODM_Write4Byte(pDM_Odm, 0xe60, 0x77777777);
			ODM_Write4Byte(pDM_Odm, 0xe64, 0x77777777);

			ODM_Write4Byte(pDM_Odm, 0xe68, 0x19791979);

			ODM_SetBBReg(pDM_Odm, 0xe00, 0xf, 0x4);// 	hardware 3-wire off

			// DAC/ADC sampling rate (160 MHz)
			ODM_SetBBReg(pDM_Odm, 0xe5c, BIT(26)|BIT(25)|BIT(24), 0x7);
			ODM_SetBBReg(pDM_Odm, 0x8c4, BIT(30), 0x1);
			//ODM_SetBBReg(pDM_Odm, 0xeb0, 0x000000f0, 0x7);
			ODM_SetBBReg(pDM_Odm, 0xeb0, 0x00ff0000, 0x77);
			ODM_SetBBReg(pDM_Odm, 0xeb4, 0x03000000, 0x0);			
			}
			break;
		default:
			break;
			}
	while (cal < cal_num){
		switch (Path) {
			case ODM_RF_PATH_A:
			{	
				//====== TX IQK ======
				ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
				ODM_SetRFReg(pDM_Odm, Path, 0xef, bRFRegOffsetMask, 0x80002);
				ODM_SetRFReg(pDM_Odm, Path, 0x30, bRFRegOffsetMask, 0x20000);
				ODM_SetRFReg(pDM_Odm, Path, 0x31, bRFRegOffsetMask, 0x3fffd);
				ODM_SetRFReg(pDM_Odm, Path, 0x32, bRFRegOffsetMask, 0xfe83f);
				ODM_SetRFReg(pDM_Odm, Path, 0x65, bRFRegOffsetMask, 0x931d5);
				ODM_SetRFReg(pDM_Odm, Path, 0x8f, bRFRegOffsetMask, 0x8a001);
				ODM_Write4Byte(pDM_Odm, 0x90c, 0x00008000);
				ODM_Write4Byte(pDM_Odm, 0xb00, 0x03000100);
				ODM_SetBBReg(pDM_Odm, 0xc94, BIT(0), 0x1);
				ODM_Write4Byte(pDM_Odm, 0x978, 0x29002000);// TX (X,Y)
				ODM_Write4Byte(pDM_Odm, 0x97c, 0xa9002000);// RX (X,Y)
				ODM_Write4Byte(pDM_Odm, 0x984, 0x00462910);// [0]:AGC_en, [15]:idac_K_Mask
				
				ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 1 --> Page C1

				if (pDM_Odm->ExtPA)
					ODM_Write4Byte(pDM_Odm, 0xc88, 0x821403e3);
				else
					ODM_Write4Byte(pDM_Odm, 0xc88, 0x821403f1);
				
				if (*pDM_Odm->pBandType)
					ODM_Write4Byte(pDM_Odm, 0xc8c, 0x68163e96);
				else
					ODM_Write4Byte(pDM_Odm, 0xc8c, 0x28163e96);
				
				if (VDF_enable == 1){
					ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("TXVDF Start\n"));
					for (k = 0;k <= 2; k++){
						switch (k){
							case 0:
								{
								ODM_Write4Byte(pDM_Odm, 0xc80, 0x18008c38);// TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16
								ODM_Write4Byte(pDM_Odm, 0xc84, 0x38008c38);// RX_Tone_idx[9:0], RxK_Mask[29]
								ODM_Write4Byte(pDM_Odm, 0x984, 0x00462910);// [0]:AGC_en, [15]:idac_K_Mask
								ODM_SetBBReg(pDM_Odm, 0xce8, BIT(31), 0x0);
								}
								break;
							case 1:
								{
								ODM_SetBBReg(pDM_Odm, 0xc80, BIT(28), 0x0);
								ODM_SetBBReg(pDM_Odm, 0xc84, BIT(28), 0x0);
								ODM_Write4Byte(pDM_Odm, 0x984, 0x0046a910);// [0]:AGC_en, [15]:idac_K_Mask
								}
								break;
							case 2:
								{
								ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("VDF_Y[1] = %x;;;VDF_Y[0] = %x\n", VDF_Y[1]>>21 & 0x00007ff, VDF_Y[0]>>21 & 0x00007ff));
								ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("VDF_X[1] = %x;;;VDF_X[0] = %x\n", VDF_X[1]>>21 & 0x00007ff, VDF_X[0]>>21 & 0x00007ff));
								Tx_dt[cal] = (VDF_Y[1]>>20)-(VDF_Y[0]>>20);
								ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Tx_dt = %d\n", Tx_dt[cal]));
								Tx_dt[cal] = ((16*Tx_dt[cal])*10000/15708);
								Tx_dt[cal] = (Tx_dt[cal] >> 1 )+(Tx_dt[cal] & BIT(0));
								ODM_Write4Byte(pDM_Odm, 0xc80, 0x18008c20);// TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16
								ODM_Write4Byte(pDM_Odm, 0xc84, 0x38008c20);// RX_Tone_idx[9:0], RxK_Mask[29]
								ODM_SetBBReg(pDM_Odm, 0xce8, BIT(31), 0x1);
								ODM_SetBBReg(pDM_Odm, 0xce8, 0x3fff0000, Tx_dt[cal] & 0x00003fff);
								}
								break;
							default:
								break;
						}
				ODM_Write4Byte(pDM_Odm, 0xcb8, 0x00100000);// cb8[20] ±N SI/PI ¨?¥??v¤?µ¹ iqk_dpk module
				cal_retry = 0;
				while(1){
					// one shot
					ODM_Write4Byte(pDM_Odm, 0x980, 0xfa000000);
					ODM_Write4Byte(pDM_Odm, 0x980, 0xf8000000);

								delay_ms(10); //Delay 10ms
								ODM_Write4Byte(pDM_Odm, 0xcb8, 0x00000000);
								delay_count = 0;
				                    while (1){
									IQK_ready = ODM_GetBBReg(pDM_Odm, 0xd00, BIT(10));
				                        	if ((IQK_ready) || (delay_count>20)){
										break;
				                        	}
								else{
										delay_ms(1);
										delay_count++;
									}
								}

					if (delay_count < 20){							// If 20ms No Result, then cal_retry++
			                     // ============TXIQK Check==============
						TX_fail = ODM_GetBBReg(pDM_Odm, 0xd00, BIT(12));
						
						if (~TX_fail){
							ODM_Write4Byte(pDM_Odm, 0xcb8, 0x02000000);
									VDF_X[k] = ODM_GetBBReg(pDM_Odm, 0xd00, 0x07ff0000)<<21;
							ODM_Write4Byte(pDM_Odm, 0xcb8, 0x04000000);
									VDF_Y[k] = ODM_GetBBReg(pDM_Odm, 0xd00, 0x07ff0000)<<21;
							TX0IQKOK = TRUE;
							break;
						}
								else{
							TX0IQKOK = FALSE;
							cal_retry++;
							if (cal_retry == 10) {
								break;
							}
						}
					}
                    else{
                        TX0IQKOK = FALSE;
                        cal_retry++;
                        if (cal_retry == 10){
                            break;
                        }
                    }
                }
					}
					ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("TXA_VDF_cal_retry = %d\n", cal_retry));
						TX_X0[cal] = VDF_X[k-1] ;
						TX_Y0[cal] = VDF_Y[k-1];
				}
				else{
					ODM_Write4Byte(pDM_Odm, 0xc80, 0x18008c10);// TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16
					ODM_Write4Byte(pDM_Odm, 0xc84, 0x38008c10);// RX_Tone_idx[9:0], RxK_Mask[29]
					ODM_Write4Byte(pDM_Odm, 0xce8, 0x00000000);
					ODM_Write4Byte(pDM_Odm, 0xcb8, 0x00100000);// cb8[20] ±N SI/PI ¨?¥??v¤?µ¹ iqk_dpk module
					cal_retry = 0;
					while(1){
						ODM_Write4Byte(pDM_Odm, 0x980, 0xfa000000);
						ODM_Write4Byte(pDM_Odm, 0x980, 0xf8000000);

						delay_ms(10); //Delay 25ms
						ODM_Write4Byte(pDM_Odm, 0xcb8, 0x00000000);
						delay_count = 0;
						while (1){
							IQK_ready = ODM_GetBBReg(pDM_Odm, 0xd00, BIT(10));
							if ((IQK_ready) || (delay_count>20)) {
						       	break;
						}
						else{
							delay_ms(1);
						       delay_count++;
						}
						}

						if (delay_count < 20){							// If 20ms No Result, then cal_retry++
						TX_fail = ODM_GetBBReg(pDM_Odm, 0xd00, BIT(12));
							if (~TX_fail){
								ODM_Write4Byte(pDM_Odm, 0xcb8, 0x02000000);
								TX_X0[cal] = ODM_GetBBReg(pDM_Odm, 0xd00, 0x07ff0000)<<21;
								ODM_Write4Byte(pDM_Odm, 0xcb8, 0x04000000);
								TX_Y0[cal] = ODM_GetBBReg(pDM_Odm, 0xd00, 0x07ff0000)<<21;
								TX0IQKOK = TRUE;
								/*
									
									ODM_Write4Byte(pDM_Odm, 0xcb8, 0x01000000);
									reg1 = ODM_GetBBReg(pDM_Odm, 0xd00, 0xffffffff);
									ODM_Write4Byte(pDM_Odm, 0xcb8, 0x02000000);
									reg2 = ODM_GetBBReg(pDM_Odm, 0xd00, 0x0000001f);
									Image_Power = (reg2<<32)+reg1;
									DbgPrint("Before PW = %d\n", Image_Power);
									ODM_Write4Byte(pDM_Odm, 0xcb8, 0x03000000);
									reg1 = ODM_GetBBReg(pDM_Odm, 0xd00, 0xffffffff);
									ODM_Write4Byte(pDM_Odm, 0xcb8, 0x04000000);
									reg2 = ODM_GetBBReg(pDM_Odm, 0xd00, 0x0000001f);
									Image_Power = (reg2<<32)+reg1;
									DbgPrint("After PW = %d\n", Image_Power);
								*/
								break;
							}
							else{
								TX0IQKOK = FALSE;
								cal_retry++;
								if (cal_retry == 10) {
									break;
								}
							}
						}
		                    		else{
		                        		TX0IQKOK = FALSE;
		                        		cal_retry++;
		                        		if (cal_retry == 10)
		                            		break;	
		                    		}
		                	}
					ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("TXA_cal_retry = %d\n", cal_retry));
				}
			ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
			ODM_SetRFReg(pDM_Odm, Path, 0x58, 0x7fe00, ODM_GetRFReg(pDM_Odm, Path, 0x8, 0xffc00)); // Load LOK
			ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 1 --> Page C1
				if (TX0IQKOK == FALSE)
					break;				// TXK fail, Don't do RXK


			if (VDF_enable == 1){
				ODM_SetBBReg(pDM_Odm, 0xce8, BIT(31), 0x0);    // TX VDF Disable
				ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RXVDF Start\n"));
				
					//====== RX mode TXK (RXK Step 1) ======
					ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
					ODM_SetRFReg(pDM_Odm, Path, 0xef, bRFRegOffsetMask, 0x80000);
					ODM_SetRFReg(pDM_Odm, Path, 0x30, bRFRegOffsetMask, 0x30000);
					ODM_SetRFReg(pDM_Odm, Path, 0x31, bRFRegOffsetMask, 0x3f7ff);
					ODM_SetRFReg(pDM_Odm, Path, 0x32, bRFRegOffsetMask, 0xfe7bf);
					ODM_SetRFReg(pDM_Odm, Path, 0x8f, bRFRegOffsetMask, 0x88001);
					ODM_SetRFReg(pDM_Odm, Path, 0x65, bRFRegOffsetMask, 0x931d0);
					ODM_SetRFReg(pDM_Odm, Path, 0xef, bRFRegOffsetMask, 0x00000);
					ODM_SetBBReg(pDM_Odm, 0x978, BIT(31), 0x1);
					ODM_SetBBReg(pDM_Odm, 0x97c, BIT(31), 0x0);
					ODM_Write4Byte(pDM_Odm, 0x984, 0x0046a911);
					
					ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 1 --> Page C1
					ODM_Write4Byte(pDM_Odm, 0xc88, 0x02140119);
					ODM_Write4Byte(pDM_Odm, 0xc8c, 0x28161420);
					
				for (k = 0;k <= 2; k++){	
					ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
					ODM_SetBBReg(pDM_Odm, 0x978, 0x03FF8000, (VDF_X[k])>>21&0x000007ff);
			              ODM_SetBBReg(pDM_Odm, 0x978, 0x000007FF, (VDF_Y[k])>>21&0x000007ff);
					
					
					ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 1 --> Page C1
					switch (k){
						case 0:
							{
							ODM_Write4Byte(pDM_Odm, 0xc80, 0x38008c38);// TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16
							ODM_Write4Byte(pDM_Odm, 0xc84, 0x18008c38);// RX_Tone_idx[9:0], RxK_Mask[29]
							ODM_SetBBReg(pDM_Odm, 0xce8, BIT(30), 0x0);
							}
							break;
						case 1:
							{
							ODM_Write4Byte(pDM_Odm, 0xc80, 0x28008c38);// TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16
							ODM_Write4Byte(pDM_Odm, 0xc84, 0x08008c38);// RX_Tone_idx[9:0], RxK_Mask[29]
							}
							break;
						case 2:
							{
							ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("VDF_Y[1] = %x;;;VDF_Y[0] = %x\n", VDF_Y[1]>>21 & 0x00007ff, VDF_Y[0]>>21 & 0x00007ff));
							ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("VDF_X[1] = %x;;;VDF_X[0] = %x\n", VDF_X[1]>>21 & 0x00007ff, VDF_X[0]>>21 & 0x00007ff));
							Rx_dt[cal] = (VDF_Y[1]>>20)-(VDF_Y[0]>>20);
							ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Rx_dt = %d\n", Rx_dt[cal]));
							Rx_dt[cal] = ((16*Rx_dt[cal])*10000/13823);
							Rx_dt[cal] = (Rx_dt[cal] >> 1 )+(Rx_dt[cal] & BIT(0));
							ODM_Write4Byte(pDM_Odm, 0xc80, 0x38008c20);// TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16
							ODM_Write4Byte(pDM_Odm, 0xc84, 0x18008c20);// RX_Tone_idx[9:0], RxK_Mask[29]
							ODM_SetBBReg(pDM_Odm, 0xce8, 0x00003fff, Rx_dt[cal] & 0x00003fff);
							}
							break;
						default:
							break;
					}
	
					
					if (k==2){
						ODM_SetBBReg(pDM_Odm, 0xce8, BIT(30), 0x1);  //RX VDF Enable
						}
					ODM_Write4Byte(pDM_Odm, 0xcb8, 0x00100000);// cb8[20] ±N SI/PI ¨?¥??v¤?µ¹ iqk_dpk module
					
					cal_retry = 0;
					while(1){
						// one shot
						ODM_Write4Byte(pDM_Odm, 0x980, 0xfa000000);
						ODM_Write4Byte(pDM_Odm, 0x980, 0xf8000000);

						delay_ms(10); //Delay 10ms
						ODM_Write4Byte(pDM_Odm, 0xcb8, 0x00000000);
						delay_count = 0;
						while (1){
							IQK_ready = ODM_GetBBReg(pDM_Odm, 0xd00, BIT(10));
							if ((IQK_ready)||(delay_count>20)){
								break;
							}
							else{
								delay_ms(1);
								delay_count++;
							}
						}
							
						if (delay_count < 20){	// If 20ms No Result, then cal_retry++
							// ============RXIQK Check==============
							RX_fail = ODM_GetBBReg(pDM_Odm, 0xd00, BIT(11));
							if (RX_fail == 0){
								ODM_Write4Byte(pDM_Odm, 0xcb8, 0x06000000);
								VDF_X[k] = ODM_GetBBReg(pDM_Odm, 0xd00, 0x07ff0000)<<21;
								ODM_Write4Byte(pDM_Odm, 0xcb8, 0x08000000);
								VDF_Y[k] = ODM_GetBBReg(pDM_Odm, 0xd00, 0x07ff0000)<<21;
								RX0IQKOK = TRUE;
								break;
							}
							else{
								ODM_SetBBReg(pDM_Odm, 0xc10, 0x000003ff, 0x200>>1);
								ODM_SetBBReg(pDM_Odm, 0xc10, 0x03ff0000, 0x0>>1);
								RX0IQKOK = FALSE;
								cal_retry++;
								if (cal_retry == 10)
									break;
									
							}
						}
						else{
				                    	RX0IQKOK = FALSE;
				                    	cal_retry++;
				                    	if (cal_retry == 10)
				                     	break;
			                		}
			            	}
					
				}
				ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RXA_VDF_cal_retry = %d\n", cal_retry));
					RX_X0[cal] = VDF_X[k-1] ;
					RX_Y0[cal] = VDF_Y[k-1];
			ODM_SetBBReg(pDM_Odm, 0xce8, BIT(31), 0x1);    // TX VDF Enable
			}
			else{
				//====== RX mode TXK (RXK Step 1) ======
				ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
				// 1. TX RF Setting
					ODM_SetRFReg(pDM_Odm, Path, 0xef, bRFRegOffsetMask, 0x80000);
					ODM_SetRFReg(pDM_Odm, Path, 0x30, bRFRegOffsetMask, 0x30000);
				ODM_SetRFReg(pDM_Odm, Path, 0x31, bRFRegOffsetMask, 0x3f7ff);
				ODM_SetRFReg(pDM_Odm, Path, 0x32, bRFRegOffsetMask, 0xfe7bf);
				ODM_SetRFReg(pDM_Odm, Path, 0x8f, bRFRegOffsetMask, 0x88001);
				ODM_SetRFReg(pDM_Odm, Path, 0x65, bRFRegOffsetMask, 0x931d0);
					ODM_SetRFReg(pDM_Odm, Path, 0xef, bRFRegOffsetMask, 0x00000);
				ODM_SetBBReg(pDM_Odm, 0x978, 0x03FF8000, (TX_X0[cal])>>21&0x000007ff);
		              ODM_SetBBReg(pDM_Odm, 0x978, 0x000007FF, (TX_Y0[cal])>>21&0x000007ff);
				ODM_SetBBReg(pDM_Odm, 0x978, BIT(31), 0x1);
				ODM_SetBBReg(pDM_Odm, 0x97c, BIT(31), 0x0);
					ODM_Write4Byte(pDM_Odm, 0x90c, 0x00008000);
				ODM_Write4Byte(pDM_Odm, 0x984, 0x0046a910);

					ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 1 --> Page C1
				ODM_Write4Byte(pDM_Odm, 0xc80, 0x38008c10);// TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16
				ODM_Write4Byte(pDM_Odm, 0xc84, 0x18008c10);// RX_Tone_idx[9:0], RxK_Mask[29]
				ODM_Write4Byte(pDM_Odm, 0xc88, 0x02140119);
				ODM_Write4Byte(pDM_Odm, 0xc8c, 0x28161cc0);
					ODM_Write4Byte(pDM_Odm, 0xcb8, 0x00100000);// cb8[20] ±N SI/PI ¨?¥??v¤?µ¹ iqk_dpk module
					cal_retry = 0;
				while(1){
						// one shot
						ODM_Write4Byte(pDM_Odm, 0x980, 0xfa000000);
						ODM_Write4Byte(pDM_Odm, 0x980, 0xf8000000);

					delay_ms(10); //Delay 10ms
						ODM_Write4Byte(pDM_Odm, 0xcb8, 0x00000000);
						delay_count = 0;
						while (1){
							IQK_ready = ODM_GetBBReg(pDM_Odm, 0xd00, BIT(10));
						if ((IQK_ready)||(delay_count>20)){
								break;
						}
						else{
								delay_ms(1);
								delay_count++;
							}
						}
						
					if (delay_count < 20){	// If 20ms No Result, then cal_retry++
							// ============RXIQK Check==============
							RX_fail = ODM_GetBBReg(pDM_Odm, 0xd00, BIT(11));
						if (RX_fail == 0){
								ODM_Write4Byte(pDM_Odm, 0xcb8, 0x06000000);
								RX_X0[cal] = ODM_GetBBReg(pDM_Odm, 0xd00, 0x07ff0000)<<21;
								ODM_Write4Byte(pDM_Odm, 0xcb8, 0x08000000);
								RX_Y0[cal] = ODM_GetBBReg(pDM_Odm, 0xd00, 0x07ff0000)<<21;
								RX0IQKOK = TRUE;
/*
							ODM_Write4Byte(pDM_Odm, 0xcb8, 0x05000000);
							reg1 = ODM_GetBBReg(pDM_Odm, 0xd00, 0xffffffff);
							ODM_Write4Byte(pDM_Odm, 0xcb8, 0x06000000);
							reg2 = ODM_GetBBReg(pDM_Odm, 0xd00, 0x0000001f);
							Image_Power = (reg2<<32)+reg1;
							DbgPrint("Before PW = %d\n", Image_Power);
							ODM_Write4Byte(pDM_Odm, 0xcb8, 0x07000000);
							reg1 = ODM_GetBBReg(pDM_Odm, 0xd00, 0xffffffff);
							ODM_Write4Byte(pDM_Odm, 0xcb8, 0x08000000);
							reg2 = ODM_GetBBReg(pDM_Odm, 0xd00, 0x0000001f);
							Image_Power = (reg2<<32)+reg1;
							DbgPrint("After PW = %d\n", Image_Power);
*/
								break;
							}
						else{
								ODM_SetBBReg(pDM_Odm, 0xc10, 0x000003ff, 0x200>>1);
								ODM_SetBBReg(pDM_Odm, 0xc10, 0x03ff0000, 0x0>>1);
								RX0IQKOK = FALSE;
								cal_retry++;
							if (cal_retry == 10)
									break;
								
							}
						}
					else{
							RX0IQKOK = FALSE;
							cal_retry++;
							if (cal_retry == 10)
								break;
						}
					}	
				ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RXA_cal_retry = %d\n", cal_retry));
			}
					if (TX0IQKOK)
						TX_Average++;
					if (RX0IQKOK)
						RX_Average++;
				}
				break;
			case ODM_RF_PATH_B:
				{
			//Path-B TX/RX IQK
			ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
			ODM_SetRFReg(pDM_Odm, Path, 0xef, bRFRegOffsetMask, 0x80002);
			ODM_SetRFReg(pDM_Odm, Path, 0x30, bRFRegOffsetMask, 0x20000);
			ODM_SetRFReg(pDM_Odm, Path, 0x31, bRFRegOffsetMask, 0x3fffd);
			ODM_SetRFReg(pDM_Odm, Path, 0x32, bRFRegOffsetMask, 0xfe83f);
			ODM_SetRFReg(pDM_Odm, Path, 0x65, bRFRegOffsetMask, 0x931d5);
			ODM_SetRFReg(pDM_Odm, Path, 0x8f, bRFRegOffsetMask, 0x8a001);
					ODM_Write4Byte(pDM_Odm, 0x90c, 0x00008000);
					ODM_Write4Byte(pDM_Odm, 0xb00, 0x03000100);
					ODM_SetBBReg(pDM_Odm, 0xe94, BIT(0), 0x1);
					ODM_Write4Byte(pDM_Odm, 0x978, 0x29002000);// TX (X,Y)
					ODM_Write4Byte(pDM_Odm, 0x97c, 0xa9002000);// RX (X,Y)
					ODM_Write4Byte(pDM_Odm, 0x984, 0x00462910);// [0]:AGC_en, [15]:idac_K_Mask


                ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 1 --> Page C1
			
			

			if (pDM_Odm->ExtPA)
				ODM_Write4Byte(pDM_Odm, 0xe88, 0x821403e3);
			else
				ODM_Write4Byte(pDM_Odm, 0xe88, 0x821403f1);

                if (*pDM_Odm->pBandType)
                    ODM_Write4Byte(pDM_Odm, 0xe8c, 0x68163e96);
                else
                    ODM_Write4Byte(pDM_Odm, 0xe8c, 0x28163e96);

			if (VDF_enable == 1){
				for (k = 0;k <= 2; k++){
					switch (k){
						case 0:
                {
                    // one shot
							ODM_Write4Byte(pDM_Odm, 0xe80, 0x18008c38);// TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16
							ODM_Write4Byte(pDM_Odm, 0xe84, 0x38008c38);// RX_Tone_idx[9:0], RxK_Mask[29]
							ODM_Write4Byte(pDM_Odm, 0x984, 0x00462910);
							ODM_SetBBReg(pDM_Odm, 0xee8, BIT(31), 0x0);
								}
                            break;
						case 1:
							{
							ODM_SetBBReg(pDM_Odm, 0xe80, BIT(28), 0x0);
							ODM_SetBBReg(pDM_Odm, 0xe84, BIT(28), 0x0);
							ODM_Write4Byte(pDM_Odm, 0x984, 0x0046a910);
							ODM_SetBBReg(pDM_Odm, 0xee8, BIT(31), 0x0);
                    }
							break;
						case 2:
							{
							ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("VDF_Y[1] = %x;;;VDF_Y[0] = %x\n", VDF_Y[1]>>21 & 0x00007ff, VDF_Y[0]>>21 & 0x00007ff));
							ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("VDF_X[1] = %x;;;VDF_X[0] = %x\n", VDF_X[1]>>21 & 0x00007ff, VDF_X[0]>>21 & 0x00007ff));
							Tx_dt[cal] = (VDF_Y[1]>>20)-(VDF_Y[0]>>20);
							ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Tx_dt = %d\n", Tx_dt[cal]));
							Tx_dt[cal] = ((16*Tx_dt[cal])*10000/15708);
							Tx_dt[cal] = (Tx_dt[cal] >> 1 )+(Tx_dt[cal] & BIT(0));
							ODM_Write4Byte(pDM_Odm, 0xe80, 0x18008c20);// TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16
							ODM_Write4Byte(pDM_Odm, 0xe84, 0x38008c20);// RX_Tone_idx[9:0], RxK_Mask[29]
							ODM_SetBBReg(pDM_Odm, 0xee8, BIT(31), 0x1);
							ODM_SetBBReg(pDM_Odm, 0xee8, 0x3fff0000, Tx_dt[cal] & 0x00003fff);
								}
							break;
						default:
							break;
						}
						ODM_Write4Byte(pDM_Odm, 0xeb8, 0x00100000);// cb8[20] ±N SI/PI ¨?¥??v¤?µ¹ iqk_dpk module
						cal_retry = 0;
						while(1){
							// one shot
							ODM_Write4Byte(pDM_Odm, 0x980, 0xfa000000);
							ODM_Write4Byte(pDM_Odm, 0x980, 0xf8000000);

							delay_ms(10); //Delay 10ms
							ODM_Write4Byte(pDM_Odm, 0xeb8, 0x00000000);
							delay_count = 0;
							while (1){
								IQK_ready = ODM_GetBBReg(pDM_Odm, 0xd40, BIT(10));
			                        	if ((IQK_ready) || (delay_count>20)) {
									break;
			                        	}
							else {
									delay_ms(1);
									delay_count++;
								}
							}

							if (delay_count < 20){							// If 20ms No Result, then cal_retry++
								// ============TXIQK Check==============
								TX_fail = ODM_GetBBReg(pDM_Odm, 0xd40, BIT(12));

								if (~TX_fail){
									ODM_Write4Byte(pDM_Odm, 0xeb8, 0x02000000);
								VDF_X[k] = ODM_GetBBReg(pDM_Odm, 0xd40, 0x07ff0000)<<21;
									ODM_Write4Byte(pDM_Odm, 0xeb8, 0x04000000);
								VDF_Y[k] = ODM_GetBBReg(pDM_Odm, 0xd40, 0x07ff0000)<<21;
									TX1IQKOK = TRUE;
									break;
								}
							else{
									TX1IQKOK = FALSE;
									cal_retry++;
									if (cal_retry == 10){
										break;
									}
								}
							}
			                     else{
			                    		TX1IQKOK = FALSE;
			                        	cal_retry++;
			                        	if (cal_retry == 10){
			                            	break;
			                        	}
			                    	}
			       	}
				}
				ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("TXB_VDF_cal_retry = %d\n", cal_retry));
					TX_X1[cal] = VDF_X[k-1] ;
					TX_Y1[cal] = VDF_Y[k-1];
			}
			else{
				ODM_Write4Byte(pDM_Odm, 0xe80, 0x18008c10);// TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16
				ODM_Write4Byte(pDM_Odm, 0xe84, 0x38008c10);// RX_Tone_idx[9:0], RxK_Mask[29]
				ODM_Write4Byte(pDM_Odm, 0xee8, 0x00000000);
				ODM_Write4Byte(pDM_Odm, 0xeb8, 0x00100000);// cb8[20] ±N SI/PI ¨?¥??v¤?µ¹ iqk_dpk module
				cal_retry = 0;
	                	while(1){
	                    	// one shot
	                    		ODM_Write4Byte(pDM_Odm, 0x980, 0xfa000000);
	                    		ODM_Write4Byte(pDM_Odm, 0x980, 0xf8000000);

	                    		delay_ms(10); //Delay 25ms
	                    		ODM_Write4Byte(pDM_Odm, 0xeb8, 0x00000000);
	                    		delay_count = 0;
	                    		while (1){
	                        		IQK_ready = ODM_GetBBReg(pDM_Odm, 0xd40, BIT(10));
	                        		if ((IQK_ready)||(delay_count>20)){
	                            		break;
	                        		}
						else{
	                            		delay_ms(1);
	                            		delay_count++;
	                        		}
	                    		}

		                    	if (delay_count < 20){							// If 20ms No Result, then cal_retry++
		                        	// ============TXIQK Check==============
		                        	TX_fail = ODM_GetBBReg(pDM_Odm, 0xd40, BIT(12));
		                        	if (~TX_fail){
		                            	ODM_Write4Byte(pDM_Odm, 0xeb8, 0x02000000);
			                            TX_X1[cal] = ODM_GetBBReg(pDM_Odm, 0xd40, 0x07ff0000)<<21;
			                            ODM_Write4Byte(pDM_Odm, 0xeb8, 0x04000000);
			                            TX_Y1[cal] = ODM_GetBBReg(pDM_Odm, 0xd40, 0x07ff0000)<<21;
			                            TX1IQKOK = TRUE;
							/*
								int			reg1 = 0, reg2 = 0, Image_Power = 0;
								ODM_Write4Byte(pDM_Odm, 0xeb8, 0x01000000);
								reg1 = ODM_GetBBReg(pDM_Odm, 0xd40, 0xffffffff);
								ODM_Write4Byte(pDM_Odm, 0xeb8, 0x02000000);
								reg2 = ODM_GetBBReg(pDM_Odm, 0xd40, 0x0000001f);
								Image_Power = (reg2<<32)+reg1;
								DbgPrint("Before PW = %d\n", Image_Power);
								ODM_Write4Byte(pDM_Odm, 0xeb8, 0x03000000);
								reg1 = ODM_GetBBReg(pDM_Odm, 0xd40, 0xffffffff);
								ODM_Write4Byte(pDM_Odm, 0xeb8, 0x04000000);
								reg2 = ODM_GetBBReg(pDM_Odm, 0xd40, 0x0000001f);
								Image_Power = (reg2<<32)+reg1;
								DbgPrint("After PW = %d\n", Image_Power);
							*/
			                            break;
						}
						else{
					        	TX1IQKOK = FALSE;
					        	cal_retry++;
					        	if (cal_retry == 10){
						       	break;
						       }
						}
					}
		                    	else {
		                   		TX1IQKOK = FALSE;
		                        	cal_retry++;
		                        	if (cal_retry == 10){
		                            	break;
		                        	}
		                    	}
		        	}
						ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("TXB_cal_retry = %d\n", cal_retry));
			}

			ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
			ODM_SetRFReg(pDM_Odm, Path, 0x58, 0x7fe00, ODM_GetRFReg(pDM_Odm, Path, 0x8, 0xffc00)); // Load LOK
			ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 1 --> Page C1
			
			if (TX1IQKOK == FALSE)
			    	break;				// TXK fail, Don't do RXK
			


				if (VDF_enable == 1){
				ODM_SetBBReg(pDM_Odm, 0xee8, BIT(31), 0x0);    // TX VDF Disable
				ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RXVDF Start\n"));
				
					//====== RX IQK ======
			            	ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
					ODM_SetRFReg(pDM_Odm, Path, 0xef, bRFRegOffsetMask, 0x80000);
					ODM_SetRFReg(pDM_Odm, Path, 0x30, bRFRegOffsetMask, 0x30000);
					ODM_SetRFReg(pDM_Odm, Path, 0x31, bRFRegOffsetMask, 0x3f7ff);
					ODM_SetRFReg(pDM_Odm, Path, 0x32, bRFRegOffsetMask, 0xfe7bf);
					ODM_SetRFReg(pDM_Odm, Path, 0x8f, bRFRegOffsetMask, 0x88001);
					ODM_SetRFReg(pDM_Odm, Path, 0x65, bRFRegOffsetMask, 0x931d0);
					ODM_SetRFReg(pDM_Odm, Path, 0xef, bRFRegOffsetMask, 0x00000);
					
					ODM_SetBBReg(pDM_Odm, 0x978, BIT(31), 0x1);
					ODM_SetBBReg(pDM_Odm, 0x97c, BIT(31), 0x0);
					ODM_Write4Byte(pDM_Odm, 0x984, 0x0046a911);
					ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 1 --> Page C1
					ODM_Write4Byte(pDM_Odm, 0xe88, 0x02140119);
					ODM_Write4Byte(pDM_Odm, 0xe8c, 0x28161420);
					
				for (k = 0;k <= 2; k++){
					ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
					ODM_SetBBReg(pDM_Odm, 0x978, 0x03FF8000, (VDF_X[k])>>21&0x000007ff);
			              ODM_SetBBReg(pDM_Odm, 0x978, 0x000007FF, (VDF_Y[k])>>21&0x000007ff);
					ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 1 --> Page C1
					switch (k){
						case 0:
							{
							ODM_Write4Byte(pDM_Odm, 0xe80, 0x38008c38);// TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16
							ODM_Write4Byte(pDM_Odm, 0xe84, 0x18008c38);// RX_Tone_idx[9:0], RxK_Mask[29]
							ODM_SetBBReg(pDM_Odm, 0xee8, BIT(30), 0x0);
							}
							break;
						case 1:
							{
							ODM_Write4Byte(pDM_Odm, 0xe80, 0x28008c38);// TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16
							ODM_Write4Byte(pDM_Odm, 0xe84, 0x08008c38);// RX_Tone_idx[9:0], RxK_Mask[29]
							ODM_SetBBReg(pDM_Odm, 0xee8, BIT(30), 0x0);
							}
							break;
						case 2:
							{
							ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("VDF_Y[1] = %x;;;VDF_Y[0] = %x\n", VDF_Y[1]>>21 & 0x00007ff, VDF_Y[0]>>21 & 0x00007ff));
							ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("VDF_X[1] = %x;;;VDF_X[0] = %x\n", VDF_X[1]>>21 & 0x00007ff, VDF_X[0]>>21 & 0x00007ff));
							Rx_dt[cal] = (VDF_Y[1]>>20)-(VDF_Y[0]>>20);
							ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Rx_dt = %d\n", Rx_dt[cal]));
							Rx_dt[cal] = ((16*Rx_dt[cal])*10000/13823);
							Rx_dt[cal] = (Rx_dt[cal] >> 1 )+(Rx_dt[cal] & BIT(0));
							ODM_Write4Byte(pDM_Odm, 0xe80, 0x38008c20);// TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16
							ODM_Write4Byte(pDM_Odm, 0xe84, 0x18008c20);// RX_Tone_idx[9:0], RxK_Mask[29]
							ODM_SetBBReg(pDM_Odm, 0xee8, 0x00003fff, Rx_dt[cal] & 0x00003fff);
							}
							break;
						default:
							break;
					}
					
					
					if (k==2){
						ODM_SetBBReg(pDM_Odm, 0xee8, BIT(30), 0x1);  //RX VDF Enable
						}
					ODM_Write4Byte(pDM_Odm, 0xeb8, 0x00100000);// cb8[20] ±N SI/PI ¨?¥??v¤?µ¹ iqk_dpk module
					
					cal_retry = 0;
					while(1){
						// one shot
						ODM_Write4Byte(pDM_Odm, 0x980, 0xfa000000);
						ODM_Write4Byte(pDM_Odm, 0x980, 0xf8000000);

						delay_ms(10); //Delay 10ms
						ODM_Write4Byte(pDM_Odm, 0xeb8, 0x00000000);
						delay_count = 0;
						while (1){
							IQK_ready = ODM_GetBBReg(pDM_Odm, 0xd40, BIT(10));
							if ((IQK_ready)||(delay_count>20)){
								break;
							}
							else{
								delay_ms(1);
								delay_count++;
							}
						}
							
						if (delay_count < 20){	// If 20ms No Result, then cal_retry++
							// ============RXIQK Check==============
							RX_fail = ODM_GetBBReg(pDM_Odm, 0xd40, BIT(11));
							if (RX_fail == 0){
								ODM_Write4Byte(pDM_Odm, 0xeb8, 0x06000000);
								VDF_X[k] = ODM_GetBBReg(pDM_Odm, 0xd40, 0x07ff0000)<<21;
								ODM_Write4Byte(pDM_Odm, 0xeb8, 0x08000000);
								VDF_Y[k] = ODM_GetBBReg(pDM_Odm, 0xd40, 0x07ff0000)<<21;
								RX1IQKOK = TRUE;
								break;
							}
							else{
								ODM_SetBBReg(pDM_Odm, 0xe10, 0x000003ff, 0x200>>1);
								ODM_SetBBReg(pDM_Odm, 0xe10, 0x03ff0000, 0x0>>1);
								RX1IQKOK = FALSE;
								cal_retry++;
								if (cal_retry == 10)
									break;
									
							}
						}
						else{
				                    	RX1IQKOK = FALSE;
				                    	cal_retry++;
				                    	if (cal_retry == 10)
				                     	break;
			                		}
			            	}
					
				}
				ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RXB_VDF_cal_retry = %d\n", cal_retry));
					RX_X1[cal] = VDF_X[k-1] ;
					RX_Y1[cal] = VDF_Y[k-1];
				ODM_SetBBReg(pDM_Odm, 0xee8, BIT(31), 0x1);    // TX VDF Enable
				}
				else{
					//====== RX mode TXK (RXK Step 1) ======
					ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
					ODM_SetRFReg(pDM_Odm, Path, 0xef, bRFRegOffsetMask, 0x80000);
					ODM_SetRFReg(pDM_Odm, Path, 0x30, bRFRegOffsetMask, 0x30000);
					ODM_SetRFReg(pDM_Odm, Path, 0x31, bRFRegOffsetMask, 0x3f7ff);
					ODM_SetRFReg(pDM_Odm, Path, 0x32, bRFRegOffsetMask, 0xfe7bf);
					ODM_SetRFReg(pDM_Odm, Path, 0x8f, bRFRegOffsetMask, 0x88001);
					ODM_SetRFReg(pDM_Odm, Path, 0x65, bRFRegOffsetMask, 0x931d0);
					ODM_SetRFReg(pDM_Odm, Path, 0xef, bRFRegOffsetMask, 0x00000);

					ODM_SetBBReg(pDM_Odm, 0x978, 0x03FF8000, (TX_X1[cal])>>21&0x000007ff);
					ODM_SetBBReg(pDM_Odm, 0x978, 0x000007FF, (TX_Y1[cal])>>21&0x000007ff);
					ODM_SetBBReg(pDM_Odm, 0x978, BIT(31), 0x1);
					ODM_SetBBReg(pDM_Odm, 0x97c, BIT(31), 0x0);
					ODM_Write4Byte(pDM_Odm, 0x90c, 0x00008000);
					ODM_Write4Byte(pDM_Odm, 0x984, 0x0046a911);

					ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 1 --> Page C1
					ODM_Write4Byte(pDM_Odm, 0xe80, 0x38008c15);// TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16
					ODM_Write4Byte(pDM_Odm, 0xe84, 0x18008c15);// RX_Tone_idx[9:0], RxK_Mask[29]
					ODM_Write4Byte(pDM_Odm, 0xe88, 0x02140119);
					ODM_Write4Byte(pDM_Odm, 0xe8c, 0x28161cc0);
					ODM_Write4Byte(pDM_Odm, 0xeb8, 0x00100000);// cb8[20] ???SI/PI 使用�?????iqk_dpk module

					cal_retry = 0;
	                    		while(1){
		                        	// one shot
		                        	ODM_Write4Byte(pDM_Odm, 0x980, 0xfa000000);
		                        	ODM_Write4Byte(pDM_Odm, 0x980, 0xf8000000);

						delay_ms(10); //Delay 10ms
	                        		ODM_Write4Byte(pDM_Odm, 0xeb8, 0x00000000);
	                        		delay_count = 0;
	                        		while (1){
			                            IQK_ready = ODM_GetBBReg(pDM_Odm, 0xd40, BIT(10));
			                            if ((IQK_ready)||(delay_count>20)){
			                                	break;
		                            	}
							else{
		                                		delay_ms(1);
		                                		delay_count++;
		                            	}
		                        	}

		                        	if (delay_count < 20){							// If 20ms No Result, then cal_retry++
		                            // ============TXIQK Check==============
								RX_fail = ODM_GetBBReg(pDM_Odm, 0xd40, BIT(11));
								if (RX_fail == 0){
									ODM_Write4Byte(pDM_Odm, 0xeb8, 0x06000000);
									RX_X1[cal] = ODM_GetBBReg(pDM_Odm, 0xd40, 0x07ff0000)<<21;
									ODM_Write4Byte(pDM_Odm, 0xeb8, 0x08000000);
									RX_Y1[cal] = ODM_GetBBReg(pDM_Odm, 0xd40, 0x07ff0000)<<21;
									RX1IQKOK = TRUE;
									break;
								}
		                            	else{
									ODM_SetBBReg(pDM_Odm, 0xe10, 0x000003ff, 0x200>>1);
									ODM_SetBBReg(pDM_Odm, 0xe10, 0x03ff0000, 0x0>>1);
									RX1IQKOK = FALSE;
									cal_retry++;
		                                		if (cal_retry == 10)
										break;
									}
								}
							else{
								RX1IQKOK = FALSE;
								cal_retry++;
	                            		if (cal_retry == 10)
									break;
								}
							}
				ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RXB_cal_retry = %d\n", cal_retry));
						}


            		if (RX1IQKOK)
							RX_Average++;
            		
                	
			if (TX1IQKOK)
						TX_Average++;
			
				}
				break;		
			default:
				break;					
		}
		cal++;
	}
	// FillIQK Result
	switch (Path){
		case ODM_RF_PATH_A:
			{
				ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("========Path_A =======\n"));
		if (TX_Average == 0){
			_IQK_TX_FillIQC_8812A(pDM_Odm, Path, 0x200, 0x0);
					break;
		}
				for (i = 0; i < TX_Average; i++){
					ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("TX_X0[%d] = %x ;; TX_Y0[%d] = %x\n", i, (TX_X0[i])>>21&0x000007ff, i, (TX_Y0[i])>>21&0x000007ff));
		}
		for (i = 0; i < TX_Average; i++){
			for (ii = i+1; ii <TX_Average; ii++){
				dx = (TX_X0[i]>>21) - (TX_X0[ii]>>21);
				if (dx < 4 && dx > -4){
					dy = (TX_Y0[i]>>21) - (TX_Y0[ii]>>21);
						if (dy < 4 && dy > -4){
							TX_X = ((TX_X0[i]>>21) + (TX_X0[ii]>>21))/2;
							TX_Y = ((TX_Y0[i]>>21) + (TX_Y0[ii]>>21))/2;
							if (*pDM_Odm->pBandWidth == 2){
								Tx_dt[0] = (Tx_dt[i] + Tx_dt[ii])/2;
							}
							TX_finish = 1;
							break;
						}
				}
			}
			if (TX_finish == 1)
				break;
		}	
		if (*pDM_Odm->pBandWidth == 2){
			ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 0 --> Page C
			ODM_SetBBReg(pDM_Odm, 0xce8, 0x3fff0000, Tx_dt[0] & 0x00003fff);
			ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
		}
		if (TX_finish == 1){
				_IQK_TX_FillIQC_8812A(pDM_Odm, Path, TX_X, TX_Y);
		}
		else{
			_IQK_TX_FillIQC_8812A(pDM_Odm, Path, 0x200, 0x0);
		}
		if (RX_Average == 0){
			_IQK_RX_FillIQC_8812A(pDM_Odm, Path, 0x200, 0x0);
			break;
		}

				for (i = 0; i < RX_Average; i++){
			ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RX_X0[%d] = %x ;; RX_Y0[%d] = %x\n", i, (RX_X0[i])>>21&0x000007ff, i, (RX_Y0[i])>>21&0x000007ff));
		}
		for (i = 0; i < RX_Average; i++){
			for (ii = i+1; ii <RX_Average; ii++){
				dx = (RX_X0[i]>>21) - (RX_X0[ii]>>21);
				if (dx < 4 && dx > -4){
					dy = (RX_Y0[i]>>21) - (RX_Y0[ii]>>21);
						if (dy < 4 && dy > -4){
							RX_X = ((RX_X0[i]>>21) + (RX_X0[ii]>>21))/2;
							RX_Y = ((RX_Y0[i]>>21) + (RX_Y0[ii]>>21))/2;
							if (*pDM_Odm->pBandWidth == 2){
								Rx_dt[0] = (Rx_dt[i] + Rx_dt[ii])/2;
							}
							RX_finish = 1;
							break;
						}
				}
			}
			if (RX_finish == 1)
				break;
		}
		if (*pDM_Odm->pBandWidth == 2){
			ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 0 --> Page C
			ODM_SetBBReg(pDM_Odm, 0xce8, 0x00003fff, Rx_dt[0] & 0x00003fff);
			ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
		}
		if (RX_finish == 1){
				_IQK_RX_FillIQC_8812A(pDM_Odm, Path, RX_X, RX_Y);
			}
		else{
			_IQK_RX_FillIQC_8812A(pDM_Odm, Path, 0x200, 0x0);
		}
		if (TX_finish && RX_finish){
			pRFCalibrateInfo->bNeedIQK = FALSE;
			pRFCalibrateInfo->IQKMatrixRegSetting[chnlIdx].Value[*pDM_Odm->pBandWidth][0] = ((TX_X & 0x000007ff) << 16) + (TX_Y & 0x000007ff); 	//Path A TX
			pRFCalibrateInfo->IQKMatrixRegSetting[chnlIdx].Value[*pDM_Odm->pBandWidth][1] = ((RX_X & 0x000007ff) << 16) + (RX_Y & 0x000007ff); 	//Path A RX
			
			if (*pDM_Odm->pBandWidth == 2){
				ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 0 --> Page C
				pRFCalibrateInfo->IQKMatrixRegSetting[chnlIdx].Value[*pDM_Odm->pBandWidth][4] = ODM_Read4Byte( pDM_Odm, 0xce8);	//Path B VDF
				ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
			}	
		}
		
	}
			break;
		case ODM_RF_PATH_B:
			{
				ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("========Path_B =======\n"));
		if (TX_Average == 0){
			_IQK_TX_FillIQC_8812A(pDM_Odm, Path, 0x200, 0x0);
		    	break;
		}

				for (i = 0; i < TX_Average; i++){
					ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("TX_X1[%d] = %x ;; TX_Y1[%d] = %x\n", i, (TX_X1[i])>>21&0x000007ff, i, (TX_Y1[i])>>21&0x000007ff));
			}
		for (i = 0; i < TX_Average; i++){
			for (ii = i+1; ii <TX_Average; ii++){
				dx = (TX_X1[i]>>21) - (TX_X1[ii]>>21);
				if (dx < 4 && dx > -4){
					dy = (TX_Y1[i]>>21) - (TX_Y1[ii]>>21);
						if (dy < 4 && dy > -4){
							TX_X = ((TX_X1[i]>>21) + (TX_X1[ii]>>21))/2;
							TX_Y = ((TX_Y1[i]>>21) + (TX_Y1[ii]>>21))/2;
							if (*pDM_Odm->pBandWidth == 2){
								Tx_dt[0] = (Tx_dt[i] + Tx_dt[ii])/2;
							}
							TX_finish = 1;
							break;
						}
				}
			}
			if (TX_finish == 1)
				break;
		}	
		if (*pDM_Odm->pBandWidth == 2){
			ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 0 --> Page C
			ODM_SetBBReg(pDM_Odm, 0xee8, 0x3fff0000, Tx_dt[0] & 0x00003fff);
			ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
		}
		if (TX_finish == 1){
		_IQK_TX_FillIQC_8812A(pDM_Odm, Path, TX_X, TX_Y);
		}
		else{
			_IQK_TX_FillIQC_8812A(pDM_Odm, Path, 0x200, 0x0);
		}
		if (RX_Average == 0){
			_IQK_RX_FillIQC_8812A(pDM_Odm, Path, 0x200, 0x0);
			break;
		}
			
                for (i = 0; i < RX_Average; i++){
			ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RX_X1[%d] = %x ;; RX_Y1[%d] = %x\n", i, (RX_X1[i])>>21&0x000007ff, i, (RX_Y1[i])>>21&0x000007ff));
		}
		for (i = 0; i < RX_Average; i++){
			for (ii = i+1; ii <RX_Average; ii++){
				dx = (RX_X1[i]>>21) - (RX_X1[ii]>>21);
				if (dx < 4 && dx > -4){
					dy = (RX_Y1[i]>>21) - (RX_Y1[ii]>>21);
						if (dy < 4 && dy > -4){
							RX_X = ((RX_X1[i]>>21) + (RX_X1[ii]>>21))/2;
							RX_Y = ((RX_Y1[i]>>21) + (RX_Y1[ii]>>21))/2;
							if (*pDM_Odm->pBandWidth == 2){
								Rx_dt[0] = (Rx_dt[i] + Rx_dt[ii])/2;
							}
							RX_finish = 1;
							break;
                }
				}
			}
			if (RX_finish == 1)
				break;
		}	
		if (*pDM_Odm->pBandWidth == 2){
			ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 0 --> Page C
			ODM_SetBBReg(pDM_Odm, 0xee8, 0x00003fff, Rx_dt[0] & 0x00003fff);
			ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
		}
		if (RX_finish == 1){
                _IQK_RX_FillIQC_8812A(pDM_Odm, Path, RX_X, RX_Y);
            }
		else{
			_IQK_RX_FillIQC_8812A(pDM_Odm, Path, 0x200, 0x0);
		}
		if (TX_finish && RX_finish){
			pRFCalibrateInfo->bNeedIQK = FALSE;
			pRFCalibrateInfo->IQKMatrixRegSetting[chnlIdx].Value[*pDM_Odm->pBandWidth][2] = ((TX_X & 0x000007ff) << 16) + (TX_Y & 0x000007ff); 	//Path B TX
			pRFCalibrateInfo->IQKMatrixRegSetting[chnlIdx].Value[*pDM_Odm->pBandWidth][3] = ((RX_X & 0x000007ff) << 16) + (RX_Y & 0x000007ff); 	//Path B RX
			
			if (*pDM_Odm->pBandWidth == 2){
				ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 0 --> Page C
				pRFCalibrateInfo->IQKMatrixRegSetting[chnlIdx].Value[*pDM_Odm->pBandWidth][5] = ODM_Read4Byte( pDM_Odm, 0xee8);	//Path B VDF
				ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
			}	
		}
		
	}
			break;
		default:
			break;
	}

	if (!TX_finish && !RX_finish)
		priv->pshare->IQK_fail_cnt++;
}

#define MACBB_REG_NUM 10
#define AFE_REG_NUM 14
#define RF_REG_NUM 3
/*
IQK v1.0 update TXIQK high power parameter 
*/
VOID	
phy_IQCalibrate_8812A(
	IN PDM_ODM_T		pDM_Odm,
	IN u1Byte		Channel
		)
{
	u4Byte	MACBB_backup[MACBB_REG_NUM], AFE_backup[AFE_REG_NUM], RFA_backup[RF_REG_NUM], RFB_backup[RF_REG_NUM];
	u4Byte 	Backup_MACBB_REG[MACBB_REG_NUM] = {0x520, 0x550, 0x808, 0x838, 0x90c, 0xb00, 0xc00, 0xe00, 0x8c4, 0x82c}; 
	u4Byte 	Backup_AFE_REG[AFE_REG_NUM] = {0xc5c, 0xc60, 0xc64, 0xc68, 0xcb8, 0xcb0, 0xcb4,
		       	                                                   0xe5c, 0xe60, 0xe64, 0xe68, 0xeb8, 0xeb0, 0xeb4}; 
	u4Byte 	Backup_RF_REG[RF_REG_NUM] = {0x65, 0x8f, 0x0}; 
	u1Byte 	chnlIdx = ODM_GetRightChnlPlaceforIQK(Channel);

	_IQK_BackupMacBB_8812A(pDM_Odm, MACBB_backup, Backup_MACBB_REG, MACBB_REG_NUM);
	_IQK_BackupAFE_8812A(pDM_Odm, AFE_backup, Backup_AFE_REG, AFE_REG_NUM);
	_IQK_BackupRF_8812A(pDM_Odm, RFA_backup, RFB_backup, Backup_RF_REG, RF_REG_NUM);

	_IQK_ConfigureMAC_8812A(pDM_Odm);
	_IQK_Tx_8812A(pDM_Odm, ODM_RF_PATH_A, chnlIdx);
	_IQK_RestoreRF_8812A(pDM_Odm, ODM_RF_PATH_A, Backup_RF_REG, RFA_backup, RF_REG_NUM);

	_IQK_Tx_8812A(pDM_Odm, ODM_RF_PATH_B, chnlIdx);
	_IQK_RestoreRF_8812A(pDM_Odm, ODM_RF_PATH_B, Backup_RF_REG, RFB_backup, RF_REG_NUM);

	_IQK_RestoreAFE_8812A(pDM_Odm, AFE_backup, Backup_AFE_REG, AFE_REG_NUM);
	_IQK_RestoreMacBB_8812A(pDM_Odm, MACBB_backup, Backup_MACBB_REG, MACBB_REG_NUM);

	//_IQK_Exit_8812A(pDM_Odm);
	//_IQK_TX_CheckResult_8812A

}


VOID	
	phy_LCCalibrate_8812A(
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
			IN PDM_ODM_T		pDM_Odm,
#else
			IN	PADAPTER	pAdapter,
#endif
			IN	BOOLEAN		is2T
			)
{
	u1Byte	tmpReg;
	u4Byte	RF_Amode=0, RF_Bmode=0, LC_Cal;
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
#endif
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;
#endif
#endif	
	//Check continuous TX and Packet TX
	tmpReg = ODM_Read1Byte(pDM_Odm, 0xd03);

	if((tmpReg&0x70) != 0)			//Deal with contisuous TX case
		ODM_Write1Byte(pDM_Odm, 0xd03, tmpReg&0x8F);	//disable all continuous TX
	else							// Deal with Packet TX case
		ODM_Write1Byte(pDM_Odm, REG_TXPAUSE, 0xFF);			// block all queues

	if((tmpReg&0x70) != 0)
	{
		//1. Read original RF mode
		//Path-A
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
		RF_Amode = PHY_QueryRFReg(pAdapter, RF_PATH_A, RF_AC, bMask12Bits);

		//Path-B
		if(is2T)
			RF_Bmode = PHY_QueryRFReg(pAdapter, RF_PATH_B, RF_AC, bMask12Bits);	
#else
		RF_Amode = ODM_GetRFReg(pDM_Odm, RF_PATH_A, RF_AC, bMask12Bits);

		//Path-B
		if(is2T)
			RF_Bmode = ODM_GetRFReg(pDM_Odm, RF_PATH_B, RF_AC, bMask12Bits);	
#endif	

		//2. Set RF mode = standby mode
		//Path-A
		ODM_SetRFReg(pDM_Odm, RF_PATH_A, RF_AC, bMask12Bits, (RF_Amode&0x8FFFF)|0x10000);

		//Path-B
		if(is2T)
			ODM_SetRFReg(pDM_Odm, RF_PATH_B, RF_AC, bMask12Bits, (RF_Bmode&0x8FFFF)|0x10000);			
	}

	//3. Read RF reg18
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	LC_Cal = PHY_QueryRFReg(pAdapter, RF_PATH_A, RF_CHNLBW, bMask12Bits);
#else
	LC_Cal = ODM_GetRFReg(pDM_Odm, RF_PATH_A, RF_CHNLBW, bMask12Bits);
#endif	

	//4. Set LC calibration begin	bit15
	ODM_SetRFReg(pDM_Odm, RF_PATH_A, RF_CHNLBW, bMask12Bits, LC_Cal|0x08000);

	ODM_delay_ms(100);		


	//Restore original situation
	if((tmpReg&0x70) != 0)	//Deal with contisuous TX case 
	{  
		//Path-A
		ODM_Write1Byte(pDM_Odm, 0xd03, tmpReg);
		ODM_SetRFReg(pDM_Odm, RF_PATH_A, RF_AC, bMask12Bits, RF_Amode);

		//Path-B
		if(is2T)
			ODM_SetRFReg(pDM_Odm, RF_PATH_B, RF_AC, bMask12Bits, RF_Bmode);
	}
	else // Deal with Packet TX case
	{
		ODM_Write1Byte(pDM_Odm, REG_TXPAUSE, 0x00);	
	}
}

//Analog Pre-distortion calibration
#define		APK_BB_REG_NUM	8
#define		APK_CURVE_REG_NUM 4
#define		PATH_NUM		2

VOID	
	phy_APCalibrate_8812A(
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
			IN PDM_ODM_T		pDM_Odm,
#else
			IN	PADAPTER	pAdapter,
#endif
			IN	s1Byte 		delta,
			IN	BOOLEAN		is2T
			)
{
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
#endif
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;
#endif
#endif
	u4Byte 			regD[PATH_NUM];
	u4Byte			tmpReg, index, offset,  apkbound;
	u1Byte			path, i, pathbound = PATH_NUM;		
	u4Byte			BB_backup[APK_BB_REG_NUM];
	u4Byte			BB_REG[APK_BB_REG_NUM] = {	
		rFPGA1_TxBlock, 	rOFDM0_TRxPathEnable, 
		rFPGA0_RFMOD, 	rOFDM0_TRMuxPar, 
		rFPGA0_XCD_RFInterfaceSW,	rFPGA0_XAB_RFInterfaceSW, 
		rFPGA0_XA_RFInterfaceOE, 	rFPGA0_XB_RFInterfaceOE	};
	u4Byte			BB_AP_MODE[APK_BB_REG_NUM] = {	
		0x00000020, 0x00a05430, 0x02040000, 
		0x000800e4, 0x00204000 };
	u4Byte			BB_normal_AP_MODE[APK_BB_REG_NUM] = {	
		0x00000020, 0x00a05430, 0x02040000, 
		0x000800e4, 0x22204000 };						

	u4Byte			AFE_backup[IQK_ADDA_REG_NUM];
	u4Byte			AFE_REG[IQK_ADDA_REG_NUM] = {	
		rFPGA0_XCD_SwitchControl, 	rBlue_Tooth, 	
		rRx_Wait_CCA, 		rTx_CCK_RFON,
		rTx_CCK_BBON, 	rTx_OFDM_RFON, 	
		rTx_OFDM_BBON, 	rTx_To_Rx,
		rTx_To_Tx, 		rRx_CCK, 	
		rRx_OFDM, 		rRx_Wait_RIFS,
		rRx_TO_Rx, 		rStandby, 	
		rSleep, 			rPMPD_ANAEN };

	u4Byte			MAC_backup[IQK_MAC_REG_NUM];
	u4Byte			MAC_REG[IQK_MAC_REG_NUM] = {
		REG_TXPAUSE, 		REG_BCN_CTRL,	
		REG_BCN_CTRL_1,	REG_GPIO_MUXCFG};

	u4Byte			APK_RF_init_value[PATH_NUM][APK_BB_REG_NUM] = {
		{0x0852c, 0x1852c, 0x5852c, 0x1852c, 0x5852c},
		{0x2852e, 0x0852e, 0x3852e, 0x0852e, 0x0852e}
	};	

	u4Byte			APK_normal_RF_init_value[PATH_NUM][APK_BB_REG_NUM] = {
		{0x0852c, 0x0a52c, 0x3a52c, 0x5a52c, 0x5a52c},	//path settings equal to path b settings
		{0x0852c, 0x0a52c, 0x5a52c, 0x5a52c, 0x5a52c}
	};

	u4Byte			APK_RF_value_0[PATH_NUM][APK_BB_REG_NUM] = {
		{0x52019, 0x52014, 0x52013, 0x5200f, 0x5208d},
		{0x5201a, 0x52019, 0x52016, 0x52033, 0x52050}
	};

	u4Byte			APK_normal_RF_value_0[PATH_NUM][APK_BB_REG_NUM] = {
		{0x52019, 0x52017, 0x52010, 0x5200d, 0x5206a},	//path settings equal to path b settings
		{0x52019, 0x52017, 0x52010, 0x5200d, 0x5206a}
	};

	u4Byte			AFE_on_off[PATH_NUM] = {
		0x04db25a4, 0x0b1b25a4};	//path A on path B off / path A off path B on

	u4Byte			APK_offset[PATH_NUM] = {
		rConfig_AntA, rConfig_AntB};

	u4Byte			APK_normal_offset[PATH_NUM] = {
		rConfig_Pmpd_AntA, rConfig_Pmpd_AntB};

	u4Byte			APK_value[PATH_NUM] = {
		0x92fc0000, 0x12fc0000};					

	u4Byte			APK_normal_value[PATH_NUM] = {
		0x92680000, 0x12680000};					

	s1Byte			APK_delta_mapping[APK_BB_REG_NUM][13] = {
		{-4, -3, -2, -2, -1, -1, 0, 1, 2, 3, 4, 5, 6},
		{-4, -3, -2, -2, -1, -1, 0, 1, 2, 3, 4, 5, 6},											
		{-6, -4, -2, -2, -1, -1, 0, 1, 2, 3, 4, 5, 6},
		{-1, -1, -1, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6},
		{-11, -9, -7, -5, -3, -1, 0, 0, 0, 0, 0, 0, 0}
	};

	u4Byte			APK_normal_setting_value_1[13] = {
		0x01017018, 0xf7ed8f84, 0x1b1a1816, 0x2522201e, 0x322e2b28,
		0x433f3a36, 0x5b544e49, 0x7b726a62, 0xa69a8f84, 0xdfcfc0b3,
		0x12680000, 0x00880000, 0x00880000
	};

	u4Byte			APK_normal_setting_value_2[16] = {
		0x01c7021d, 0x01670183, 0x01000123, 0x00bf00e2, 0x008d00a3,
		0x0068007b, 0x004d0059, 0x003a0042, 0x002b0031, 0x001f0025,
		0x0017001b, 0x00110014, 0x000c000f, 0x0009000b, 0x00070008,
		0x00050006
	};

	u4Byte			APK_result[PATH_NUM][APK_BB_REG_NUM];	//val_1_1a, val_1_2a, val_2a, val_3a, val_4a
	//	u4Byte			AP_curve[PATH_NUM][APK_CURVE_REG_NUM];

	s4Byte			BB_offset, delta_V, delta_offset;

#if defined(MP_DRIVER) && (MP_DRIVER == 1)
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PMPT_CONTEXT	pMptCtx = &(pAdapter->mppriv.MptCtx);	
#else
	PMPT_CONTEXT	pMptCtx = &(pAdapter->MptCtx);	
#endif
	pMptCtx->APK_bound[0] = 45;
	pMptCtx->APK_bound[1] = 52;		

#endif

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("==>phy_APCalibrate_8812A() delta %d\n", delta));
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("AP Calibration for %s\n", (is2T ? "2T2R" : "1T1R")));
	if(!is2T)
		pathbound = 1;

	//2 FOR NORMAL CHIP SETTINGS

	// Temporarily do not allow normal driver to do the following settings because these offset
	// and value will cause RF internal PA to be unpredictably disabled by HW, such that RF Tx signal
	// will disappear after disable/enable card many times on 88CU. RF SD and DD have not find the
	// root cause, so we remove these actions temporarily. Added by tynli and SD3 Allen. 2010.05.31.
#if !defined(MP_DRIVER) || (MP_DRIVER != 1)
	return;
#endif
	//settings adjust for normal chip
	for(index = 0; index < PATH_NUM; index ++)
	{
		APK_offset[index] = APK_normal_offset[index];
		APK_value[index] = APK_normal_value[index];
		AFE_on_off[index] = 0x6fdb25a4;
	}

	for(index = 0; index < APK_BB_REG_NUM; index ++)
	{
		for(path = 0; path < pathbound; path++)
		{
			APK_RF_init_value[path][index] = APK_normal_RF_init_value[path][index];
			APK_RF_value_0[path][index] = APK_normal_RF_value_0[path][index];
		}
		BB_AP_MODE[index] = BB_normal_AP_MODE[index];
	}			

	apkbound = 6;

	//save BB default value
	for(index = 0; index < APK_BB_REG_NUM ; index++)
	{
		if(index == 0)		//skip 
			continue;				
		BB_backup[index] = ODM_GetBBReg(pDM_Odm, BB_REG[index], bMaskDWord);
	}

	//save MAC default value													
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	_PHY_SaveMACRegisters_8812A(pAdapter, MAC_REG, MAC_backup);

	//save AFE default value
	_PHY_SaveADDARegisters_8812A(pAdapter, AFE_REG, AFE_backup, IQK_ADDA_REG_NUM);
#else
	_PHY_SaveMACRegisters_8812A(pDM_Odm, MAC_REG, MAC_backup);

	//save AFE default value
	_PHY_SaveADDARegisters_8812A(pDM_Odm, AFE_REG, AFE_backup, IQK_ADDA_REG_NUM);
#endif

	for(path = 0; path < pathbound; path++)
	{


		if(path == RF_PATH_A)
		{
			//path A APK
			//load APK setting
			//path-A		
			offset = rPdp_AntA;
			for(index = 0; index < 11; index ++)			
			{
				ODM_SetBBReg(pDM_Odm, offset, bMaskDWord, APK_normal_setting_value_1[index]);
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("phy_APCalibrate_8812A() offset 0x%x value 0x%x\n", offset, ODM_GetBBReg(pDM_Odm, offset, bMaskDWord))); 	

				offset += 0x04;
			}

			ODM_SetBBReg(pDM_Odm, rConfig_Pmpd_AntB, bMaskDWord, 0x12680000);

			offset = rConfig_AntA;
			for(; index < 13; index ++) 		
			{
				ODM_SetBBReg(pDM_Odm, offset, bMaskDWord, APK_normal_setting_value_1[index]);
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("phy_APCalibrate_8812A() offset 0x%x value 0x%x\n", offset, ODM_GetBBReg(pDM_Odm, offset, bMaskDWord))); 	

				offset += 0x04;
			}	

			//page-B1
			ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, 0xffffff00, 0x400000);

			//path A
			offset = rPdp_AntA;
			for(index = 0; index < 16; index++)
			{
				ODM_SetBBReg(pDM_Odm, offset, bMaskDWord, APK_normal_setting_value_2[index]);		
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("phy_APCalibrate_8812A() offset 0x%x value 0x%x\n", offset, ODM_GetBBReg(pDM_Odm, offset, bMaskDWord))); 	

				offset += 0x04;
			}				
			ODM_SetBBReg(pDM_Odm,  rFPGA0_IQK, 0xffffff00, 0);							
		}
		else if(path == RF_PATH_B)
		{
			//path B APK
			//load APK setting
			//path-B		
			offset = rPdp_AntB;
			for(index = 0; index < 10; index ++)			
			{
				ODM_SetBBReg(pDM_Odm, offset, bMaskDWord, APK_normal_setting_value_1[index]);
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("phy_APCalibrate_8812A() offset 0x%x value 0x%x\n", offset, ODM_GetBBReg(pDM_Odm, offset, bMaskDWord))); 	

				offset += 0x04;
			}
			ODM_SetBBReg(pDM_Odm, rConfig_Pmpd_AntA, bMaskDWord, 0x12680000);			
			ODM_SetBBReg(pDM_Odm, rConfig_Pmpd_AntB, bMaskDWord, 0x12680000);

			offset = rConfig_AntA;
			index = 11;
			for(; index < 13; index ++) //offset 0xb68, 0xb6c		
			{
				ODM_SetBBReg(pDM_Odm, offset, bMaskDWord, APK_normal_setting_value_1[index]);
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("phy_APCalibrate_8812A() offset 0x%x value 0x%x\n", offset, ODM_GetBBReg(pDM_Odm, offset, bMaskDWord))); 	

				offset += 0x04;
			}	

			//page-B1
			ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, 0xffffff00, 0x400000);

			//path B
			offset = 0xb60;
			for(index = 0; index < 16; index++)
			{
				ODM_SetBBReg(pDM_Odm, offset, bMaskDWord, APK_normal_setting_value_2[index]);		
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("phy_APCalibrate_8812A() offset 0x%x value 0x%x\n", offset, ODM_GetBBReg(pDM_Odm, offset, bMaskDWord))); 	

				offset += 0x04;
			}				
			ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, 0xffffff00, 0);							
		}

		//save RF default value
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
		regD[path] = PHY_QueryRFReg(pAdapter, path, RF_TXBIAS_A, bMaskDWord);
#else
		regD[path] = ODM_GetRFReg(pDM_Odm, path, RF_TXBIAS_A, bMaskDWord);
#endif

		//Path A AFE all on, path B AFE All off or vise versa
		for(index = 0; index < IQK_ADDA_REG_NUM ; index++)
			ODM_SetBBReg(pDM_Odm, AFE_REG[index], bMaskDWord, AFE_on_off[path]);
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("phy_APCalibrate_8812A() offset 0xe70 %x\n", ODM_GetBBReg(pDM_Odm, rRx_Wait_CCA, bMaskDWord)));		

		//BB to AP mode
		if(path == 0)
		{				
			for(index = 0; index < APK_BB_REG_NUM ; index++)
			{

				if(index == 0)		//skip 
					continue;			
				else if (index < 5)
					ODM_SetBBReg(pDM_Odm, BB_REG[index], bMaskDWord, BB_AP_MODE[index]);
				else if (BB_REG[index] == 0x870)
					ODM_SetBBReg(pDM_Odm, BB_REG[index], bMaskDWord, BB_backup[index]|BIT10|BIT26);
				else
					ODM_SetBBReg(pDM_Odm, BB_REG[index], BIT10, 0x0);					
			}

			ODM_SetBBReg(pDM_Odm, rTx_IQK_Tone_A, bMaskDWord, 0x01008c00);			
			ODM_SetBBReg(pDM_Odm, rRx_IQK_Tone_A, bMaskDWord, 0x01008c00);					
		}
		else		//path B
		{
			ODM_SetBBReg(pDM_Odm, rTx_IQK_Tone_B, bMaskDWord, 0x01008c00);			
			ODM_SetBBReg(pDM_Odm, rRx_IQK_Tone_B, bMaskDWord, 0x01008c00);					

		}

		ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("phy_APCalibrate_8812A() offset 0x800 %x\n", ODM_GetBBReg(pDM_Odm, 0x800, bMaskDWord)));				

		//MAC settings
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
		_PHY_MACSettingCalibration_8812A(pAdapter, MAC_REG, MAC_backup);
#else
		_PHY_MACSettingCalibration_8812A(pDM_Odm, MAC_REG, MAC_backup);
#endif

		if(path == RF_PATH_A)	//Path B to standby mode
		{
			ODM_SetRFReg(pDM_Odm, RF_PATH_B, RF_AC, bMaskDWord, 0x10000);			
		}
		else			//Path A to standby mode
		{
			ODM_SetRFReg(pDM_Odm, RF_PATH_A, RF_AC, bMaskDWord, 0x10000);			
			ODM_SetRFReg(pDM_Odm, RF_PATH_A, RF_MODE1, bMaskDWord, 0x1000f);			
			ODM_SetRFReg(pDM_Odm, RF_PATH_A, RF_MODE2, bMaskDWord, 0x20103);						
		}

		delta_offset = ((delta+14)/2);
		if(delta_offset < 0)
			delta_offset = 0;
		else if (delta_offset > 12)
			delta_offset = 12;

		//AP calibration
		for(index = 0; index < APK_BB_REG_NUM; index++)
		{
			if(index != 1)	//only DO PA11+PAD01001, AP RF setting
				continue;

			tmpReg = APK_RF_init_value[path][index];
#if 1			
			if(!pDM_Odm->RFCalibrateInfo.bAPKThermalMeterIgnore)
			{
				BB_offset = (tmpReg & 0xF0000) >> 16;

				if(!(tmpReg & BIT15)) //sign bit 0
				{
					BB_offset = -BB_offset;
				}

				delta_V = APK_delta_mapping[index][delta_offset];

				BB_offset += delta_V;

				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("phy_APCalibrate_8812A() APK index %d tmpReg 0x%x delta_V %d delta_offset %d\n", index, tmpReg, (int)delta_V, (int)delta_offset));

				if(BB_offset < 0)
				{
					tmpReg = tmpReg & (~BIT15);
					BB_offset = -BB_offset;
				}
				else
				{
					tmpReg = tmpReg | BIT15;
				}
				tmpReg = (tmpReg & 0xFFF0FFFF) | (BB_offset << 16);
			}
#endif

			ODM_SetRFReg(pDM_Odm, path, RF_IPA_A, bMaskDWord, 0x8992e);
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("phy_APCalibrate_8812A() offset 0xc %x\n", PHY_QueryRFReg(pAdapter, path, RF_IPA_A, bMaskDWord)));		
			ODM_SetRFReg(pDM_Odm, path, RF_AC, bMaskDWord, APK_RF_value_0[path][index]);
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("phy_APCalibrate_8812A() offset 0x0 %x\n", PHY_QueryRFReg(pAdapter, path, RF_AC, bMaskDWord)));		
			ODM_SetRFReg(pDM_Odm, path, RF_TXBIAS_A, bMaskDWord, tmpReg);
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("phy_APCalibrate_8812A() offset 0xd %x\n", PHY_QueryRFReg(pAdapter, path, RF_TXBIAS_A, bMaskDWord)));					
#else
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("phy_APCalibrate_8812A() offset 0xc %x\n", ODM_GetRFReg(pDM_Odm, path, RF_IPA_A, bMaskDWord)));		
			ODM_SetRFReg(pDM_Odm, path, RF_AC, bMaskDWord, APK_RF_value_0[path][index]);
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("phy_APCalibrate_8812A() offset 0x0 %x\n", ODM_GetRFReg(pDM_Odm, path, RF_AC, bMaskDWord)));		
			ODM_SetRFReg(pDM_Odm, path, RF_TXBIAS_A, bMaskDWord, tmpReg);
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("phy_APCalibrate_8812A() offset 0xd %x\n", ODM_GetRFReg(pDM_Odm, path, RF_TXBIAS_A, bMaskDWord)));					
#endif

			// PA11+PAD01111, one shot	
			i = 0;
			do
			{
				ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, 0xffffff00, 0x800000);
				{
					ODM_SetBBReg(pDM_Odm, APK_offset[path], bMaskDWord, APK_value[0]);		
					ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("phy_APCalibrate_8812A() offset 0x%x value 0x%x\n", APK_offset[path], ODM_GetBBReg(pDM_Odm, APK_offset[path], bMaskDWord)));
					ODM_delay_ms(3);				
					ODM_SetBBReg(pDM_Odm, APK_offset[path], bMaskDWord, APK_value[1]);
					ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("phy_APCalibrate_8812A() offset 0x%x value 0x%x\n", APK_offset[path], ODM_GetBBReg(pDM_Odm, APK_offset[path], bMaskDWord)));

					ODM_delay_ms(20);
				}
				ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, 0xffffff00, 0);

				if(path == RF_PATH_A)
					tmpReg = ODM_GetBBReg(pDM_Odm, rAPK, 0x03E00000);
				else
					tmpReg = ODM_GetBBReg(pDM_Odm, rAPK, 0xF8000000);
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("phy_APCalibrate_8812A() offset 0xbd8[25:21] %x\n", tmpReg));		


				i++;
			}
			while(tmpReg > apkbound && i < 4);

			APK_result[path][index] = tmpReg;
		}
	}

	//reload MAC default value	
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	_PHY_ReloadMACRegisters_8812A(pAdapter, MAC_REG, MAC_backup);
#else
	_PHY_ReloadMACRegisters_8812A(pDM_Odm, MAC_REG, MAC_backup);
#endif

	//reload BB default value	
	for(index = 0; index < APK_BB_REG_NUM ; index++)
	{

		if(index == 0)		//skip 
			continue;					
		ODM_SetBBReg(pDM_Odm, BB_REG[index], bMaskDWord, BB_backup[index]);
	}

	//reload AFE default value
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	_PHY_ReloadADDARegisters_8812A(pAdapter, AFE_REG, AFE_backup, IQK_ADDA_REG_NUM);
#else
	_PHY_ReloadADDARegisters_8812A(pDM_Odm, AFE_REG, AFE_backup, IQK_ADDA_REG_NUM);
#endif

	//reload RF path default value
	for(path = 0; path < pathbound; path++)
	{
		ODM_SetRFReg(pDM_Odm, path, 0xd, bMaskDWord, regD[path]);
		if(path == RF_PATH_B)
		{
			ODM_SetRFReg(pDM_Odm, RF_PATH_A, RF_MODE1, bMaskDWord, 0x1000f);			
			ODM_SetRFReg(pDM_Odm, RF_PATH_A, RF_MODE2, bMaskDWord, 0x20101);						
		}

		//note no index == 0
		if (APK_result[path][1] > 6)
			APK_result[path][1] = 6;
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("apk path %d result %d 0x%x \t", path, 1, APK_result[path][1]));					
	}

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("\n"));


	for(path = 0; path < pathbound; path++)
	{
		ODM_SetRFReg(pDM_Odm, path, 0x3, bMaskDWord, 
				((APK_result[path][1] << 15) | (APK_result[path][1] << 10) | (APK_result[path][1] << 5) | APK_result[path][1]));
		if(path == RF_PATH_A)
			ODM_SetRFReg(pDM_Odm, path, 0x4, bMaskDWord, 
					((APK_result[path][1] << 15) | (APK_result[path][1] << 10) | (0x00 << 5) | 0x05));		
		else
			ODM_SetRFReg(pDM_Odm, path, 0x4, bMaskDWord, 
					((APK_result[path][1] << 15) | (APK_result[path][1] << 10) | (0x02 << 5) | 0x05));							
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
			ODM_SetRFReg(pDM_Odm, path, RF_BS_PA_APSET_G9_G11, bMaskDWord, 
					((0x08 << 15) | (0x08 << 10) | (0x08 << 5) | 0x08));			
#endif		
	}

	pDM_Odm->RFCalibrateInfo.bAPKdone = TRUE;

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("<==phy_APCalibrate_8812A()\n"));
}



#define		DP_BB_REG_NUM		7
#define		DP_RF_REG_NUM		1
#define		DP_RETRY_LIMIT		10
#define		DP_PATH_NUM		2
#define		DP_DPK_NUM			3
#define		DP_DPK_VALUE_NUM	2


#if 0 //FOR_8812_IQK
VOID
PHY_IQCalibrate_8812A(
		IN	PADAPTER	pAdapter,
		IN	BOOLEAN 	bReCovery
		)
{

#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);	

#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;	
#else  // (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;	
#endif
#endif	
#if (MP_DRIVER == 1)
	#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)	
	PMPT_CONTEXT	pMptCtx = &(pAdapter->MptCtx);	
	#else// (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PMPT_CONTEXT	pMptCtx = &(pAdapter->mppriv.MptCtx);		
	#endif	
#endif//(MP_DRIVER == 1)

#if (DM_ODM_SUPPORT_TYPE & (ODM_WIN|ODM_CE) )
	if (ODM_CheckPowerStatus(pAdapter) == FALSE)
		return;
#endif

#if MP_DRIVER == 1	
	if( ! (pMptCtx->bSingleTone || pMptCtx->bCarrierSuppression) )
#endif		
	phy_IQCalibrate_8812A(pDM_Odm);


}
#endif

VOID
PHY_LCCalibrate_8812A(
		IN PDM_ODM_T		pDM_Odm
		)
{
#if 0
	BOOLEAN 		bStartContTx = FALSE, bSingleTone = FALSE, bCarrierSuppression = FALSE;
	u4Byte			timeout = 2000, timecount = 0;

#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);	

#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;	
#else  // (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;	
#endif

#if (MP_DRIVER == 1)
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)	
	PMPT_CONTEXT	pMptCtx = &(pAdapter->MptCtx);	
#else// (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PMPT_CONTEXT	pMptCtx = &(pAdapter->mppriv.MptCtx);		
#endif	
#endif//(MP_DRIVER == 1)
#endif	




#if MP_DRIVER == 1	
	bStartContTx = pMptCtx->bStartContTx;
	bSingleTone = pMptCtx->bSingleTone;
	bCarrierSuppression = pMptCtx->bCarrierSuppression;	
#endif


#ifdef DISABLE_BB_RF
	return;
#endif

#if (DM_ODM_SUPPORT_TYPE & (ODM_CE|ODM_AP))
	if(!(pDM_Odm->SupportAbility & ODM_RF_CALIBRATION))
	{
		return;
	}
#endif	
	// 20120213<Kordan> Turn on when continuous Tx to pass lab testing. (required by Edlu)
	if(bSingleTone || bCarrierSuppression)
		return;

	while(*(pDM_Odm->pbScanInProcess) && timecount < timeout)
	{
		ODM_delay_ms(50);
		timecount += 50;
	}	

	pDM_Odm->RFCalibrateInfo.bLCKInProgress = TRUE;

	//ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("LCK:Start!!!interface %d currentband %x delay %d ms\n", pDM_Odm->interfaceIndex, pHalData->CurrentBandType92D, timecount));
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)

	if(IS_2T2R(pHalData->VersionID))
	{
		phy_LCCalibrate_8812A(pAdapter, TRUE);
	}
	else
#endif		
	{
		// For 88C 1T1R
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
		phy_LCCalibrate_8812A(pAdapter, FALSE);
#else
		phy_LCCalibrate_8812A(pDM_Odm, FALSE);
#endif		
	}

	pDM_Odm->RFCalibrateInfo.bLCKInProgress = FALSE;

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("LCK:Finish!!!interface %d\n", pDM_Odm->InterfaceIndex));
#endif
}

VOID
	PHY_APCalibrate_8812A(
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
			IN PDM_ODM_T		pDM_Odm,
#else
			IN	PADAPTER	pAdapter,
#endif
			IN	s1Byte 		delta	
			)
{
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
#endif
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;
#endif
#endif	
#ifdef DISABLE_BB_RF
	return;
#endif

	return;
#if (DM_ODM_SUPPORT_TYPE & (ODM_CE|ODM_AP))
	if(!(pDM_Odm->SupportAbility & ODM_RF_CALIBRATION))
	{
		return;
	}
#endif	

#if defined(FOR_BRAZIL_PRETEST) && (FOR_BRAZIL_PRETEST != 1)
	if(pDM_Odm->RFCalibrateInfo.bAPKdone)
#endif		
		return;

#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	if(IS_92C_SERIAL( pHalData->VersionID)){
		phy_APCalibrate_8812A(pAdapter, delta, TRUE);
	}
	else
#endif
	{
		// For 88C 1T1R
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
		phy_APCalibrate_8812A(pAdapter, delta, FALSE);
#else
		phy_APCalibrate_8812A(pDM_Odm, delta, FALSE);
#endif
	}
}
	VOID phy_SetRFPathSwitch_8812A(
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
			IN PDM_ODM_T		pDM_Odm,
#else
			IN	PADAPTER	pAdapter,
#endif
			IN	BOOLEAN		bMain,
			IN	BOOLEAN		is2T
			)
{
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
#elif (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;
#endif

#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	if(!pAdapter->bHWInitReady)	
#elif  (DM_ODM_SUPPORT_TYPE == ODM_CE)
		if(pAdapter->hw_init_completed == _FALSE)
#endif
		{
			u1Byte	u1bTmp;
			u1bTmp = ODM_Read1Byte(pDM_Odm, REG_LEDCFG2) | BIT7;
			ODM_Write1Byte(pDM_Odm, REG_LEDCFG2, u1bTmp);
			//ODM_SetBBReg(pDM_Odm, REG_LEDCFG0, BIT23, 0x01);
			ODM_SetBBReg(pDM_Odm, rFPGA0_XAB_RFParameter, BIT13, 0x01);
		}

#endif		

	if(is2T)	//92C
	{
		if(bMain)
			ODM_SetBBReg(pDM_Odm, rFPGA0_XB_RFInterfaceOE, BIT5|BIT6, 0x1);	//92C_Path_A			
		else
			ODM_SetBBReg(pDM_Odm, rFPGA0_XB_RFInterfaceOE, BIT5|BIT6, 0x2);	//BT							
	}
	else			//88C
	{

		if(bMain)
			ODM_SetBBReg(pDM_Odm, rFPGA0_XA_RFInterfaceOE, BIT8|BIT9, 0x2);	//Main
		else
			ODM_SetBBReg(pDM_Odm, rFPGA0_XA_RFInterfaceOE, BIT8|BIT9, 0x1);	//Aux		
	}	
}
	VOID PHY_SetRFPathSwitch_8812A(
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
			IN PDM_ODM_T		pDM_Odm,
#else
			IN	PADAPTER	pAdapter,
#endif
			IN	BOOLEAN		bMain
			)
{
	//HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);

#ifdef DISABLE_BB_RF
	return;
#endif

#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	if (IS_92C_SERIAL(pHalData->VersionID))
	{
		phy_SetRFPathSwitch_8812A(pAdapter, bMain, TRUE);
	}
	else
#endif		
	{
		// For 88C 1T1R
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
		phy_SetRFPathSwitch_8812A(pAdapter, bMain, FALSE);
#else
		phy_SetRFPathSwitch_8812A(pDM_Odm, bMain, FALSE);
#endif
	}
}

#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
//digital predistortion
VOID	
	phy_DigitalPredistortion_8812A(
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
			IN	PADAPTER	pAdapter,
#else
			IN PDM_ODM_T	pDM_Odm,
#endif
			IN	BOOLEAN		is2T
			)
{
#if (RT_PLATFORM == PLATFORM_WINDOWS)
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
#endif
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;
#endif
#endif	

	u4Byte			tmpReg, tmpReg2, index,  i;		
	u1Byte			path, pathbound = PATH_NUM;
	u4Byte			AFE_backup[IQK_ADDA_REG_NUM];
	u4Byte			AFE_REG[IQK_ADDA_REG_NUM] = {	
		rFPGA0_XCD_SwitchControl, 	rBlue_Tooth, 	
		rRx_Wait_CCA, 		rTx_CCK_RFON,
		rTx_CCK_BBON, 	rTx_OFDM_RFON, 	
		rTx_OFDM_BBON, 	rTx_To_Rx,
		rTx_To_Tx, 		rRx_CCK, 	
		rRx_OFDM, 		rRx_Wait_RIFS,
		rRx_TO_Rx, 		rStandby, 	
		rSleep, 			rPMPD_ANAEN };

	u4Byte			BB_backup[DP_BB_REG_NUM];	
	u4Byte			BB_REG[DP_BB_REG_NUM] = {
		rOFDM0_TRxPathEnable, rFPGA0_RFMOD, 
		rOFDM0_TRMuxPar, 	rFPGA0_XCD_RFInterfaceSW,
		rFPGA0_XAB_RFInterfaceSW, rFPGA0_XA_RFInterfaceOE, 
		rFPGA0_XB_RFInterfaceOE};						
	u4Byte			BB_settings[DP_BB_REG_NUM] = {
		0x00a05430, 0x02040000, 0x000800e4, 0x22208000, 
		0x0, 0x0, 0x0};	

	u4Byte			RF_backup[DP_PATH_NUM][DP_RF_REG_NUM];
	u4Byte			RF_REG[DP_RF_REG_NUM] = {
		RF_TXBIAS_A};

	u4Byte			MAC_backup[IQK_MAC_REG_NUM];
	u4Byte			MAC_REG[IQK_MAC_REG_NUM] = {
		REG_TXPAUSE, 		REG_BCN_CTRL,	
		REG_BCN_CTRL_1,	REG_GPIO_MUXCFG};

	u4Byte			Tx_AGC[DP_DPK_NUM][DP_DPK_VALUE_NUM] = {
		{0x1e1e1e1e, 0x03901e1e},
		{0x18181818, 0x03901818},
		{0x0e0e0e0e, 0x03900e0e}
	};

	u4Byte			AFE_on_off[PATH_NUM] = {
		0x04db25a4, 0x0b1b25a4};	//path A on path B off / path A off path B on

	u1Byte			RetryCount = 0;


	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("==>phy_DigitalPredistortion_8812A()\n"));

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("phy_DigitalPredistortion_8812A for %s %s\n", (is2T ? "2T2R" : "1T1R")));

	//save BB default value
	for(index=0; index<DP_BB_REG_NUM; index++)
		BB_backup[index] = ODM_GetBBReg(pDM_Odm, BB_REG[index], bMaskDWord);

	//save MAC default value
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	_PHY_SaveMACRegisters_8812A(pAdapter, BB_REG, MAC_backup);
#else
	_PHY_SaveMACRegisters_8812A(pDM_Odm, BB_REG, MAC_backup);
#endif	

	//save RF default value
	for(path=0; path<DP_PATH_NUM; path++)
	{
		for(index=0; index<DP_RF_REG_NUM; index++)
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
			RF_backup[path][index] = PHY_QueryRFReg(pAdapter, path, RF_REG[index], bMaskDWord);	
#else
		RF_backup[path][index] = ODM_GetRFReg(pAdapter, path, RF_REG[index], bMaskDWord);	
#endif	
	}	

	//save AFE default value
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	_PHY_SaveADDARegisters_8812A(pAdapter, AFE_REG, AFE_backup, IQK_ADDA_REG_NUM);
#else
	RF_backup[path][index] = ODM_GetRFReg(pAdapter, path, RF_REG[index], bMaskDWord);	
#endif	

	//Path A/B AFE all on
	for(index = 0; index < IQK_ADDA_REG_NUM ; index++)
		ODM_SetBBReg(pDM_Odm, AFE_REG[index], bMaskDWord, 0x6fdb25a4);

	//BB register setting
	for(index = 0; index < DP_BB_REG_NUM; index++)
	{
		if(index < 4)
			ODM_SetBBReg(pDM_Odm, BB_REG[index], bMaskDWord, BB_settings[index]);
		else if (index == 4)
			ODM_SetBBReg(pDM_Odm,BB_REG[index], bMaskDWord, BB_backup[index]|BIT10|BIT26);			
		else
			ODM_SetBBReg(pDM_Odm, BB_REG[index], BIT10, 0x00);			
	}

	//MAC register setting
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	_PHY_MACSettingCalibration_8812A(pAdapter, MAC_REG, MAC_backup);
#else
	_PHY_MACSettingCalibration_8812A(pDM_Odm, MAC_REG, MAC_backup);
#endif	

	//PAGE-E IQC setting	
	ODM_SetBBReg(pDM_Odm, rTx_IQK_Tone_A, bMaskDWord, 0x01008c00); 		
	ODM_SetBBReg(pDM_Odm, rRx_IQK_Tone_A, bMaskDWord, 0x01008c00);	
	ODM_SetBBReg(pDM_Odm, rTx_IQK_Tone_B, bMaskDWord, 0x01008c00);	
	ODM_SetBBReg(pDM_Odm, rRx_IQK_Tone_B, bMaskDWord, 0x01008c00);	

	//path_A DPK
	//Path B to standby mode
	ODM_SetRFReg(pDM_Odm, RF_PATH_B, RF_AC, bMaskDWord, 0x10000);

	// PA gain = 11 & PAD1 => tx_agc 1f ~11
	// PA gain = 11 & PAD2 => tx_agc 10~0e
	// PA gain = 01 => tx_agc 0b~0d
	// PA gain = 00 => tx_agc 0a~00
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, 0xffffff00, 0x400000);	
	ODM_SetBBReg(pDM_Odm, 0xbc0, bMaskDWord, 0x0005361f);		
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, 0xffffff00, 0);	

	//do inner loopback DPK 3 times 
	for(i = 0; i < 3; i++)
	{
		//PA gain = 11 & PAD2 => tx_agc = 0x0f/0x0c/0x07
		for(index = 0; index < 3; index++)
			ODM_SetBBReg(pDM_Odm, 0xe00+index*4, bMaskDWord, Tx_AGC[i][0]);			
		ODM_SetBBReg(pDM_Odm,0xe00+index*4, bMaskDWord, Tx_AGC[i][1]);			
		for(index = 0; index < 4; index++)
			ODM_SetBBReg(pDM_Odm,0xe10+index*4, bMaskDWord, Tx_AGC[i][0]);			

		// PAGE_B for Path-A inner loopback DPK setting
		ODM_SetBBReg(pDM_Odm,rPdp_AntA, bMaskDWord, 0x02097098);
		ODM_SetBBReg(pDM_Odm,rPdp_AntA_4, bMaskDWord, 0xf76d9f84);
		ODM_SetBBReg(pDM_Odm,rConfig_Pmpd_AntA, bMaskDWord, 0x0004ab87);
		ODM_SetBBReg(pDM_Odm,rConfig_AntA, bMaskDWord, 0x00880000);		

		//----send one shot signal----//
		// Path A
		ODM_SetBBReg(pDM_Odm,rConfig_Pmpd_AntA, bMaskDWord, 0x80047788);
		ODM_delay_ms(1);
		ODM_SetBBReg(pDM_Odm, rConfig_Pmpd_AntA, bMaskDWord, 0x00047788);
		ODM_delay_ms(50);
	}

	//PA gain = 11 => tx_agc = 1a
	for(index = 0; index < 3; index++)		
		ODM_SetBBReg(pDM_Odm,0xe00+index*4, bMaskDWord, 0x34343434);	
	ODM_SetBBReg(pDM_Odm,0xe08+index*4, bMaskDWord, 0x03903434);	
	for(index = 0; index < 4; index++)		
		ODM_SetBBReg(pDM_Odm,0xe10+index*4, bMaskDWord, 0x34343434);	

	//====================================
	// PAGE_B for Path-A DPK setting
	//====================================
	// open inner loopback @ b00[19]:10 od 0xb00 0x01097018
	ODM_SetBBReg(pDM_Odm,rPdp_AntA, bMaskDWord, 0x02017098);
	ODM_SetBBReg(pDM_Odm,rPdp_AntA_4, bMaskDWord, 0xf76d9f84);
	ODM_SetBBReg(pDM_Odm,rConfig_Pmpd_AntA, bMaskDWord, 0x0004ab87);
	ODM_SetBBReg(pDM_Odm,rConfig_AntA, bMaskDWord, 0x00880000);		

	//rf_lpbk_setup
	//1.rf 00:5205a, rf 0d:0e52c
	ODM_SetRFReg(pDM_Odm, RF_PATH_A, 0x0c, bMaskDWord, 0x8992b);
	ODM_SetRFReg(pDM_Odm, RF_PATH_A, 0x0d, bMaskDWord, 0x0e52c); 	
	ODM_SetRFReg(pDM_Odm, RF_PATH_A, 0x00, bMaskDWord, 0x5205a );		

	//----send one shot signal----//
	// Path A
	ODM_SetBBReg(pDM_Odm,rConfig_Pmpd_AntA, bMaskDWord, 0x800477c0);
	ODM_delay_ms(1);
	ODM_SetBBReg(pDM_Odm,rConfig_Pmpd_AntA, bMaskDWord, 0x000477c0);
	ODM_delay_ms(50);

	while(RetryCount < DP_RETRY_LIMIT && !pDM_Odm->RFCalibrateInfo.bDPPathAOK)
	{
		//----read back measurement results----//
		ODM_SetBBReg(pDM_Odm, rPdp_AntA, bMaskDWord, 0x0c297018);
		tmpReg = ODM_GetBBReg(pDM_Odm, 0xbe0, bMaskDWord);
		ODM_delay_ms(10);
		ODM_SetBBReg(pDM_Odm, rPdp_AntA, bMaskDWord, 0x0c29701f);
		tmpReg2 = ODM_GetBBReg(pDM_Odm, 0xbe8, bMaskDWord);
		ODM_delay_ms(10);

		tmpReg = (tmpReg & bMaskHWord) >> 16;
		tmpReg2 = (tmpReg2 & bMaskHWord) >> 16;		
		if(tmpReg < 0xf0 || tmpReg > 0x105 || tmpReg2 > 0xff )
		{
			ODM_SetBBReg(pDM_Odm, rPdp_AntA, bMaskDWord, 0x02017098);

			ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, 0xffffff00, 0x800000);
			ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, 0xffffff00, 0);
			ODM_delay_ms(1);
			ODM_SetBBReg(pDM_Odm, rConfig_Pmpd_AntA, bMaskDWord, 0x800477c0);
			ODM_delay_ms(1);			
			ODM_SetBBReg(pDM_Odm, rConfig_Pmpd_AntA, bMaskDWord, 0x000477c0);			
			ODM_delay_ms(50);			
			RetryCount++;			
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("path A DPK RetryCount %d 0xbe0[31:16] %x 0xbe8[31:16] %x\n", RetryCount, tmpReg, tmpReg2));										
		}
		else
		{
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("path A DPK Sucess\n"));		
			pDM_Odm->RFCalibrateInfo.bDPPathAOK = TRUE;
			break;
		}		
	}
	RetryCount = 0;

	//DPP path A
	if(pDM_Odm->RFCalibrateInfo.bDPPathAOK)
	{	
		// DP settings
		ODM_SetBBReg(pDM_Odm, rPdp_AntA, bMaskDWord, 0x01017098);
		ODM_SetBBReg(pDM_Odm, rPdp_AntA_4, bMaskDWord, 0x776d9f84);
		ODM_SetBBReg(pDM_Odm, rConfig_Pmpd_AntA, bMaskDWord, 0x0004ab87);
		ODM_SetBBReg(pDM_Odm, rConfig_AntA, bMaskDWord, 0x00880000);
		ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, 0xffffff00, 0x400000);

		for(i=rPdp_AntA; i<=0xb3c; i+=4)
		{
			ODM_SetBBReg(pDM_Odm, i, bMaskDWord, 0x40004000);	
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("path A ofsset = 0x%x\n", i));		
		}

		//pwsf
		ODM_SetBBReg(pDM_Odm, 0xb40, bMaskDWord, 0x40404040);	
		ODM_SetBBReg(pDM_Odm, 0xb44, bMaskDWord, 0x28324040);			
		ODM_SetBBReg(pDM_Odm, 0xb48, bMaskDWord, 0x10141920);					

		for(i=0xb4c; i<=0xb5c; i+=4)
		{
			ODM_SetBBReg(pDM_Odm, i, bMaskDWord, 0x0c0c0c0c);	
		}		

		//TX_AGC boundary
		ODM_SetBBReg(pDM_Odm, 0xbc0, bMaskDWord, 0x0005361f);	
		ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, 0xffffff00, 0);					
	}
	else
	{
		ODM_SetBBReg(pDM_Odm, rPdp_AntA, bMaskDWord, 0x00000000);	
		ODM_SetBBReg(pDM_Odm, rPdp_AntA_4, bMaskDWord, 0x00000000);			
	}

	//DPK path B
	if(is2T)
	{
		//Path A to standby mode
		ODM_SetRFReg(pDM_Odm, RF_PATH_A, RF_AC, bMaskDWord, 0x10000);

		// LUTs => tx_agc
		// PA gain = 11 & PAD1, => tx_agc 1f ~11
		// PA gain = 11 & PAD2, => tx_agc 10 ~0e
		// PA gain = 01 => tx_agc 0b ~0d
		// PA gain = 00 => tx_agc 0a ~00
		ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, 0xffffff00, 0x400000);	
		ODM_SetBBReg(pDM_Odm, 0xbc4, bMaskDWord, 0x0005361f);		
		ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, 0xffffff00, 0);	

		//do inner loopback DPK 3 times 
		for(i = 0; i < 3; i++)
		{
			//PA gain = 11 & PAD2 => tx_agc = 0x0f/0x0c/0x07
			for(index = 0; index < 4; index++)
				ODM_SetBBReg(pDM_Odm, 0x830+index*4, bMaskDWord, Tx_AGC[i][0]);			
			for(index = 0; index < 2; index++)
				ODM_SetBBReg(pDM_Odm, 0x848+index*4, bMaskDWord, Tx_AGC[i][0]);			
			for(index = 0; index < 2; index++)
				ODM_SetBBReg(pDM_Odm, 0x868+index*4, bMaskDWord, Tx_AGC[i][0]);			

			// PAGE_B for Path-A inner loopback DPK setting
			ODM_SetBBReg(pDM_Odm, rPdp_AntB, bMaskDWord, 0x02097098);
			ODM_SetBBReg(pDM_Odm, rPdp_AntB_4, bMaskDWord, 0xf76d9f84);
			ODM_SetBBReg(pDM_Odm, rConfig_Pmpd_AntB, bMaskDWord, 0x0004ab87);
			ODM_SetBBReg(pDM_Odm, rConfig_AntB, bMaskDWord, 0x00880000);		

			//----send one shot signal----//
			// Path B
			ODM_SetBBReg(pDM_Odm,rConfig_Pmpd_AntB, bMaskDWord, 0x80047788);
			ODM_delay_ms(1);
			ODM_SetBBReg(pDM_Odm, rConfig_Pmpd_AntB, bMaskDWord, 0x00047788);
			ODM_delay_ms(50);
		}

		// PA gain = 11 => tx_agc = 1a	
		for(index = 0; index < 4; index++)
			ODM_SetBBReg(pDM_Odm, 0x830+index*4, bMaskDWord, 0x34343434);	
		for(index = 0; index < 2; index++)
			ODM_SetBBReg(pDM_Odm, 0x848+index*4, bMaskDWord, 0x34343434);	
		for(index = 0; index < 2; index++)
			ODM_SetBBReg(pDM_Odm, 0x868+index*4, bMaskDWord, 0x34343434);	

		// PAGE_B for Path-B DPK setting
		ODM_SetBBReg(pDM_Odm, rPdp_AntB, bMaskDWord, 0x02017098);		
		ODM_SetBBReg(pDM_Odm, rPdp_AntB_4, bMaskDWord, 0xf76d9f84);		
		ODM_SetBBReg(pDM_Odm, rConfig_Pmpd_AntB, bMaskDWord, 0x0004ab87);		
		ODM_SetBBReg(pDM_Odm, rConfig_AntB, bMaskDWord, 0x00880000);		

		// RF lpbk switches on
		ODM_SetBBReg(pDM_Odm, 0x840, bMaskDWord, 0x0101000f);		
		ODM_SetBBReg(pDM_Odm, 0x840, bMaskDWord, 0x01120103);		

		//Path-B RF lpbk
		ODM_SetRFReg(pDM_Odm, RF_PATH_B, 0x0c, bMaskDWord, 0x8992b);
		ODM_SetRFReg(pDM_Odm, RF_PATH_B, 0x0d, bMaskDWord, 0x0e52c);
		ODM_SetRFReg(pDM_Odm, RF_PATH_B, RF_AC, bMaskDWord, 0x5205a); 

		//----send one shot signal----//
		ODM_SetBBReg(pDM_Odm, rConfig_Pmpd_AntB, bMaskDWord, 0x800477c0);		
		ODM_delay_ms(1);	
		ODM_SetBBReg(pDM_Odm, rConfig_Pmpd_AntB, bMaskDWord, 0x000477c0);		
		ODM_delay_ms(50);

		while(RetryCount < DP_RETRY_LIMIT && !pDM_Odm->RFCalibrateInfo.bDPPathBOK)
		{
			//----read back measurement results----//
			ODM_SetBBReg(pDM_Odm, rPdp_AntB, bMaskDWord, 0x0c297018);		
			tmpReg = ODM_GetBBReg(pDM_Odm, 0xbf0, bMaskDWord);
			ODM_SetBBReg(pDM_Odm, rPdp_AntB, bMaskDWord, 0x0c29701f);		
			tmpReg2 = ODM_GetBBReg(pDM_Odm, 0xbf8, bMaskDWord);

			tmpReg = (tmpReg & bMaskHWord) >> 16;
			tmpReg2 = (tmpReg2 & bMaskHWord) >> 16;

			if(tmpReg < 0xf0 || tmpReg > 0x105 || tmpReg2 > 0xff)
			{
				ODM_SetBBReg(pDM_Odm, rPdp_AntB, bMaskDWord, 0x02017098);		

				ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, 0xffffff00, 0x800000);
				ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, 0xffffff00, 0);
				ODM_delay_ms(1);
				ODM_SetBBReg(pDM_Odm, rConfig_Pmpd_AntB, bMaskDWord, 0x800477c0);		
				ODM_delay_ms(1);	
				ODM_SetBBReg(pDM_Odm, rConfig_Pmpd_AntB, bMaskDWord, 0x000477c0);		
				ODM_delay_ms(50);			
				RetryCount++;			
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("path B DPK RetryCount %d 0xbf0[31:16] %x, 0xbf8[31:16] %x\n", RetryCount , tmpReg, tmpReg2));														
			}
			else
			{
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("path B DPK Success\n"));						
				pDM_Odm->RFCalibrateInfo.bDPPathBOK = TRUE;
				break;
			}						
		}

		//DPP path B
		if(pDM_Odm->RFCalibrateInfo.bDPPathBOK)
		{
			// DP setting
			// LUT by SRAM
			ODM_SetBBReg(pDM_Odm, rPdp_AntB, bMaskDWord, 0x01017098);
			ODM_SetBBReg(pDM_Odm, rPdp_AntB_4, bMaskDWord, 0x776d9f84);
			ODM_SetBBReg(pDM_Odm, rConfig_Pmpd_AntB, bMaskDWord, 0x0004ab87);
			ODM_SetBBReg(pDM_Odm, rConfig_AntB, bMaskDWord, 0x00880000);

			ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, 0xffffff00, 0x400000);
			for(i=0xb60; i<=0xb9c; i+=4)
			{
				ODM_SetBBReg(pDM_Odm, i, bMaskDWord, 0x40004000);	
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("path B ofsset = 0x%x\n", i));
			}

			// PWSF
			ODM_SetBBReg(pDM_Odm, 0xba0, bMaskDWord, 0x40404040);	
			ODM_SetBBReg(pDM_Odm, 0xba4, bMaskDWord, 0x28324050);			
			ODM_SetBBReg(pDM_Odm, 0xba8, bMaskDWord, 0x0c141920);					

			for(i=0xbac; i<=0xbbc; i+=4)
			{
				ODM_SetBBReg(pDM_Odm, i, bMaskDWord, 0x0c0c0c0c);	
			}		

			// tx_agc boundary
			ODM_SetBBReg(pDM_Odm, 0xbc4, bMaskDWord, 0x0005361f);	
			ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, 0xffffff00, 0);			

		}
		else
		{
			ODM_SetBBReg(pDM_Odm, rPdp_AntB, bMaskDWord, 0x00000000);	
			ODM_SetBBReg(pDM_Odm, rPdp_AntB_4, bMaskDWord, 0x00000000);					
		}
	}

	//reload BB default value
	for(index=0; index<DP_BB_REG_NUM; index++)
		ODM_SetBBReg(pDM_Odm, BB_REG[index], bMaskDWord, BB_backup[index]);

	//reload RF default value
	for(path = 0; path<DP_PATH_NUM; path++)
	{
		for( i = 0 ; i < DP_RF_REG_NUM ; i++){
			ODM_SetRFReg(pDM_Odm, path, RF_REG[i], bMaskDWord, RF_backup[path][i]);
		}
	}
	ODM_SetRFReg(pDM_Odm, RF_PATH_A, RF_MODE1, bMaskDWord, 0x1000f);	//standby mode
	ODM_SetRFReg(pDM_Odm, RF_PATH_A, RF_MODE2, bMaskDWord, 0x20101);		//RF lpbk switches off

	//reload AFE default value
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	_PHY_ReloadADDARegisters_8812A(pAdapter, AFE_REG, AFE_backup, IQK_ADDA_REG_NUM);	

	//reload MAC default value	
	_PHY_ReloadMACRegisters_8812A(pAdapter, MAC_REG, MAC_backup);
#else
	_PHY_ReloadADDARegisters_8812A(pDM_Odm, AFE_REG, AFE_backup, IQK_ADDA_REG_NUM);	

	//reload MAC default value	
	_PHY_ReloadMACRegisters_8812A(pDM_Odm, MAC_REG, MAC_backup);
#endif		

	pDM_Odm->RFCalibrateInfo.bDPdone = TRUE;
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("<==phy_DigitalPredistortion_8812A()\n"));
#endif		
}

VOID
	phy_DigitalPredistortion_8812A_8812A(
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
			IN	PADAPTER	pAdapter
#else
			IN PDM_ODM_T	pDM_Odm
#endif		
			)
{
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
#endif
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;
#endif
#endif	
#ifdef DISABLE_BB_RF
	return;
#endif

	return;

	if(pDM_Odm->RFCalibrateInfo.bDPdone)
		return;
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)

	if(IS_92C_SERIAL( pHalData->VersionID)){
		phy_DigitalPredistortion_8812A(pAdapter, TRUE);
	}
	else
#endif		
	{
		// For 88C 1T1R
		phy_DigitalPredistortion_8812A(pAdapter, FALSE);
	}
}



//return value TRUE => Main; FALSE => Aux

	BOOLEAN phy_QueryRFPathSwitch_8812A(
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
			IN PDM_ODM_T		pDM_Odm,
#else
			IN	PADAPTER	pAdapter,
#endif
			IN	BOOLEAN		is2T
			)
{
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
#endif
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;
#endif
#endif	
	if(!pAdapter->bHWInitReady)
	{
		u1Byte	u1bTmp;
		u1bTmp = ODM_Read1Byte(pDM_Odm, REG_LEDCFG2) | BIT7;
		ODM_Write1Byte(pDM_Odm, REG_LEDCFG2, u1bTmp);
		//ODM_SetBBReg(pDM_Odm, REG_LEDCFG0, BIT23, 0x01);
		ODM_SetBBReg(pDM_Odm, rFPGA0_XAB_RFParameter, BIT13, 0x01);
	}

	if(is2T)		//
	{
		if(ODM_GetBBReg(pDM_Odm, rFPGA0_XB_RFInterfaceOE, BIT5|BIT6) == 0x01)
			return TRUE;
		else 
			return FALSE;
	}
	else
	{
		if((ODM_GetBBReg(pDM_Odm, rFPGA0_XB_RFInterfaceOE, BIT5|BIT4|BIT3) == 0x0) ||
				(ODM_GetBBReg(pDM_Odm, rConfig_ram64x16, BIT31) == 0x0))
			return TRUE;
		else 
			return FALSE;
	}
}



//return value TRUE => Main; FALSE => Aux
	BOOLEAN PHY_QueryRFPathSwitch_8812A(	
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
			IN PDM_ODM_T		pDM_Odm
#else
			IN	PADAPTER	pAdapter
#endif
			)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);

#ifdef DISABLE_BB_RF
	return TRUE;
#endif
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)

	//if(IS_92C_SERIAL( pHalData->VersionID)){
	if(IS_2T2R( pHalData->VersionID)){
		return phy_QueryRFPathSwitch_8812A(pAdapter, TRUE);
	}
	else
#endif		
	{
		// For 88C 1T1R
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
		return phy_QueryRFPathSwitch_8812A(pAdapter, FALSE);
#else
		return phy_QueryRFPathSwitch_8812A(pDM_Odm, FALSE);
#endif
	}
}
#endif



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

#include "mp_precomp.h"
#include "../phydm_precomp.h"



/*---------------------------Define Local Constant---------------------------*/
// 2010/04/25 MH Define the max tx power tracking tx agc power.
#define		ODM_TXPWRTRACK_MAX_IDX8821A		6

/*---------------------------Define Local Constant---------------------------*/


//3 ============================================================
//3 Tx Power Tracking
//3 ============================================================

VOID
ODM_TxPwrTrackSetPwr8821A(
	IN	PVOID		pDM_VOID,
	PWRTRACK_METHOD 	Method,
	u1Byte 				RFPath,
	u1Byte 				ChannelMappedIndex
	)
{
	PDM_ODM_T		pDM_Odm = (PDM_ODM_T)pDM_VOID;
	PADAPTER		Adapter = pDM_Odm->Adapter;
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);

	u1Byte			PwrTrackingLimit = 26; //+1.0dB
	u1Byte			TxRate = 0xFF;
	u1Byte			Final_OFDM_Swing_Index = 0; 
	u1Byte			Final_CCK_Swing_Index = 0; 
	u1Byte			i = 0;
	u4Byte			finalBbSwingIdx[1];
	PODM_RF_CAL_T	pRFCalibrateInfo = &(pDM_Odm->RFCalibrateInfo);

	if (pDM_Odm->mp_mode == TRUE) {
	#if (DM_ODM_SUPPORT_TYPE & (ODM_WIN | ODM_CE))
		#if (DM_ODM_SUPPORT_TYPE & ODM_WIN)
			#if (MP_DRIVER == 1)
					PMPT_CONTEXT pMptCtx = &(Adapter->MptCtx);
					
					TxRate = MptToMgntRate(pMptCtx->MptRateIndex);
			#endif
		#elif (DM_ODM_SUPPORT_TYPE & ODM_CE)
				PMPT_CONTEXT pMptCtx = &(Adapter->mppriv.MptCtx);
				
				TxRate = MptToMgntRate(pMptCtx->MptRateIndex);
		#endif	
	#endif
	} else {
		u2Byte	rate	 = *(pDM_Odm->pForcedDataRate);
		
		if (!rate) { /*auto rate*/
			if (rate != 0xFF) {
			#if (DM_ODM_SUPPORT_TYPE & ODM_WIN)
						TxRate = Adapter->HalFunc.GetHwRateFromMRateHandler(pDM_Odm->TxRate);
			#elif (DM_ODM_SUPPORT_TYPE & ODM_CE)
						TxRate = HwRateToMRate(pDM_Odm->TxRate);
			#endif
			}
		} else { /*force rate*/
			TxRate = (u1Byte)rate;
		}
	}
		
	ODM_RT_TRACE(pDM_Odm, ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD, ("Power Tracking TxRate=0x%X\n", TxRate));
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,("===>ODM_TxPwrTrackSetPwr8821A\n"));

	if(TxRate != 0xFF)
	{
		//2 CCK
		if((TxRate >= MGN_1M)&&(TxRate <= MGN_11M))
			PwrTrackingLimit = 32; //+4dB
		//2 OFDM
		else if((TxRate >= MGN_6M)&&(TxRate <= MGN_48M))
			PwrTrackingLimit = 30; //+3dB
		else if(TxRate == MGN_54M)
			PwrTrackingLimit = 28; //+2dB
		//2 HT
		else if((TxRate >= MGN_MCS0)&&(TxRate <= MGN_MCS2)) //QPSK/BPSK
			PwrTrackingLimit = 34; //+5dB
		else if((TxRate >= MGN_MCS3)&&(TxRate <= MGN_MCS4)) //16QAM
			PwrTrackingLimit = 30; //+3dB
		else if((TxRate >= MGN_MCS5)&&(TxRate <= MGN_MCS7)) //64QAM
			PwrTrackingLimit = 28; //+2dB
			
		//2 VHT
		else if((TxRate >= MGN_VHT1SS_MCS0)&&(TxRate <= MGN_VHT1SS_MCS2)) //QPSK/BPSK
			PwrTrackingLimit = 34; //+5dB
		else if((TxRate >= MGN_VHT1SS_MCS3)&&(TxRate <= MGN_VHT1SS_MCS4)) //16QAM
			PwrTrackingLimit = 30; //+3dB
		else if((TxRate >= MGN_VHT1SS_MCS5)&&(TxRate <= MGN_VHT1SS_MCS6)) //64QAM
			PwrTrackingLimit = 28; //+2dB
		else if(TxRate == MGN_VHT1SS_MCS7) //64QAM
			PwrTrackingLimit = 26; //+1dB
		else if(TxRate == MGN_VHT1SS_MCS8) //256QAM
			PwrTrackingLimit = 24; //+0dB
		else if(TxRate == MGN_VHT1SS_MCS9) //256QAM
			PwrTrackingLimit = 22; //-1dB

		else			
			PwrTrackingLimit = 24;
	}
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,("TxRate=0x%x, PwrTrackingLimit=%d\n", TxRate, PwrTrackingLimit));

	if (Method == BBSWING)
	{
		if (RFPath == ODM_RF_PATH_A)
		{		
			finalBbSwingIdx[ODM_RF_PATH_A] = (pDM_Odm->RFCalibrateInfo.OFDM_index[ODM_RF_PATH_A] > PwrTrackingLimit) ? PwrTrackingLimit : pDM_Odm->RFCalibrateInfo.OFDM_index[ODM_RF_PATH_A];
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,("pDM_Odm->RFCalibrateInfo.OFDM_index[ODM_RF_PATH_A]=%d, pDM_Odm->RealBbSwingIdx[ODM_RF_PATH_A]=%d\n",
				pDM_Odm->RFCalibrateInfo.OFDM_index[ODM_RF_PATH_A], finalBbSwingIdx[ODM_RF_PATH_A]));
			
			ODM_SetBBReg(pDM_Odm, rA_TxScale_Jaguar, 0xFFE00000, TxScalingTable_Jaguar[finalBbSwingIdx[ODM_RF_PATH_A]]);
		}
	}
	else if (Method == MIX_MODE)
	{
		ODM_RT_TRACE(pDM_Odm, ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD, ("pRFCalibrateInfo->DefaultOfdmIndex=%d, pRFCalibrateInfo->Absolute_OFDMSwingIdx[RFPath]=%d, RF_Path = %d\n",
			pRFCalibrateInfo->DefaultOfdmIndex, pRFCalibrateInfo->Absolute_OFDMSwingIdx[RFPath], RFPath));

		Final_CCK_Swing_Index = pRFCalibrateInfo->DefaultCckIndex + pRFCalibrateInfo->Absolute_OFDMSwingIdx[RFPath];
		Final_OFDM_Swing_Index = pRFCalibrateInfo->DefaultOfdmIndex + pRFCalibrateInfo->Absolute_OFDMSwingIdx[RFPath];

		if (RFPath == ODM_RF_PATH_A)  
		{
			if (Final_OFDM_Swing_Index > PwrTrackingLimit)     /*BBSwing higher then Limit*/
			{
				pRFCalibrateInfo->Remnant_CCKSwingIdx = Final_CCK_Swing_Index - PwrTrackingLimit;
				pRFCalibrateInfo->Remnant_OFDMSwingIdx[RFPath] = Final_OFDM_Swing_Index - PwrTrackingLimit;            

				ODM_SetBBReg(pDM_Odm, rA_TxScale_Jaguar, 0xFFE00000, TxScalingTable_Jaguar[PwrTrackingLimit]);

				pRFCalibrateInfo->Modify_TxAGC_Flag_PathA = TRUE;

				PHY_SetTxPowerLevelByPath(Adapter, pHalData->CurrentChannel, ODM_RF_PATH_A);

				ODM_RT_TRACE(pDM_Odm, ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD, ("******Path_A Over BBSwing Limit , PwrTrackingLimit = %d , Remnant TxAGC Value = %d\n", PwrTrackingLimit, pRFCalibrateInfo->Remnant_OFDMSwingIdx[RFPath]));
			}
			else if (Final_OFDM_Swing_Index <= 0)
			{
				pRFCalibrateInfo->Remnant_CCKSwingIdx = Final_CCK_Swing_Index;
				pRFCalibrateInfo->Remnant_OFDMSwingIdx[RFPath] = Final_OFDM_Swing_Index;     

				ODM_SetBBReg(pDM_Odm, rA_TxScale_Jaguar, 0xFFE00000, TxScalingTable_Jaguar[0]);

				pRFCalibrateInfo->Modify_TxAGC_Flag_PathA = TRUE;

				PHY_SetTxPowerLevelByPath(Adapter, pHalData->CurrentChannel, ODM_RF_PATH_A);

				ODM_RT_TRACE(pDM_Odm, ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD, ("******Path_A Lower then BBSwing lower bound  0 , Remnant TxAGC Value = %d\n", pRFCalibrateInfo->Remnant_OFDMSwingIdx[RFPath]));
			}
			else
			{
				ODM_SetBBReg(pDM_Odm, rA_TxScale_Jaguar, 0xFFE00000, TxScalingTable_Jaguar[Final_OFDM_Swing_Index]);

				ODM_RT_TRACE(pDM_Odm, ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD, ("******Path_A Compensate with BBSwing , Final_OFDM_Swing_Index = %d\n", Final_OFDM_Swing_Index));

				if (pRFCalibrateInfo->Modify_TxAGC_Flag_PathA)  /*If TxAGC has changed, reset TxAGC again*/
				{
					pRFCalibrateInfo->Remnant_CCKSwingIdx = 0;
					pRFCalibrateInfo->Remnant_OFDMSwingIdx[RFPath] = 0;

					PHY_SetTxPowerLevelByPath(Adapter, pHalData->CurrentChannel, ODM_RF_PATH_A);

					pRFCalibrateInfo->Modify_TxAGC_Flag_PathA = FALSE;

					ODM_RT_TRACE(pDM_Odm, ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD, ("******Path_A pDM_Odm->Modify_TxAGC_Flag = FALSE\n"));
				}
			}
		}
	}
	else
	{
		return;
	}
}	// odm_TxPwrTrackSetPwr88E

VOID
GetDeltaSwingTable_8821A(
	IN	PVOID		pDM_VOID,
	OUT pu1Byte 			*TemperatureUP_A,
	OUT pu1Byte 			*TemperatureDOWN_A,
	OUT pu1Byte 			*TemperatureUP_B,
	OUT pu1Byte 			*TemperatureDOWN_B	
	)
{
	PDM_ODM_T		pDM_Odm = (PDM_ODM_T)pDM_VOID;
    PADAPTER        Adapter   		 = pDM_Odm->Adapter;
	PODM_RF_CAL_T  	pRFCalibrateInfo = &(pDM_Odm->RFCalibrateInfo);
	HAL_DATA_TYPE  	*pHalData  		 = GET_HAL_DATA(Adapter);
	u1Byte			TxRate			= 0xFF;
	u1Byte         	channel   		 = pHalData->CurrentChannel;

	if (pDM_Odm->mp_mode == TRUE) {
	#if (DM_ODM_SUPPORT_TYPE & (ODM_WIN | ODM_CE))
		#if (DM_ODM_SUPPORT_TYPE & ODM_WIN)
			#if (MP_DRIVER == 1)
					PMPT_CONTEXT pMptCtx = &(Adapter->MptCtx);
					
					TxRate = MptToMgntRate(pMptCtx->MptRateIndex);
			#endif
		#elif (DM_ODM_SUPPORT_TYPE & ODM_CE)
				PMPT_CONTEXT pMptCtx = &(Adapter->mppriv.MptCtx);
				
				TxRate = MptToMgntRate(pMptCtx->MptRateIndex);
		#endif	
	#endif
	} else {
		u2Byte	rate	 = *(pDM_Odm->pForcedDataRate);
		
		if (!rate) { /*auto rate*/
			if (rate != 0xFF) {
			#if (DM_ODM_SUPPORT_TYPE & ODM_WIN)
						TxRate = Adapter->HalFunc.GetHwRateFromMRateHandler(pDM_Odm->TxRate);
			#elif (DM_ODM_SUPPORT_TYPE & ODM_CE)
						TxRate = HwRateToMRate(pDM_Odm->TxRate);
			#endif
			}
		} else { /*force rate*/
			TxRate = (u1Byte)rate;
		}
	}
		
	ODM_RT_TRACE(pDM_Odm, ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD, ("Power Tracking TxRate=0x%X\n", TxRate));

		
	if ( 1 <= channel && channel <= 14) {
		if (IS_CCK_RATE(TxRate)) {
	        *TemperatureUP_A   = pRFCalibrateInfo->DeltaSwingTableIdx_2GCCKA_P;
	        *TemperatureDOWN_A = pRFCalibrateInfo->DeltaSwingTableIdx_2GCCKA_N;
	        *TemperatureUP_B   = pRFCalibrateInfo->DeltaSwingTableIdx_2GCCKB_P;
	        *TemperatureDOWN_B = pRFCalibrateInfo->DeltaSwingTableIdx_2GCCKB_N;		
		} else {
	        *TemperatureUP_A   = pRFCalibrateInfo->DeltaSwingTableIdx_2GA_P;
	        *TemperatureDOWN_A = pRFCalibrateInfo->DeltaSwingTableIdx_2GA_N;
	        *TemperatureUP_B   = pRFCalibrateInfo->DeltaSwingTableIdx_2GB_P;
	        *TemperatureDOWN_B = pRFCalibrateInfo->DeltaSwingTableIdx_2GB_N;			
		}
 	} else if ( 36 <= channel && channel <= 64) {
        *TemperatureUP_A   = pRFCalibrateInfo->DeltaSwingTableIdx_5GA_P[0];
        *TemperatureDOWN_A = pRFCalibrateInfo->DeltaSwingTableIdx_5GA_N[0];
        *TemperatureUP_B   = pRFCalibrateInfo->DeltaSwingTableIdx_5GB_P[0];
        *TemperatureDOWN_B = pRFCalibrateInfo->DeltaSwingTableIdx_5GB_N[0];
    } else if ( 100 <= channel && channel <= 140) {
        *TemperatureUP_A   = pRFCalibrateInfo->DeltaSwingTableIdx_5GA_P[1];
        *TemperatureDOWN_A = pRFCalibrateInfo->DeltaSwingTableIdx_5GA_N[1];
        *TemperatureUP_B   = pRFCalibrateInfo->DeltaSwingTableIdx_5GB_P[1];
        *TemperatureDOWN_B = pRFCalibrateInfo->DeltaSwingTableIdx_5GB_N[1];
    } else if ( 149 <= channel && channel <= 173) {
        *TemperatureUP_A   = pRFCalibrateInfo->DeltaSwingTableIdx_5GA_P[2]; 
        *TemperatureDOWN_A = pRFCalibrateInfo->DeltaSwingTableIdx_5GA_N[2]; 
        *TemperatureUP_B   = pRFCalibrateInfo->DeltaSwingTableIdx_5GB_P[2]; 
        *TemperatureDOWN_B = pRFCalibrateInfo->DeltaSwingTableIdx_5GB_N[2]; 
    } else {
	    *TemperatureUP_A   = (pu1Byte)DeltaSwingTableIdx_2GA_P_8188E;
	    *TemperatureDOWN_A = (pu1Byte)DeltaSwingTableIdx_2GA_N_8188E;	
	    *TemperatureUP_B   = (pu1Byte)DeltaSwingTableIdx_2GA_P_8188E;
	    *TemperatureDOWN_B = (pu1Byte)DeltaSwingTableIdx_2GA_N_8188E;		
    }
	
	return;
}

void ConfigureTxpowerTrack_8821A(
	PTXPWRTRACK_CFG	pConfig
	)
{
	pConfig->SwingTableSize_CCK = TXSCALE_TABLE_SIZE;
	pConfig->SwingTableSize_OFDM = TXSCALE_TABLE_SIZE;
	pConfig->Threshold_IQK = IQK_THRESHOLD;
	pConfig->AverageThermalNum = AVG_THERMAL_NUM_8812A;
	pConfig->RfPathCount = MAX_PATH_NUM_8821A;
	pConfig->ThermalRegAddr = RF_T_METER_8812A;
		
	pConfig->ODM_TxPwrTrackSetPwr = ODM_TxPwrTrackSetPwr8821A;
	pConfig->DoIQK = DoIQK_8821A;
	pConfig->PHY_LCCalibrate = PHY_LCCalibrate_8821A;
	pConfig->GetDeltaSwingTable = GetDeltaSwingTable_8821A;
}

#define		DP_BB_REG_NUM		7
#define		DP_RF_REG_NUM		1
#define		DP_RETRY_LIMIT		10
#define		DP_PATH_NUM		2
#define		DP_DPK_NUM		3
#define		DP_DPK_VALUE_NUM	2

VOID
PHY_LCCalibrate_8821A(
	IN	PVOID		pDM_VOID
	)
{
	PDM_ODM_T		pDM_Odm = (PDM_ODM_T)pDM_VOID;
	u8Byte		StartTime; 
	u8Byte		ProgressingTime;

	StartTime = ODM_GetCurrentTime( pDM_Odm);
	PHY_LCCalibrate_8812A(pDM_Odm);
	ProgressingTime = ODM_GetProgressingTime( pDM_Odm, StartTime);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("LCK ProgressingTime = %lld\n", ProgressingTime));
}



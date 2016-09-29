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
#define		ODM_TXPWRTRACK_MAX_IDX8812A		6

/*---------------------------Define Local Constant---------------------------*/

//3============================================================
//3 Tx Power Tracking
//3============================================================


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
#endif

	ODM_ResetIQKResult(pDM_Odm);		


	pDM_Odm->RFCalibrateInfo.ThermalValue_IQK= ThermalValue;
	PHY_IQCalibrate_8812A(Adapter, FALSE);
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
	u4Byte 	finalBbSwingIdx[2];
	
	PADAPTER		Adapter = pDM_Odm->Adapter;
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);

	u1Byte			PwrTrackingLimit = 26; //+1.0dB
	u1Byte			TxRate = 0xFF;
	u1Byte			Final_OFDM_Swing_Index = 0; 
	u1Byte			Final_CCK_Swing_Index = 0; 
	u1Byte			i = 0;
	PODM_RF_CAL_T  	pRFCalibrateInfo = &(pDM_Odm->RFCalibrateInfo);

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
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,("===>ODM_TxPwrTrackSetPwr8812A\n"));

	if(TxRate != 0xFF)   //20130429 Mimic Modify High Rate BBSwing Limit.
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

        else if((TxRate >= MGN_MCS8)&&(TxRate <= MGN_MCS10)) //QPSK/BPSK
            PwrTrackingLimit = 34; //+5dB
        else if((TxRate >= MGN_MCS11)&&(TxRate <= MGN_MCS12)) //16QAM
            PwrTrackingLimit = 30; //+3dB
        else if((TxRate >= MGN_MCS13)&&(TxRate <= MGN_MCS15)) //64QAM
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

        else if((TxRate >= MGN_VHT2SS_MCS0)&&(TxRate <= MGN_VHT2SS_MCS2)) //QPSK/BPSK
            PwrTrackingLimit = 34; //+5dB
        else if((TxRate >= MGN_VHT2SS_MCS3)&&(TxRate <= MGN_VHT2SS_MCS4)) //16QAM
            PwrTrackingLimit = 30; //+3dB
        else if((TxRate >= MGN_VHT2SS_MCS5)&&(TxRate <= MGN_VHT2SS_MCS6)) //64QAM
            PwrTrackingLimit = 28; //+2dB
        else if(TxRate == MGN_VHT2SS_MCS7) //64QAM
            PwrTrackingLimit = 26; //+1dB
        else if(TxRate == MGN_VHT2SS_MCS8) //256QAM
            PwrTrackingLimit = 24; //+0dB
        else if(TxRate == MGN_VHT2SS_MCS9) //256QAM
            PwrTrackingLimit = 22; //-1dB

        else			
            PwrTrackingLimit = 24;
    }
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,("TxRate=0x%x, PwrTrackingLimit=%d\n", TxRate, PwrTrackingLimit));

	
	if (Method == BBSWING)
	{
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,("===>ODM_TxPwrTrackSetPwr8812A\n"));		

		if (RFPath == ODM_RF_PATH_A)
		{		
			finalBbSwingIdx[ODM_RF_PATH_A] = (pRFCalibrateInfo->OFDM_index[ODM_RF_PATH_A] > PwrTrackingLimit) ? PwrTrackingLimit : pRFCalibrateInfo->OFDM_index[ODM_RF_PATH_A];
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,("pRFCalibrateInfo->OFDM_index[ODM_RF_PATH_A]=%d, pDM_Odm->RealBbSwingIdx[ODM_RF_PATH_A]=%d\n",
				pRFCalibrateInfo->OFDM_index[ODM_RF_PATH_A], finalBbSwingIdx[ODM_RF_PATH_A]));
			
			ODM_SetBBReg(pDM_Odm, rA_TxScale_Jaguar, 0xFFE00000, TxScalingTable_Jaguar[finalBbSwingIdx[ODM_RF_PATH_A]]);
		}
		else
		{
			finalBbSwingIdx[ODM_RF_PATH_B] = (pRFCalibrateInfo->OFDM_index[ODM_RF_PATH_B] > PwrTrackingLimit) ? PwrTrackingLimit : pRFCalibrateInfo->OFDM_index[ODM_RF_PATH_B];
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,("pRFCalibrateInfo->OFDM_index[ODM_RF_PATH_B]=%d, pDM_Odm->RealBbSwingIdx[ODM_RF_PATH_B]=%d\n",
				pRFCalibrateInfo->OFDM_index[ODM_RF_PATH_B], finalBbSwingIdx[ODM_RF_PATH_B]));
			
			ODM_SetBBReg(pDM_Odm, rB_TxScale_Jaguar, 0xFFE00000, TxScalingTable_Jaguar[finalBbSwingIdx[ODM_RF_PATH_B]]);
		}
	}

	else if (Method == MIX_MODE)
	{
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,("pRFCalibrateInfo->DefaultOfdmIndex=%d, pRFCalibrateInfo->Absolute_OFDMSwingIdx[RFPath]=%d, RF_Path = %d\n",
			pRFCalibrateInfo->DefaultOfdmIndex, pRFCalibrateInfo->Absolute_OFDMSwingIdx[RFPath],RFPath ));
		
		Final_CCK_Swing_Index = pRFCalibrateInfo->DefaultCckIndex + pRFCalibrateInfo->Absolute_OFDMSwingIdx[RFPath];	
		Final_OFDM_Swing_Index = pRFCalibrateInfo->DefaultOfdmIndex + pRFCalibrateInfo->Absolute_OFDMSwingIdx[RFPath];
		
		if (RFPath == ODM_RF_PATH_A)  
        {

            if(Final_OFDM_Swing_Index > PwrTrackingLimit)     //BBSwing higher then Limit
            {
                pRFCalibrateInfo->Remnant_CCKSwingIdx= Final_CCK_Swing_Index - PwrTrackingLimit;            // CCK Follow the same compensate value as Path A
                pRFCalibrateInfo->Remnant_OFDMSwingIdx[RFPath] = Final_OFDM_Swing_Index - PwrTrackingLimit;            

                ODM_SetBBReg(pDM_Odm, rA_TxScale_Jaguar, 0xFFE00000, TxScalingTable_Jaguar[PwrTrackingLimit]);

                pRFCalibrateInfo->Modify_TxAGC_Flag_PathA= TRUE;

                PHY_SetTxPowerLevelByPath(Adapter, pHalData->CurrentChannel, ODM_RF_PATH_A);

                ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,("******Path_A Over BBSwing Limit , PwrTrackingLimit = %d , Remnant TxAGC Value = %d \n", PwrTrackingLimit, pRFCalibrateInfo->Remnant_OFDMSwingIdx[RFPath]));
            }
            else if (Final_OFDM_Swing_Index < 0)
            {
                pRFCalibrateInfo->Remnant_CCKSwingIdx= Final_CCK_Swing_Index;            // CCK Follow the same compensate value as Path A
                pRFCalibrateInfo->Remnant_OFDMSwingIdx[RFPath] = Final_OFDM_Swing_Index;     

                ODM_SetBBReg(pDM_Odm, rA_TxScale_Jaguar, 0xFFE00000, TxScalingTable_Jaguar[0]);

                pRFCalibrateInfo->Modify_TxAGC_Flag_PathA= TRUE;

                PHY_SetTxPowerLevelByPath(Adapter, pHalData->CurrentChannel, ODM_RF_PATH_A);

                ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,("******Path_A Lower then BBSwing lower bound  0 , Remnant TxAGC Value = %d \n", pRFCalibrateInfo->Remnant_OFDMSwingIdx[RFPath]));
            }
            else
            {
                ODM_SetBBReg(pDM_Odm, rA_TxScale_Jaguar, 0xFFE00000, TxScalingTable_Jaguar[Final_OFDM_Swing_Index]);

                ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,("******Path_A Compensate with BBSwing , Final_OFDM_Swing_Index = %d \n", Final_OFDM_Swing_Index));

                if(pRFCalibrateInfo->Modify_TxAGC_Flag_PathA)  //If TxAGC has changed, reset TxAGC again
                {
                    pRFCalibrateInfo->Remnant_CCKSwingIdx= 0;
                    pRFCalibrateInfo->Remnant_OFDMSwingIdx[RFPath] = 0;     

                    PHY_SetTxPowerLevelByPath(Adapter, pHalData->CurrentChannel, ODM_RF_PATH_A);

                    pRFCalibrateInfo->Modify_TxAGC_Flag_PathA= FALSE;

                    ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,("******Path_A pDM_Odm->Modify_TxAGC_Flag = FALSE \n"));
                }
            }	
        }
		
		if (RFPath == ODM_RF_PATH_B)  
		{
			if(Final_OFDM_Swing_Index > PwrTrackingLimit)     //BBSwing higher then Limit
			{
				pRFCalibrateInfo->Remnant_OFDMSwingIdx[RFPath] = Final_OFDM_Swing_Index - PwrTrackingLimit;            

				ODM_SetBBReg(pDM_Odm, rB_TxScale_Jaguar, 0xFFE00000, TxScalingTable_Jaguar[PwrTrackingLimit]);

				pRFCalibrateInfo->Modify_TxAGC_Flag_PathB= TRUE;

				PHY_SetTxPowerLevelByPath(Adapter, pHalData->CurrentChannel, ODM_RF_PATH_B);

				ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,("******Path_B Over BBSwing Limit , PwrTrackingLimit = %d , Remnant TxAGC Value = %d \n", PwrTrackingLimit, pRFCalibrateInfo->Remnant_OFDMSwingIdx[RFPath]));
			}
			else if (Final_OFDM_Swing_Index < 0)
			{
				pRFCalibrateInfo->Remnant_OFDMSwingIdx[RFPath] = Final_OFDM_Swing_Index;     

				ODM_SetBBReg(pDM_Odm, rB_TxScale_Jaguar, 0xFFE00000, TxScalingTable_Jaguar[0]);

				pRFCalibrateInfo->Modify_TxAGC_Flag_PathB = TRUE;

				PHY_SetTxPowerLevelByPath(Adapter, pHalData->CurrentChannel, ODM_RF_PATH_B);

				ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,("******Path_B Lower then BBSwing lower bound  0 , Remnant TxAGC Value = %d \n", pRFCalibrateInfo->Remnant_OFDMSwingIdx[RFPath]));
			}
			else
			{
				ODM_SetBBReg(pDM_Odm, rB_TxScale_Jaguar, 0xFFE00000, TxScalingTable_Jaguar[Final_OFDM_Swing_Index]);

				ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,("******Path_B Compensate with BBSwing , Final_OFDM_Swing_Index = %d \n", Final_OFDM_Swing_Index));

				if(pRFCalibrateInfo->Modify_TxAGC_Flag_PathB)  //If TxAGC has changed, reset TxAGC again
				{
					pRFCalibrateInfo->Remnant_OFDMSwingIdx[RFPath] = 0;     

					PHY_SetTxPowerLevelByPath(Adapter, pHalData->CurrentChannel, ODM_RF_PATH_B);

					pRFCalibrateInfo->Modify_TxAGC_Flag_PathB = FALSE;

					ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,("******Path_B pDM_Odm->Modify_TxAGC_Flag = FALSE \n"));
				}
			}
		}
		
	}
	else
	{
		return;
	}
}

VOID
GetDeltaSwingTable_8812A(
	IN 	PDM_ODM_T			pDM_Odm,
	OUT pu1Byte 			*TemperatureUP_A,
	OUT pu1Byte 			*TemperatureDOWN_A,
	OUT pu1Byte 			*TemperatureUP_B,
	OUT pu1Byte 			*TemperatureDOWN_B	
	)
{
    PADAPTER        Adapter   		 = pDM_Odm->Adapter;
	PODM_RF_CAL_T  	pRFCalibrateInfo = &(pDM_Odm->RFCalibrateInfo);
	HAL_DATA_TYPE  	*pHalData  		 = GET_HAL_DATA(Adapter);
	u1Byte		TxRate			= 0xFF;
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

void ConfigureTxpowerTrack_8812A(
	PTXPWRTRACK_CFG	pConfig
	)
{
	pConfig->SwingTableSize_CCK = TXSCALE_TABLE_SIZE;
	pConfig->SwingTableSize_OFDM = TXSCALE_TABLE_SIZE;
	pConfig->Threshold_IQK = IQK_THRESHOLD;
	pConfig->Threshold_DPK = DPK_THRESHOLD;	
	pConfig->AverageThermalNum = AVG_THERMAL_NUM_8812A;
	pConfig->RfPathCount = MAX_PATH_NUM_8812A;
	pConfig->ThermalRegAddr = RF_T_METER_8812A;
		
	pConfig->ODM_TxPwrTrackSetPwr = ODM_TxPwrTrackSetPwr8812A;
	pConfig->DoIQK = DoIQK_8812A;
	pConfig->PHY_LCCalibrate = PHY_LCCalibrate_8812A;
	pConfig->GetDeltaSwingTable = GetDeltaSwingTable_8812A;
}


#define BW_20M 	0
#define	BW_40M  1
#define	BW_80M	2

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
			if (RX_X>>1 >=0x112 || (RX_Y>>1 >= 0x12 && RX_Y>>1 <= 0x3ee)){
				ODM_SetBBReg(pDM_Odm, 0xc10, 0x000003ff, 0x100);
				ODM_SetBBReg(pDM_Odm, 0xc10, 0x03ff0000, 0);
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RX_X = %x;;RX_Y = %x ====>fill to IQC\n", RX_X>>1&0x000003ff, RX_Y>>1&0x000003ff));
			}
			else{
				ODM_SetBBReg(pDM_Odm, 0xc10, 0x000003ff, RX_X>>1);
				ODM_SetBBReg(pDM_Odm, 0xc10, 0x03ff0000, RX_Y>>1);
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RX_X = %x;;RX_Y = %x ====>fill to IQC\n", RX_X>>1&0x000003ff, RX_Y>>1&0x000003ff));
				ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("0xc10 = %x ====>fill to IQC\n", ODM_Read4Byte(pDM_Odm, 0xc10)));
			}
		}
		break;
	case ODM_RF_PATH_B:
		{	
			ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C	
			if (RX_X>>1 >=0x112 || (RX_Y>>1 >= 0x12 && RX_Y>>1 <= 0x3ee)){
				ODM_SetBBReg(pDM_Odm, 0xe10, 0x000003ff, 0x100);
				ODM_SetBBReg(pDM_Odm, 0xe10, 0x03ff0000, 0);
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RX_X = %x;;RX_Y = %x ====>fill to IQC\n", RX_X>>1&0x000003ff, RX_Y>>1&0x000003ff));
			}
			else{
				ODM_SetBBReg(pDM_Odm, 0xe10, 0x000003ff, RX_X>>1);
				ODM_SetBBReg(pDM_Odm, 0xe10, 0x03ff0000, RX_Y>>1);
				ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RX_X = %x;;RX_Y = %x====>fill to IQC\n ", RX_X>>1&0x000003ff, RX_Y>>1&0x000003ff));
				ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("0xe10 = %x====>fill to IQC\n", ODM_Read4Byte(pDM_Odm, 0xe10)));
			}
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
			ODM_SetBBReg(pDM_Odm, 0xc90, BIT(7), 0x1);
			ODM_SetBBReg(pDM_Odm, 0xcc4, BIT(18), 0x1);
			if (!pDM_Odm->DPK_Done)
				ODM_SetBBReg(pDM_Odm, 0xcc4, BIT(29), 0x1);
			ODM_SetBBReg(pDM_Odm, 0xcc8, BIT(29), 0x1);	
			ODM_SetBBReg(pDM_Odm, 0xccc, 0x000007ff, TX_Y);
			ODM_SetBBReg(pDM_Odm, 0xcd4, 0x000007ff, TX_X);
			ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("TX_X = %x;;TX_Y = %x =====> fill to IQC\n", TX_X&0x000007ff, TX_Y&0x000007ff));
			ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("0xcd4 = %x;;0xccc = %x ====>fill to IQC\n", ODM_GetBBReg(pDM_Odm, 0xcd4, 0x000007ff), ODM_GetBBReg(pDM_Odm, 0xccc, 0x000007ff)));
		}
		break;
	case ODM_RF_PATH_B:
		{
			ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 1 --> Page C1
			ODM_SetBBReg(pDM_Odm, 0xe90, BIT(7), 0x1);
			ODM_SetBBReg(pDM_Odm, 0xec4, BIT(18), 0x1);
			if (!pDM_Odm->DPK_Done)
				ODM_SetBBReg(pDM_Odm, 0xec4, BIT(29), 0x1);
			ODM_SetBBReg(pDM_Odm, 0xec8, BIT(29), 0x1);	
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
	IN PDM_ODM_T	pDM_Odm,
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
	IN PDM_ODM_T	pDM_Odm,
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
        	RFA_backup[i] = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_A, Backup_RF_REG[i], bMaskDWord);
        	RFB_backup[i] = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_B, Backup_RF_REG[i], bMaskDWord);
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
	ODM_SetBBReg(pDM_Odm, 0xc90, BIT(7), 0x1);
	ODM_SetBBReg(pDM_Odm, 0xcc4, BIT(18), 0x1);
	if (!pDM_Odm->DPK_Done)
		ODM_SetBBReg(pDM_Odm, 0xcc4, BIT(29), 0x1);
	ODM_SetBBReg(pDM_Odm, 0xcc8, BIT(29), 0x1);	
	//ODM_Write4Byte(pDM_Odm, 0xcb8, 0x0);
	ODM_Write4Byte(pDM_Odm, 0xe80, 0x0);
	ODM_Write4Byte(pDM_Odm, 0xe84, 0x0);
	ODM_Write4Byte(pDM_Odm, 0xe88, 0x0);
	ODM_Write4Byte(pDM_Odm, 0xe8c, 0x3c000000);
	ODM_SetBBReg(pDM_Odm, 0xe90, BIT(7), 0x1);
	ODM_SetBBReg(pDM_Odm, 0xec4, BIT(18), 0x1);
	if (!pDM_Odm->DPK_Done)
		ODM_SetBBReg(pDM_Odm, 0xec4, BIT(29), 0x1);
	ODM_SetBBReg(pDM_Odm, 0xec8, BIT(29), 0x1);	
	//ODM_Write4Byte(pDM_Odm, 0xeb8, 0x0);
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
	ODM_Write1Byte(pDM_Odm, 0x808, 0x00);		//		RX ante off
	ODM_SetBBReg(pDM_Odm, 0x838, 0xf, 0xc);		//		CCA off
	ODM_Write1Byte(pDM_Odm, 0xa07, 0xf);		//  		CCK RX Path off
}

#define cal_num 10

void _IQK_Tx_8812A(
	IN PDM_ODM_T		pDM_Odm,
	IN u1Byte chnlIdx
	)
{
	u1Byte 		delay_count;
	u1Byte		cal0_retry, cal1_retry, TX0_Average = 0, TX1_Average = 0, RX0_Average = 0, RX1_Average = 0;
	int			TX_IQC_temp[10][4], TX_IQC[4];		//TX_IQC = [TX0_X, TX0_Y,TX1_X,TX1_Y]; for 3 times
	int			RX_IQC_temp[10][4], RX_IQC[4];		//RX_IQC = [RX0_X, RX0_Y,RX1_X,RX1_Y]; for 3 times
	BOOLEAN 	TX0_fail = TRUE, RX0_fail = TRUE, IQK0_ready = FALSE, TX0_finish = FALSE, RX0_finish = FALSE;
	BOOLEAN  	TX1_fail = TRUE, RX1_fail = TRUE, IQK1_ready = FALSE, TX1_finish = FALSE, RX1_finish = FALSE;
	int			i, ii, dx = 0, dy = 0;
	
	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("BandWidth = %d, ExtPA5G = %d, ExtPA2G = %d\n", *pDM_Odm->pBandWidth, pDM_Odm->ExtPA5G, pDM_Odm->ExtPA));
	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Interface = %d, RFE_Type = %d\n", pDM_Odm->SupportInterface, pDM_Odm->RFEType));

	ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
	// ========Path-A AFE all on========
	// Port 0 DAC/ADC on
	ODM_Write4Byte(pDM_Odm, 0xc60, 0x77777777);
	ODM_Write4Byte(pDM_Odm, 0xc64, 0x77777777);
	 
	// Port 1 DAC/ADC on
	ODM_Write4Byte(pDM_Odm, 0xe60, 0x77777777);
	ODM_Write4Byte(pDM_Odm, 0xe64, 0x77777777);

	ODM_Write4Byte(pDM_Odm, 0xc68, 0x19791979);
	ODM_Write4Byte(pDM_Odm, 0xe68, 0x19791979);
	ODM_SetBBReg(pDM_Odm, 0xc00, 0xf, 0x4);// 	hardware 3-wire off
	ODM_SetBBReg(pDM_Odm, 0xe00, 0xf, 0x4);// 	hardware 3-wire off

	// DAC/ADC sampling rate (160 MHz)
	ODM_SetBBReg(pDM_Odm, 0xc5c, BIT(26)|BIT(25)|BIT(24), 0x7);
	ODM_SetBBReg(pDM_Odm, 0xe5c, BIT(26)|BIT(25)|BIT(24), 0x7);
			
        //====== Path A TX IQK RF Setting ======
	ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0xef, bRFRegOffsetMask, 0x80002);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x30, bRFRegOffsetMask, 0x20000);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x31, bRFRegOffsetMask, 0x3fffd);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x32, bRFRegOffsetMask, 0xfe83f);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x65, bRFRegOffsetMask, 0x931d5);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x8f, bRFRegOffsetMask, 0x8a001);
	//====== Path B TX IQK RF Setting ======
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0xef, bRFRegOffsetMask, 0x80002);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x30, bRFRegOffsetMask, 0x20000);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x31, bRFRegOffsetMask, 0x3fffd);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x32, bRFRegOffsetMask, 0xfe83f);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x65, bRFRegOffsetMask, 0x931d5);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x8f, bRFRegOffsetMask, 0x8a001);
	ODM_Write4Byte(pDM_Odm, 0x90c, 0x00008000);
	ODM_SetBBReg(pDM_Odm, 0xc94, BIT(0), 0x1);
	ODM_SetBBReg(pDM_Odm, 0xe94, BIT(0), 0x1);
	ODM_Write4Byte(pDM_Odm, 0x978, 0x29002000);// TX (X,Y)
	ODM_Write4Byte(pDM_Odm, 0x97c, 0xa9002000);// RX (X,Y)
	ODM_Write4Byte(pDM_Odm, 0x984, 0x00462910);// [0]:AGC_en, [15]:idac_K_Mask
	ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 1 --> Page C1

	if (pDM_Odm->ExtPA5G){
		if (pDM_Odm->RFEType == 1){
			ODM_Write4Byte(pDM_Odm, 0xc88, 0x821403e3);
			ODM_Write4Byte(pDM_Odm, 0xe88, 0x821403e3);
		}
		else{
			ODM_Write4Byte(pDM_Odm, 0xc88, 0x821403f7);
			ODM_Write4Byte(pDM_Odm, 0xe88, 0x821403f7);
		}
	}
	else{
		ODM_Write4Byte(pDM_Odm, 0xc88, 0x821403f1);
		ODM_Write4Byte(pDM_Odm, 0xe88, 0x821403f1);
	}
	if (*pDM_Odm->pBandType){
		ODM_Write4Byte(pDM_Odm, 0xc8c, 0x68163e96);
		ODM_Write4Byte(pDM_Odm, 0xe8c, 0x68163e96);
	}
	else{
		ODM_Write4Byte(pDM_Odm, 0xc8c, 0x28163e96);
		ODM_Write4Byte(pDM_Odm, 0xe8c, 0x28163e96);
		if (pDM_Odm->RFEType == 3){	
			if (pDM_Odm->ExtPA)
				ODM_Write4Byte(pDM_Odm, 0xc88, 0x821403e3);
			else
				ODM_Write4Byte(pDM_Odm, 0xc88, 0x821403f7);
		}
	}

	
		ODM_Write4Byte(pDM_Odm, 0xc80, 0x18008c10);// TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16
		ODM_Write4Byte(pDM_Odm, 0xc84, 0x38008c10);// RX_Tone_idx[9:0], RxK_Mask[29]
		ODM_Write4Byte(pDM_Odm, 0xce8, 0x00000000);
		ODM_Write4Byte(pDM_Odm, 0xe80, 0x18008c10);// TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16
		ODM_Write4Byte(pDM_Odm, 0xe84, 0x38008c10);// RX_Tone_idx[9:0], RxK_Mask[29]
		ODM_Write4Byte(pDM_Odm, 0xee8, 0x00000000);

		cal0_retry = 0;
		cal1_retry = 0;
		while(1){
			// one shot
			ODM_Write4Byte(pDM_Odm, 0xcb8, 0x00100000);// cb8[20] 將 SI/PI 使用權切給 iqk_dpk module
			ODM_Write4Byte(pDM_Odm, 0xeb8, 0x00100000);// cb8[20] 將 SI/PI 使用權切給 iqk_dpk module
			ODM_Write4Byte(pDM_Odm, 0x980, 0xfa000000);
			ODM_Write4Byte(pDM_Odm, 0x980, 0xf8000000);
			
		ODM_delay_ms(10); //Delay 10ms
			ODM_Write4Byte(pDM_Odm, 0xcb8, 0x00000000);
			ODM_Write4Byte(pDM_Odm, 0xeb8, 0x00000000);
			delay_count = 0;
			while (1){
				if (!TX0_finish)
					IQK0_ready = (BOOLEAN) ODM_GetBBReg(pDM_Odm, 0xd00, BIT(10));
				if (!TX1_finish)
					IQK1_ready = (BOOLEAN) ODM_GetBBReg(pDM_Odm, 0xd40, BIT(10));
				if ((IQK0_ready && IQK1_ready) || (delay_count>20))
					break;
				else{
			ODM_delay_ms(1);
				delay_count++;
				}
			}
			ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("TX delay_count = %d\n", delay_count));
			if (delay_count < 20){							// If 20ms No Result, then cal_retry++
				// ============TXIQK Check==============
				TX0_fail = (BOOLEAN) ODM_GetBBReg(pDM_Odm, 0xd00, BIT(12));
				TX1_fail = (BOOLEAN) ODM_GetBBReg(pDM_Odm, 0xd40, BIT(12));
				if (!(TX0_fail || TX0_finish)){
					ODM_Write4Byte(pDM_Odm, 0xcb8, 0x02000000);
					TX_IQC_temp[TX0_Average][0] = ODM_GetBBReg(pDM_Odm, 0xd00, 0x07ff0000)<<21;
					ODM_Write4Byte(pDM_Odm, 0xcb8, 0x04000000);
					TX_IQC_temp[TX0_Average][1] = ODM_GetBBReg(pDM_Odm, 0xd00, 0x07ff0000)<<21;
					ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("TX_X0[%d] = %x ;; TX_Y0[%d] = %x\n", TX0_Average, (TX_IQC_temp[TX0_Average][0])>>21&0x000007ff, TX0_Average, (TX_IQC_temp[TX0_Average][1])>>21&0x000007ff));
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
					TX0_Average++;
			}
			else{
				cal0_retry++;
				if (cal0_retry == 10)
					break;
				}
			if (!(TX1_fail || TX1_finish)){
				ODM_Write4Byte(pDM_Odm, 0xeb8, 0x02000000);
				TX_IQC_temp[TX1_Average][2] = ODM_GetBBReg(pDM_Odm, 0xd40, 0x07ff0000)<<21;
				ODM_Write4Byte(pDM_Odm, 0xeb8, 0x04000000);
				TX_IQC_temp[TX1_Average][3] = ODM_GetBBReg(pDM_Odm, 0xd40, 0x07ff0000)<<21;
				ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("TX_X1[%d] = %x ;; TX_Y1[%d] = %x\n", TX1_Average, (TX_IQC_temp[TX1_Average][2])>>21&0x000007ff, TX1_Average, (TX_IQC_temp[TX1_Average][3])>>21&0x000007ff));
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
				TX1_Average++;
				}
			else{
				cal1_retry++;
				if (cal1_retry == 10)
					break;
				}
			}
			else{
				cal0_retry++;
				cal1_retry++;
				ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Delay 20ms TX IQK Not Ready!!!!!\n"));
				if (cal0_retry == 10)
					break;	
			}
			if (TX0_Average >= 2){
				for (i = 0; i < TX0_Average; i++){
					for (ii = i+1; ii <TX0_Average; ii++){
						dx = (TX_IQC_temp[i][0]>>21) - (TX_IQC_temp[ii][0]>>21);
						if (dx < 4 && dx > -4){
							dy = (TX_IQC_temp[i][1]>>21) - (TX_IQC_temp[ii][1]>>21);
							if (dy < 4 && dy > -4){
								TX_IQC[0] = ((TX_IQC_temp[i][0]>>21) + (TX_IQC_temp[ii][0]>>21))/2;
								TX_IQC[1] = ((TX_IQC_temp[i][1]>>21) + (TX_IQC_temp[ii][1]>>21))/2;
								ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("TXA_X = %x;;TXA_Y = %x\n", TX_IQC[0]&0x000007ff, TX_IQC[1]&0x000007ff));
								TX0_finish = TRUE;
							}
						}
					}
				}
			}
			if (TX1_Average >= 2){
				for (i = 0; i < TX1_Average; i++){
					for (ii = i+1; ii <TX1_Average; ii++){
						dx = (TX_IQC_temp[i][2]>>21) - (TX_IQC_temp[ii][2]>>21);
						if (dx < 4 && dx > -4){
							dy = (TX_IQC_temp[i][3]>>21) - (TX_IQC_temp[ii][3]>>21);
							if (dy < 4 && dy > -4){
								TX_IQC[2] = ((TX_IQC_temp[i][2]>>21) + (TX_IQC_temp[ii][2]>>21))/2;
								TX_IQC[3] = ((TX_IQC_temp[i][3]>>21) + (TX_IQC_temp[ii][3]>>21))/2;
								ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("TXB_X = %x;;TXB_Y = %x\n", TX_IQC[2]&0x000007ff, TX_IQC[3]&0x000007ff));
								TX1_finish = TRUE;
							}
						}
					}
				}
			}
			ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("TX0_Average = %d, TX1_Average = %d\n", TX0_Average, TX1_Average));
			ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("TX0_finish = %d, TX1_finish = %d\n", TX0_finish, TX1_finish));
			if (TX0_finish && TX1_finish)
				break;
			if ((cal0_retry + TX0_Average) >= 10 || (cal1_retry + TX1_Average) >= 10 )
				break;
		}
		ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("TXA_cal_retry = %d\n", cal0_retry));
		ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("TXB_cal_retry = %d\n", cal1_retry));

	

	ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x58, 0x7fe00, ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x8, 0xffc00)); // Load LOK
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x58, 0x7fe00, ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x8, 0xffc00)); // Load LOK
	ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 1 --> Page C1
	
		ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
		if (TX0_finish){
		//====== Path A RX IQK RF Setting======
		ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0xef, bRFRegOffsetMask, 0x80000);
		ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x30, bRFRegOffsetMask, 0x30000);
		ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x31, bRFRegOffsetMask, 0x3f7ff);
		ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x32, bRFRegOffsetMask, 0xfe7bf);
		ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x8f, bRFRegOffsetMask, 0x88001);
		ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x65, bRFRegOffsetMask, 0x931d1);
		ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0xef, bRFRegOffsetMask, 0x00000);
		}
		if (TX1_finish){
		//====== Path B RX IQK RF Setting======
		ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0xef, bRFRegOffsetMask, 0x80000);
		ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x30, bRFRegOffsetMask, 0x30000);
		ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x31, bRFRegOffsetMask, 0x3f7ff);
		ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x32, bRFRegOffsetMask, 0xfe7bf);
		ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x8f, bRFRegOffsetMask, 0x88001);
		ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x65, bRFRegOffsetMask, 0x931d1);
		ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0xef, bRFRegOffsetMask, 0x00000);
		}
		ODM_SetBBReg(pDM_Odm, 0x978, BIT(31), 0x1);
		ODM_SetBBReg(pDM_Odm, 0x97c, BIT(31), 0x0);
		ODM_Write4Byte(pDM_Odm, 0x90c, 0x00008000);
            if (pDM_Odm->SupportInterface == ODM_ITRF_PCIE)
		     ODM_Write4Byte(pDM_Odm, 0x984, 0x0046a911);
                else
                     ODM_Write4Byte(pDM_Odm, 0x984, 0x0046a890);
		//ODM_Write4Byte(pDM_Odm, 0x984, 0x0046a890);
	if (pDM_Odm->RFEType == 1){
			ODM_Write4Byte(pDM_Odm, 0xcb0, 0x77777717);
			ODM_Write4Byte(pDM_Odm, 0xcb4, 0x00000077);
			ODM_Write4Byte(pDM_Odm, 0xeb0, 0x77777717);
			ODM_Write4Byte(pDM_Odm, 0xeb4, 0x00000077);
		}
		else{
		ODM_Write4Byte(pDM_Odm, 0xcb0, 0x77777717);
		ODM_Write4Byte(pDM_Odm, 0xcb4, 0x02000077);
		ODM_Write4Byte(pDM_Odm, 0xeb0, 0x77777717);
		ODM_Write4Byte(pDM_Odm, 0xeb4, 0x02000077);
		}
		
		
		ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 1 --> Page C1
		if (TX0_finish){
		ODM_Write4Byte(pDM_Odm, 0xc80, 0x38008c10);// TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16
		ODM_Write4Byte(pDM_Odm, 0xc84, 0x18008c10);// RX_Tone_idx[9:0], RxK_Mask[29]
		ODM_Write4Byte(pDM_Odm, 0xc88, 0x82140119);
		}
		if (TX1_finish){
		ODM_Write4Byte(pDM_Odm, 0xe80, 0x38008c10);// TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16
		ODM_Write4Byte(pDM_Odm, 0xe84, 0x18008c10);// RX_Tone_idx[9:0], RxK_Mask[29]
		ODM_Write4Byte(pDM_Odm, 0xe88, 0x82140119);
		}
              cal0_retry = 0;
		cal1_retry = 0;
		while(1){
		    // one shot
		    	ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
		    	if (TX0_finish){
			ODM_SetBBReg(pDM_Odm, 0x978, 0x03FF8000, (TX_IQC[0])&0x000007ff);
			ODM_SetBBReg(pDM_Odm, 0x978, 0x000007FF, (TX_IQC[1])&0x000007ff);
			ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 1 --> Page C1
		if (pDM_Odm->RFEType == 1){
				ODM_Write4Byte(pDM_Odm, 0xc8c, 0x28161500);
			}
			else{
				ODM_Write4Byte(pDM_Odm, 0xc8c, 0x28160cc0);
			}
			ODM_Write4Byte(pDM_Odm, 0xcb8, 0x00300000);// cb8[20] 將 SI/PI 使用權切給 iqk_dpk module
			ODM_Write4Byte(pDM_Odm, 0xcb8, 0x00100000);
		ODM_delay_ms(5); //Delay 5ms
			ODM_Write4Byte(pDM_Odm, 0xc8c, 0x3c000000);
			ODM_Write4Byte(pDM_Odm, 0xcb8, 0x00000000);
		    	}
			if (TX1_finish){
			ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
			ODM_SetBBReg(pDM_Odm, 0x978, 0x03FF8000, (TX_IQC[2])&0x000007ff);
			ODM_SetBBReg(pDM_Odm, 0x978, 0x000007FF, (TX_IQC[3])&0x000007ff);
			ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 1 --> Page C1
		if (pDM_Odm->RFEType == 1){
				ODM_Write4Byte(pDM_Odm, 0xe8c, 0x28161500);
			}
			else{
				ODM_Write4Byte(pDM_Odm, 0xe8c, 0x28160ca0);
			}
			ODM_Write4Byte(pDM_Odm, 0xeb8, 0x00300000);// cb8[20] 將 SI/PI 使用權切給 iqk_dpk module
			ODM_Write4Byte(pDM_Odm, 0xeb8, 0x00100000);// cb8[20] 將 SI/PI 使用權切給 iqk_dpk module
		ODM_delay_ms(5); //Delay 5ms
			ODM_Write4Byte(pDM_Odm, 0xe8c, 0x3c000000);
			ODM_Write4Byte(pDM_Odm, 0xeb8, 0x00000000);
			}
			delay_count = 0;
			while (1){
				if (!RX0_finish && TX0_finish)
					IQK0_ready = (BOOLEAN) ODM_GetBBReg(pDM_Odm, 0xd00, BIT(10));
				if (!RX1_finish && TX1_finish)
					IQK1_ready = (BOOLEAN) ODM_GetBBReg(pDM_Odm, 0xd40, BIT(10));
				if ((IQK0_ready && IQK1_ready)||(delay_count>20))
					break;
				else{
				ODM_delay_ms(1);
					delay_count++;
				}
			}
			ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RX delay_count = %d\n", delay_count));
			if (delay_count < 20){	// If 20ms No Result, then cal_retry++
				// ============RXIQK Check==============
				RX0_fail = (BOOLEAN) ODM_GetBBReg(pDM_Odm, 0xd00, BIT(11));
				RX1_fail = (BOOLEAN) ODM_GetBBReg(pDM_Odm, 0xd40, BIT(11));
				if (!(RX0_fail || RX0_finish) && TX0_finish){
					ODM_Write4Byte(pDM_Odm, 0xcb8, 0x06000000);
					RX_IQC_temp[RX0_Average][0] = ODM_GetBBReg(pDM_Odm, 0xd00, 0x07ff0000)<<21;
					ODM_Write4Byte(pDM_Odm, 0xcb8, 0x08000000);
					RX_IQC_temp[RX0_Average][1] = ODM_GetBBReg(pDM_Odm, 0xd00, 0x07ff0000)<<21;
					ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RX_X0[%d] = %x ;; RX_Y0[%d] = %x\n", RX0_Average, (RX_IQC_temp[RX0_Average][0])>>21&0x000007ff, RX0_Average, (RX_IQC_temp[RX0_Average][1])>>21&0x000007ff));
/*					
					ODM_Write4Byte(pDM_Odm, 0xcb8, 0x05000000);
					reg1 = ODM_GetBBReg(pDM_Odm, 0xd00, 0xffffffff);
					ODM_Write4Byte(pDM_Odm, 0xcb8, 0x06000000);
					reg2 = ODM_GetBBReg(pDM_Odm, 0xd00, 0x0000001f);
					DbgPrint("reg1 = %d, reg2 = %d", reg1, reg2);
					Image_Power = (reg2<<32)+reg1;
					DbgPrint("Before PW = %d\n", Image_Power);
					ODM_Write4Byte(pDM_Odm, 0xcb8, 0x07000000);
					reg1 = ODM_GetBBReg(pDM_Odm, 0xd00, 0xffffffff);
					ODM_Write4Byte(pDM_Odm, 0xcb8, 0x08000000);
					reg2 = ODM_GetBBReg(pDM_Odm, 0xd00, 0x0000001f);
					Image_Power = (reg2<<32)+reg1;
					DbgPrint("After PW = %d\n", Image_Power);
*/					
					RX0_Average++;
				}
				else{
					ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("1. RXA_cal_retry = %d\n", cal0_retry));
					cal0_retry++;
					if (cal0_retry == 10)
					break;
				}
				if (!(RX1_fail || RX1_finish) && TX1_finish){
                            		ODM_Write4Byte(pDM_Odm, 0xeb8, 0x06000000);
                            		RX_IQC_temp[RX1_Average][2] = ODM_GetBBReg(pDM_Odm, 0xd40, 0x07ff0000)<<21;
                            		ODM_Write4Byte(pDM_Odm, 0xeb8, 0x08000000);
                            		RX_IQC_temp[RX1_Average][3] = ODM_GetBBReg(pDM_Odm, 0xd40, 0x07ff0000)<<21;
					ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RX_X1[%d] = %x ;; RX_Y1[%d] = %x\n", RX1_Average, (RX_IQC_temp[RX1_Average][2])>>21&0x000007ff, RX1_Average, (RX_IQC_temp[RX1_Average][3])>>21&0x000007ff));
/*					
					ODM_Write4Byte(pDM_Odm, 0xeb8, 0x05000000);
					reg1 = ODM_GetBBReg(pDM_Odm, 0xd40, 0xffffffff);
					ODM_Write4Byte(pDM_Odm, 0xeb8, 0x06000000);
					reg2 = ODM_GetBBReg(pDM_Odm, 0xd40, 0x0000001f);
					DbgPrint("reg1 = %d, reg2 = %d", reg1, reg2);
					Image_Power = (reg2<<32)+reg1;
					DbgPrint("Before PW = %d\n", Image_Power);
					ODM_Write4Byte(pDM_Odm, 0xeb8, 0x07000000);
					reg1 = ODM_GetBBReg(pDM_Odm, 0xd40, 0xffffffff);
					ODM_Write4Byte(pDM_Odm, 0xeb8, 0x08000000);
					reg2 = ODM_GetBBReg(pDM_Odm, 0xd40, 0x0000001f);
					Image_Power = (reg2<<32)+reg1;
					DbgPrint("After PW = %d\n", Image_Power);
*/				
					RX1_Average++;
                        	}
                        	else{
                            		cal1_retry++;
                            		if (cal1_retry == 10)
                                		break;
                    		}
				
			}
			else{
				ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("2. RXA_cal_retry = %d\n", cal0_retry));
				cal0_retry++;
				cal1_retry++;
				ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Delay 20ms RX IQK Not Ready!!!!!\n"));
			    if (cal0_retry == 10)
			        break;
			}
			ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("3. RXA_cal_retry = %d\n", cal0_retry));
			if (RX0_Average >= 2){
				for (i = 0; i < RX0_Average; i++){
					for (ii = i+1; ii <RX0_Average; ii++){
					dx = (RX_IQC_temp[i][0]>>21) - (RX_IQC_temp[ii][0]>>21);
						if (dx < 4 && dx > -4){
						dy = (RX_IQC_temp[i][1]>>21) - (RX_IQC_temp[ii][1]>>21);
							if (dy < 4 && dy > -4){
								RX_IQC[0]= ((RX_IQC_temp[i][0]>>21) + (RX_IQC_temp[ii][0]>>21))/2;
								RX_IQC[1] = ((RX_IQC_temp[i][1]>>21) + (RX_IQC_temp[ii][1]>>21))/2;
								RX0_finish = TRUE;
								break;
							}
						}
					}
				}
			}
			if (RX1_Average >= 2){
				for (i = 0; i < RX1_Average; i++){
					for (ii = i+1; ii <RX1_Average; ii++){
					dx = (RX_IQC_temp[i][2]>>21) - (RX_IQC_temp[ii][2]>>21);
						if (dx < 4 && dx > -4){
						dy = (RX_IQC_temp[i][3]>>21) - (RX_IQC_temp[ii][3]>>21);
							if (dy < 4 && dy > -4){
								RX_IQC[2] = ((RX_IQC_temp[i][2]>>21) + (RX_IQC_temp[ii][2]>>21))/2;
								RX_IQC[3] = ((RX_IQC_temp[i][3]>>21) + (RX_IQC_temp[ii][3]>>21))/2;
								RX1_finish = TRUE;
								break;
							}
						}
					}
				}
			}
			ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RX0_Average = %d, RX1_Average = %d\n", RX0_Average, RX1_Average));
			ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RX0_finish = %d, RX1_finish = %d\n", RX0_finish, RX1_finish));
			if ((RX0_finish|| !TX0_finish) && (RX1_finish || !TX1_finish) )
				break;
			if ((cal0_retry + RX0_Average) >= 10 || (cal1_retry + RX1_Average) >= 10 || RX0_Average == 3 || RX1_Average == 3)
				break;
		}
		ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RXA_cal_retry = %d\n", cal0_retry));
		ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("RXB_cal_retry = %d\n", cal1_retry));


	// FillIQK Result
	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("========Path_A =======\n"));

	if (TX0_finish){
		_IQK_TX_FillIQC_8812A(pDM_Odm, ODM_RF_PATH_A, TX_IQC[0], TX_IQC[1]);
	}
	else{
		_IQK_TX_FillIQC_8812A(pDM_Odm, ODM_RF_PATH_A, 0x200, 0x0);
	}



	if (RX0_finish){
		_IQK_RX_FillIQC_8812A(pDM_Odm, ODM_RF_PATH_A, RX_IQC[0], RX_IQC[1]);
	}
	else{
		_IQK_RX_FillIQC_8812A(pDM_Odm, ODM_RF_PATH_A, 0x200, 0x0);
	}

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("========Path_B =======\n"));

	if (TX1_finish){
		_IQK_TX_FillIQC_8812A(pDM_Odm, ODM_RF_PATH_B, TX_IQC[2], TX_IQC[3]);
	}
	else{
		_IQK_TX_FillIQC_8812A(pDM_Odm, ODM_RF_PATH_B, 0x200, 0x0);
	}



	if (RX1_finish){
		_IQK_RX_FillIQC_8812A(pDM_Odm, ODM_RF_PATH_B, RX_IQC[2], RX_IQC[3]);
	}
	else{
		_IQK_RX_FillIQC_8812A(pDM_Odm, ODM_RF_PATH_B, 0x200, 0x0);
	}


		
}

#define MACBB_REG_NUM 9
#define AFE_REG_NUM 12
#define RF_REG_NUM 3

// Maintained by BB James.
VOID	
phy_IQCalibrate_8812A(
	IN PDM_ODM_T		pDM_Odm,
	IN u1Byte		Channel
	)
{
	u4Byte	MACBB_backup[MACBB_REG_NUM], AFE_backup[AFE_REG_NUM], RFA_backup[RF_REG_NUM], RFB_backup[RF_REG_NUM];
	u4Byte 	Backup_MACBB_REG[MACBB_REG_NUM] = {0x520, 0x550, 0x808, 0xa04, 0x90c, 0xc00, 0xe00, 0x838,  0x82c}; 
	u4Byte 	Backup_AFE_REG[AFE_REG_NUM] = {0xc5c, 0xc60, 0xc64, 0xc68, 0xcb0, 0xcb4,
		       	                                                   0xe5c, 0xe60, 0xe64, 0xe68, 0xeb0, 0xeb4}; 
	u4Byte	Reg_C1B8, Reg_E1B8;
	u4Byte 	Backup_RF_REG[RF_REG_NUM] = {0x65, 0x8f, 0x0}; 
	u1Byte 	chnlIdx = GetRightChnlPlaceforIQK(Channel);

	_IQK_BackupMacBB_8812A(pDM_Odm, MACBB_backup, Backup_MACBB_REG, MACBB_REG_NUM);
	ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1);
	Reg_C1B8 = ODM_Read4Byte(pDM_Odm, 0xcb8);
	Reg_E1B8 = ODM_Read4Byte(pDM_Odm, 0xeb8);
	ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0);
	_IQK_BackupAFE_8812A(pDM_Odm, AFE_backup, Backup_AFE_REG, AFE_REG_NUM);
	_IQK_BackupRF_8812A(pDM_Odm, RFA_backup, RFB_backup, Backup_RF_REG, RF_REG_NUM);
	
	_IQK_ConfigureMAC_8812A(pDM_Odm);
	_IQK_Tx_8812A(pDM_Odm, chnlIdx);
	_IQK_RestoreRF_8812A(pDM_Odm, ODM_RF_PATH_A, Backup_RF_REG, RFA_backup, RF_REG_NUM);
	_IQK_RestoreRF_8812A(pDM_Odm, ODM_RF_PATH_B, Backup_RF_REG, RFB_backup, RF_REG_NUM);
	
	_IQK_RestoreAFE_8812A(pDM_Odm, AFE_backup, Backup_AFE_REG, AFE_REG_NUM);
	ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1);
	ODM_Write4Byte(pDM_Odm, 0xcb8, Reg_C1B8);
	ODM_Write4Byte(pDM_Odm, 0xeb8, Reg_E1B8);
	ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0);
	_IQK_RestoreMacBB_8812A(pDM_Odm, MACBB_backup, Backup_MACBB_REG, MACBB_REG_NUM);


}

VOID	
phy_LCCalibrate_8812A(
	IN 	PDM_ODM_T	pDM_Odm,
	IN	BOOLEAN		is2T
	)
{
	u4Byte	/*RF_Amode=0, RF_Bmode=0,*/ LC_Cal = 0, tmp = 0;
	
	//Check continuous TX and Packet TX
	u4Byte	reg0x914 = ODM_Read4Byte(pDM_Odm, rSingleTone_ContTx_Jaguar);;

	// Backup RF reg18.
	LC_Cal = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_CHNLBW, bRFRegOffsetMask);

	if((reg0x914 & 0x70000) != 0)	//If contTx, disable all continuous TX. 0x914[18:16]
		// <20121121, Kordan> A workaround: If we set 0x914[18:16] as zero, BB turns off ContTx
		// until another packet comes in. To avoid ContTx being turned off, we skip this step.
		;//ODM_Write4Byte(pDM_Odm, rSingleTone_ContTx_Jaguar, reg0x914 & (~0x70000));	
	else							// If packet Tx-ing, pause Tx.
		ODM_Write1Byte(pDM_Odm, REG_TXPAUSE_8812A, 0xFF);			


/*
	//3 1. Read original RF mode
	RF_Amode = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_AC, bRFRegOffsetMask);
	if(is2T)
		RF_Bmode = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_B, RF_AC, bRFRegOffsetMask);	


	//3 2. Set RF mode = standby mode
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_AC, bRFRegOffsetMask, (RF_Amode&0x8FFFF)|0x10000);
	if(is2T)
		ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, RF_AC, bRFRegOffsetMask, (RF_Bmode&0x8FFFF)|0x10000);			
*/

	// Enter LCK mode
	tmp = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_LCK, bRFRegOffsetMask);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_LCK, bRFRegOffsetMask, tmp | BIT14);

	//3 3. Read RF reg18
	LC_Cal = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_CHNLBW, bRFRegOffsetMask);
	
	//3 4. Set LC calibration begin bit15
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_CHNLBW, bRFRegOffsetMask, LC_Cal|0x08000);

	ODM_delay_ms(150);		// suggest by RFSI Binson

	// Leave LCK mode
	tmp = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_LCK, bRFRegOffsetMask);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_LCK, bRFRegOffsetMask, tmp & ~BIT14);
	
	//3 Restore original situation
	if((reg0x914 & 70000) != 0) 	//Deal with contisuous TX case, 0x914[18:16]
	{  
		// <20121121, Kordan> A workaround: If we set 0x914[18:16] as zero, BB turns off ContTx
		// until another packet comes in. To avoid ContTx being turned off, we skip this step.
		//ODM_Write4Byte(pDM_Odm, rSingleTone_ContTx_Jaguar, reg0x914); 
	}
	else // Deal with Packet TX case
	{
		ODM_Write1Byte(pDM_Odm, REG_TXPAUSE_8812A, 0x00);	
	}

	// Recover channel number
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_CHNLBW, bRFRegOffsetMask, LC_Cal);

	/*
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_AC, bRFRegOffsetMask, RF_Amode);
	if(is2T)
		ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, RF_AC, bRFRegOffsetMask, RF_Bmode);
		*/

}





#define		DP_BB_REG_NUM		7
#define		DP_RF_REG_NUM		1
#define		DP_RETRY_LIMIT		10
#define		DP_PATH_NUM		2
#define		DP_DPK_NUM			3
#define		DP_DPK_VALUE_NUM	2

VOID	
phy_ReloadIQKSetting_8812A(
 	IN	PDM_ODM_T	pDM_Odm,
	IN	u1Byte		Channel
 	)
{
	PODM_RF_CAL_T  pRFCalibrateInfo = &(pDM_Odm->RFCalibrateInfo);

	u1Byte chnlIdx = GetRightChnlPlaceforIQK(Channel);
	ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 1 --> Page C1
	ODM_SetBBReg(pDM_Odm, 0xccc, 0x000007ff, pRFCalibrateInfo->IQKMatrixRegSetting[chnlIdx].Value[*pDM_Odm->pBandWidth][0]&0x7ff);
	ODM_SetBBReg(pDM_Odm, 0xcd4, 0x000007ff, (pRFCalibrateInfo->IQKMatrixRegSetting[chnlIdx].Value[*pDM_Odm->pBandWidth][0]&0x7ff0000)>>16);
	ODM_SetBBReg(pDM_Odm, 0xecc, 0x000007ff, pRFCalibrateInfo->IQKMatrixRegSetting[chnlIdx].Value[*pDM_Odm->pBandWidth][2]&0x7ff);
	ODM_SetBBReg(pDM_Odm, 0xed4, 0x000007ff, (pRFCalibrateInfo->IQKMatrixRegSetting[chnlIdx].Value[*pDM_Odm->pBandWidth][2]&0x7ff0000)>>16);

	if (*pDM_Odm->pBandWidth != 2){
		ODM_Write4Byte(pDM_Odm, 0xce8, 0x0);
		ODM_Write4Byte(pDM_Odm, 0xee8, 0x0);
	}
	else{
		ODM_Write4Byte(pDM_Odm, 0xce8, pRFCalibrateInfo->IQKMatrixRegSetting[chnlIdx].Value[*pDM_Odm->pBandWidth][4]);
		ODM_Write4Byte(pDM_Odm, 0xee8, pRFCalibrateInfo->IQKMatrixRegSetting[chnlIdx].Value[*pDM_Odm->pBandWidth][5]);
	}
	ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
	ODM_SetBBReg(pDM_Odm, 0xc10, 0x000003ff, (pRFCalibrateInfo->IQKMatrixRegSetting[chnlIdx].Value[*pDM_Odm->pBandWidth][1]&0x7ff0000)>>17);
	ODM_SetBBReg(pDM_Odm, 0xc10, 0x03ff0000, (pRFCalibrateInfo->IQKMatrixRegSetting[chnlIdx].Value[*pDM_Odm->pBandWidth][1]&0x7ff)>>1);
	ODM_SetBBReg(pDM_Odm, 0xe10, 0x000003ff, (pRFCalibrateInfo->IQKMatrixRegSetting[chnlIdx].Value[*pDM_Odm->pBandWidth][3]&0x7ff0000)>>17);
	ODM_SetBBReg(pDM_Odm, 0xe10, 0x03ff0000, (pRFCalibrateInfo->IQKMatrixRegSetting[chnlIdx].Value[*pDM_Odm->pBandWidth][3]&0x7ff)>>1);
	

}

VOID 
PHY_ResetIQKResult_8812A(
	IN	PDM_ODM_T	pDM_Odm
)
{
	ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x1); // [31] = 1 --> Page C1
	ODM_SetBBReg(pDM_Odm, 0xccc, 0x000007ff, 0x0);
	ODM_SetBBReg(pDM_Odm, 0xcd4, 0x000007ff, 0x200);
	ODM_SetBBReg(pDM_Odm, 0xecc, 0x000007ff, 0x0);
	ODM_SetBBReg(pDM_Odm, 0xed4, 0x000007ff, 0x200);
	ODM_Write4Byte(pDM_Odm, 0xce8, 0x0);
	ODM_Write4Byte(pDM_Odm, 0xee8, 0x0);
	ODM_SetBBReg(pDM_Odm, 0x82c, BIT(31), 0x0); // [31] = 0 --> Page C
	ODM_SetBBReg(pDM_Odm, 0xc10, 0x000003ff, 0x100);
	ODM_SetBBReg(pDM_Odm, 0xe10, 0x000003ff, 0x100);
}

VOID
phy_IQCalibrate_By_FW_8812A(
	IN 	PDM_ODM_T	pDM_Odm
	)
{
	
	u1Byte			IQKcmd[3] = {*pDM_Odm->pChannel, 0x0, 0x0};
	u1Byte			Buf1 = 0x0;
	u1Byte			Buf2 = 0x0;
	PODM_RF_CAL_T	pRFCalibrateInfo = &(pDM_Odm->RFCalibrateInfo);

//Byte 2, Bit 4 ~ Bit 5 : BandType
	if(*pDM_Odm->pBandType)
		Buf1 = 0x2<<4;
	else
		Buf1 = 0x1<<4;
	
//Byte 2, Bit 0 ~ Bit 3 : Bandwidth
	if(*pDM_Odm->pBandWidth == ODM_BW20M)
		Buf2 = 0x1;
	else if(*pDM_Odm->pBandWidth == ODM_BW40M)
		Buf2 = 0x1<<1;
	else if(*pDM_Odm->pBandWidth == ODM_BW80M)
		Buf2 = 0x1<<2;
	else
		Buf2 = 0x1<<3;
	
	IQKcmd[1] = Buf1 | Buf2;
	IQKcmd[2] =  pDM_Odm->ExtPA5G | pDM_Odm->ExtPA<<1 | pDM_Odm->SupportInterface<<2 | pDM_Odm->RFEType<<5;

	RT_TRACE(COMP_MP, DBG_LOUD, ("== FW IQK Start ==\n"));
	pRFCalibrateInfo->IQK_StartTime = 0;
	pRFCalibrateInfo->IQK_StartTime = ODM_GetCurrentTime(pDM_Odm);
	RT_TRACE(COMP_MP, DBG_LOUD, ("== StartTime: %u\n", pRFCalibrateInfo->IQK_StartTime));
	
	ODM_FillH2CCmd(pDM_Odm, ODM_H2C_IQ_CALIBRATION, 3, IQKcmd);


}

VOID
PHY_IQCalibrate_8812A(
	IN	PADAPTER	pAdapter,
	IN	BOOLEAN 	bReCovery
	)
{
	u4Byte			counter = 0;
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);

#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);	

	#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;	
	#else  // (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;	
	#endif
#endif	
	PODM_RF_CAL_T	pRFCalibrateInfo = &(pDM_Odm->RFCalibrateInfo);
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
	//pMgntInfo->RegIQKFWOffload = (u1Byte)ODM_GetBBReg( pDM_Odm, 0x444, BIT8);
#if MP_DRIVER == 1	
	
	// <VincentL, 131231> Add to determine IQK ON/OFF in certain case, Suggested by Cheng.
	if (!pHalData->IQK_MP_Switch)
		return;


	if( ! (pMptCtx->bSingleTone || pMptCtx->bCarrierSuppression) )
#endif		
	{

		//3 == FW IQK ==
		if(pMgntInfo->RegIQKFWOffload)
		{
			if ( ! pRFCalibrateInfo->bIQKInProgress) 
			{
				PlatformAcquireSpinLock(pAdapter, RT_IQK_SPINLOCK);
				pRFCalibrateInfo->bIQKInProgress = TRUE;
				PlatformReleaseSpinLock(pAdapter, RT_IQK_SPINLOCK);

				phy_IQCalibrate_By_FW_8812A(pDM_Odm);
			}
			else
			{
				for(counter = 0; counter < 10; counter++){
					RT_TRACE(COMP_MP, DBG_LOUD, ("== FW IQK IN PROGRESS == #%d\n", counter));
					delay_ms(50);
					if ( ! pRFCalibrateInfo->bIQKInProgress)
					{
						RT_TRACE(COMP_MP, DBG_LOUD, ("== FW IQK RETURN FROM WAITING ==\n"));
						break;
					}
				}

				if (pRFCalibrateInfo->bIQKInProgress)
				{
					RT_TRACE(COMP_MP, DBG_LOUD, ("== FW IQK TIMEOUT (Still in progress after 500ms) ==\n"));
					PlatformAcquireSpinLock(pAdapter, RT_IQK_SPINLOCK);
					pRFCalibrateInfo->bIQKInProgress = FALSE;
					PlatformReleaseSpinLock(pAdapter, RT_IQK_SPINLOCK);
				}
				else
				{
					PlatformAcquireSpinLock(pAdapter, RT_IQK_SPINLOCK);
					pRFCalibrateInfo->bIQKInProgress = TRUE;
					PlatformReleaseSpinLock(pAdapter, RT_IQK_SPINLOCK);
					phy_IQCalibrate_By_FW_8812A(pDM_Odm);
				}
				
				
			}
		}
		//3 == Driver IQK ==
		else 
		{
			if ( ! pRFCalibrateInfo->bIQKInProgress) 
			{
				PlatformAcquireSpinLock(pAdapter, RT_IQK_SPINLOCK);
				pRFCalibrateInfo->bIQKInProgress = TRUE;
				PlatformReleaseSpinLock(pAdapter, RT_IQK_SPINLOCK);
				
				//RT_TRACE(COMP_P2P, DBG_LOUD, ("1 %s: KeGetCurrentIrql(): %d\n", __FUNCTION__, KeGetCurrentIrql()));
				phy_IQCalibrate_8812A(pDM_Odm, pHalData->CurrentChannel);
				//RT_TRACE(COMP_P2P, DBG_LOUD, ("3 %s: KeGetCurrentIrql(): %d\n", __FUNCTION__, KeGetCurrentIrql()));

				PlatformAcquireSpinLock(pAdapter, RT_IQK_SPINLOCK);
				pRFCalibrateInfo->bIQKInProgress = FALSE;
				PlatformReleaseSpinLock(pAdapter, RT_IQK_SPINLOCK);
			}
		}

		
	}
	
}


VOID
PHY_LCCalibrate_8812A(
	IN PDM_ODM_T		pDM_Odm
	)
{
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	PADAPTER 		pAdapter = pDM_Odm->Adapter;
	
#if (MP_DRIVER == 1)
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)	
	PMPT_CONTEXT	pMptCtx = &(pAdapter->MptCtx);
#else
	PMPT_CONTEXT	pMptCtx = &(pAdapter->mppriv.MptCtx);		
#endif	
#endif
#endif	

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("===> PHY_LCCalibrate_8812A\n"));

#if (MP_DRIVER == 1)	
	phy_LCCalibrate_8812A(pDM_Odm, TRUE);
#endif 

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("<=== PHY_LCCalibrate_8812A\n"));

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

#endif		

	if (IS_HARDWARE_TYPE_8821(pAdapter)) 
	{
		if(bMain)
			ODM_SetBBReg(pDM_Odm, rA_RFE_Pinmux_Jaguar+4, BIT29|BIT28, 0x1);	//Main
		else
			ODM_SetBBReg(pDM_Odm, rA_RFE_Pinmux_Jaguar+4, BIT29|BIT28, 0x2);	//Aux
	}
	else if (IS_HARDWARE_TYPE_8812(pAdapter)) 
	{
		if (pHalData->RFEType == 5)
		{
			if(bMain) {
				// WiFi 
				ODM_SetBBReg(pDM_Odm, r_ANTSEL_SW_Jaguar, BIT1|BIT0, 0x2);  
            	ODM_SetBBReg(pDM_Odm, r_ANTSEL_SW_Jaguar, BIT9|BIT8, 0x3);  
			} else {
				// BT
				ODM_SetBBReg(pDM_Odm, r_ANTSEL_SW_Jaguar, BIT1|BIT0, 0x1);  
            	ODM_SetBBReg(pDM_Odm, r_ANTSEL_SW_Jaguar, BIT9|BIT8, 0x3);  
			}
		}
		else if (pHalData->RFEType == 1)
		{ // <131224, VincentL> When RFEType == 1 also Set 0x900, suggested by RF Binson.
			if(bMain) {
				// WiFi 
				ODM_SetBBReg(pDM_Odm, r_ANTSEL_SW_Jaguar, BIT1|BIT0, 0x2);  
            	ODM_SetBBReg(pDM_Odm, r_ANTSEL_SW_Jaguar, BIT9|BIT8, 0x3);  
			} else {
				// BT
				ODM_SetBBReg(pDM_Odm, r_ANTSEL_SW_Jaguar, BIT1|BIT0, 0x1);  
            	ODM_SetBBReg(pDM_Odm, r_ANTSEL_SW_Jaguar, BIT9|BIT8, 0x3);  
			}
		}
		
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

#if DISABLE_BB_RF
	return;
#endif

#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)

		phy_SetRFPathSwitch_8812A(pAdapter, bMain, TRUE);

#endif		
}

s4Byte
_Sign(
	IN u4Byte Number
	)
{
	if ((Number & BIT10) == BIT10) {// Negative
	    Number &= (~0xFFFFFC00);         // [9:0]
		Number = ~Number;
	    Number &= (~0xFFFFFC00);         // [9:0]
	    Number += 1;
	    Number &= (~0xFFFFF800);		  // [10:0]	
	    return -1 * Number;
    } else { // Positive
        return (s4Byte)Number;
    }
}


VOID
_DPK_MacBBBackup_PathA(
	IN 		PDM_ODM_T	pDM_Odm,
	IN  	pu4Byte		BackupRegAddr,
	IN OUT  pu4Byte		BackupRegData, 
	IN  	u4Byte		Number
	)
{
	u4Byte i;
	
	for (i = 0; i < Number; i++)
		BackupRegData[i] = ODM_Read4Byte(pDM_Odm, BackupRegAddr[i]);
}
VOID
_DPK_MacBBRestore_PathA(
	IN 	PDM_ODM_T	pDM_Odm,
	IN  pu4Byte		BackupRegAddr,
	IN  pu4Byte		BackupRegData, 
	IN  u4Byte		Number
	)
{
	u4Byte i;
	
	for (i = 0; i < Number; i++)
		ODM_Write4Byte(pDM_Odm, BackupRegAddr[i], BackupRegData[i]);
}


VOID
_DPK_RFBackup_PathA(
	IN 		PDM_ODM_T	        pDM_Odm,
	IN  	pu4Byte				BackupRegAddr,
	IN		ODM_RF_RADIO_PATH_E	RFPath,
	IN OUT  pu4Byte				BackupRegData, 
	IN  	u4Byte		        Number
	)
{
	u4Byte i;
	
	for (i = 0; i < Number; i++)
		BackupRegData[i] = ODM_GetRFReg(pDM_Odm, RFPath, BackupRegAddr[i], bRFRegOffsetMask);
}

VOID
_DPK_RFRestore_PathA(
	IN 	PDM_ODM_T	        pDM_Odm,
	IN  pu4Byte				BackupRegAddr,
	IN	ODM_RF_RADIO_PATH_E	RFPath,
	IN  pu4Byte		        BackupRegData, 
	IN  u4Byte		        Number
	)
{
	u4Byte i;
	
	for (i = 0; i < Number; i++)
		ODM_SetRFReg(pDM_Odm, RFPath, BackupRegAddr[i], bRFRegOffsetMask, BackupRegData[i]);
}



s4Byte
_ComputeLoopBackGain_PathA(
	IN	PDM_ODM_T			pDM_Odm
)
{
	// compute loopback gain
	// 計算tmpLBGain 用到的function hex2dec() 是將 twos complement的十六進位轉成十進位, 十六進位的格式為s11.10 
    // 舉例1, d00[10:0] = h'3ff, 經過 hex2dec(h'3ff) = 1023,
    // 舉例2, d00[10:0] = h'400, 經過 hex2dec(h'400) = -1024,
    // 舉例3, d00[10:0] = h'0A9, 經過 hex2dec(h'0A9) = 169,
    // 舉例4, d00[10:0] = h'54c, 經過 hex2dec(h'54c) = -692,
    // 舉例5, d00[10:0] = h'7ff, 經過 hex2dec(h'7ff) = -1,
    u4Byte reg0xD00_26_16 = ODM_GetBBReg(pDM_Odm, 0xD00, 0x7FF0000);
    u4Byte reg0xD00_10_0  = ODM_GetBBReg(pDM_Odm, 0xD00, 0x7FF);
	
    s4Byte tmpLBGain = _Sign(reg0xD00_26_16)*_Sign(reg0xD00_26_16) + _Sign(reg0xD00_10_0)*_Sign(reg0xD00_10_0);

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("<=== _ComputeLoopBackGain_PathA, tmpLBGain = 0x%X\n", tmpLBGain));

	return tmpLBGain;
}


BOOLEAN
_FineTuneLoopBackGain_PathA(
	IN		PDM_ODM_T			pDM_Odm, 
	IN		u4Byte 				DpkTxAGC	
	)
{
	PODM_RF_CAL_T  pRFCalibrateInfo = &(pDM_Odm->RFCalibrateInfo);	
	s4Byte tmpLBGain = 0;

    u4Byte rf0x64_orig = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x64, 0x7);
    u4Byte rf0x64_new = rf0x64_orig;

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("===> _FineTuneLoopBackGain\n"));

    do {
        // RF setting
        ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x00, bRFRegOffsetMask, DpkTxAGC);
		ODM_Write4Byte(pDM_Odm, 0x82c, 0x802083dd);
        // 注意page c1的c90在DPK過程中千萬不能被其他thread or process改寫成page c的c90值
        ODM_Write4Byte(pDM_Odm, 0xc90, 0x0101f018);
        
        // one shot
        ODM_Write4Byte(pDM_Odm, 0xcc8, 0x800c5599);
        ODM_Write4Byte(pDM_Odm, 0xcc8, 0x000c5599);
        // delay 50 ms,讓 delay 時間長一點, 確定 PA Scan function 有做完
        delay_ms(50);
        
        // reg82c[31] = 0 --> 切到 page C
        ODM_Write4Byte(pDM_Odm, 0x82c, 0x002083dd);
        ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x00, bRFRegOffsetMask, 0x33d8d);	

		tmpLBGain = _ComputeLoopBackGain_PathA(pDM_Odm);
		
	    if ((rf0x64_new == (BIT2|BIT1|BIT0)) || (rf0x64_new == 0)) {
	        //printf('DPK Phase 1 failed');
	        pRFCalibrateInfo->bDPKFail = TRUE;
			break;
	        //Go to DPK Phase 5
	    } else {
	        if (tmpLBGain < 263390) {// fine tune loopback path gain: newReg64[2:0] = reg64[2:0] - 3b'001
	        	rf0x64_new -= 1;
	            ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x64, 0x7, rf0x64_new);

	        } else if (tmpLBGain > 661607) {// fine tune loopback path gain: newReg64 [2:0] = reg64[2:0] + 3b'001
	        	rf0x64_new += 1;
	            ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x64, 0x7, rf0x64_new);

	        } else {
				pRFCalibrateInfo->bDPKFail = FALSE;
	        }
	    }

	} while (tmpLBGain < 263390 || 661607 < tmpLBGain);

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("<=== _FineTuneLoopBackGain\n"));

	return pRFCalibrateInfo->bDPKFail ? FALSE : TRUE;
            
}



VOID
_DPK_Init_PathA(
	IN 	PDM_ODM_T	pDM_Odm	
	)
{
	PODM_RF_CAL_T  pRFCalibrateInfo = &(pDM_Odm->RFCalibrateInfo);

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("===> _DPK_Init\n"));
	
	//TX pause
	ODM_Write1Byte(pDM_Odm, 0x522, 0x7f); 
	
	// reg82c[31] = b'0, 切換到 page C
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x002083dd);
	
	// AFE setting
	ODM_Write4Byte(pDM_Odm, 0xc68, 0x19791979);
	ODM_Write4Byte(pDM_Odm, 0xc60, 0x77777777);
	ODM_Write4Byte(pDM_Odm, 0xc64, 0x77777777);
	
	// external TRSW 切到 T
	ODM_Write4Byte(pDM_Odm, 0xcb0, 0x77777777);
	ODM_Write4Byte(pDM_Odm, 0xcb4, 0x01000077);
	
	// hardware 3-wire off
	ODM_Write4Byte(pDM_Odm, 0xc00, 0x00000004);
	
	// CCA off
	ODM_Write4Byte(pDM_Odm, 0x838, 0x16C89B4c);
	 
	//90c[15]: dac fifo reset by CSWU
	ODM_Write4Byte(pDM_Odm, 0x90c, 0x00008000);
	
	// r_gothrough_iqkdpk
	ODM_Write4Byte(pDM_Odm, 0xc94, 0x0100005D);
	
	// reg82c[31] = 1 --> 切到 page C1
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x802083dd);
	
	// IQK Amp off
	ODM_Write4Byte(pDM_Odm, 0xc8c, 0x3c000000);
	
	// tx_amp
	ODM_Write4Byte(pDM_Odm, 0xc98, 0x41382e21);
	ODM_Write4Byte(pDM_Odm, 0xc9c, 0x5b554f48);
	ODM_Write4Byte(pDM_Odm, 0xca0, 0x6f6b6661);
	ODM_Write4Byte(pDM_Odm, 0xca4, 0x817d7874);
	ODM_Write4Byte(pDM_Odm, 0xca8, 0x908c8884);
	ODM_Write4Byte(pDM_Odm, 0xcac, 0x9d9a9793);
	ODM_Write4Byte(pDM_Odm, 0xcb0, 0xaaa7a4a1);
	ODM_Write4Byte(pDM_Odm, 0xcb4, 0xb6b3b0ad);
	
	// tx_inverse
	ODM_Write4Byte(pDM_Odm, 0xc40, 0x02ce03e9);
	ODM_Write4Byte(pDM_Odm, 0xc44, 0x01fd0249);
	ODM_Write4Byte(pDM_Odm, 0xc48, 0x01a101c9);
	ODM_Write4Byte(pDM_Odm, 0xc4c, 0x016a0181);
	ODM_Write4Byte(pDM_Odm, 0xc50, 0x01430155);
	ODM_Write4Byte(pDM_Odm, 0xc54, 0x01270135);
	ODM_Write4Byte(pDM_Odm, 0xc58, 0x0112011c);
	ODM_Write4Byte(pDM_Odm, 0xc5c, 0x01000108);
	ODM_Write4Byte(pDM_Odm, 0xc60, 0x00f100f8);
	ODM_Write4Byte(pDM_Odm, 0xc64, 0x00e500eb);
	ODM_Write4Byte(pDM_Odm, 0xc68, 0x00db00e0);
	ODM_Write4Byte(pDM_Odm, 0xc6c, 0x00d100d5);
	ODM_Write4Byte(pDM_Odm, 0xc70, 0x00c900cd);
	ODM_Write4Byte(pDM_Odm, 0xc74, 0x00c200c5);
	ODM_Write4Byte(pDM_Odm, 0xc78, 0x00bb00be);
	ODM_Write4Byte(pDM_Odm, 0xc7c, 0x00b500b8);
	
	// reg82c[31] = b'0, 切換到 page C
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x002083dd); 
	
	pRFCalibrateInfo->bDPKFail = FALSE;

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("<=== _DPK_Init\n"));

}


BOOLEAN
_DPK_AdjustRFGain_PathA(
	IN 	PDM_ODM_T	pDM_Odm
	)
{
	PODM_RF_CAL_T  pRFCalibrateInfo = &(pDM_Odm->RFCalibrateInfo);	
	s4Byte tmpLBGain = 0;

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("===> _DPK_AdjustRFGain\n"));
	
	// RF setting
    ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x00, bRFRegOffsetMask, 0x50bfc);
    // Attn: A mode @ reg64[2:0], G mode @ reg56
    ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x64, bRFRegOffsetMask, 0x19aac); 
    // PGA gain: RF reg8f[14:13]
    ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x8f, bRFRegOffsetMask, 0x8a001); 
    
    // reg82c[31] = 1 --> 切到 page C1
    ODM_Write4Byte(pDM_Odm, 0x82c, 0x802083dd);
    
    // DPK setting
    ODM_Write4Byte(pDM_Odm, 0xc94, 0xf76c9f84);
    ODM_Write4Byte(pDM_Odm, 0xcc8, 0x000c5599);
    ODM_Write4Byte(pDM_Odm, 0xcc4, 0x11838000);
    ODM_SetBBReg(pDM_Odm, 0xcd4, 0xFFF000, 0x100);  // 將cd4[23:12] 改成 h'100, 其它位元請保留原值不要寫到
    // 注意page c1的c90在DPK過程中千萬不能被其他thread or process改寫成page c的c90值
    ODM_Write4Byte(pDM_Odm, 0xc90, 0x0101f018);
    
    // one shot
    ODM_Write4Byte(pDM_Odm, 0xcc8, 0x800c5599);
    ODM_Write4Byte(pDM_Odm, 0xcc8, 0x000c5599);
    // delay 50 ms,讓 delay 時間長一點, 確定 PA Scan function 有做完
    delay_ms(50);
    
    // read back Loopback Gain
    ODM_Write4Byte(pDM_Odm, 0xcb8, 0x09000000);

    // reg82c[31] = 0 --> 切到 page C
    ODM_Write4Byte(pDM_Odm, 0x82c, 0x002083dd);
    ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x00, bRFRegOffsetMask, 0x33d8d);
    
    tmpLBGain = _ComputeLoopBackGain_PathA(pDM_Odm);
    
    // coarse tune loopback gain by RF reg8f[14:13] = 2b'11
    if (tmpLBGain < 263390) 
    {
        ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x8f, bRFRegOffsetMask, 0x8e001);
        _FineTuneLoopBackGain_PathA(pDM_Odm, 0x50bfc);

    } 
    else if (tmpLBGain > 661607)
    {
        // coarse tune loopback gain by RF reg8f[14:13] = 2b'00
    	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x8f, bRFRegOffsetMask, 0x88001);
		_FineTuneLoopBackGain_PathA(pDM_Odm, 0x50bfc);
    }
    else
    	pRFCalibrateInfo->bDPKFail = FALSE;

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("<=== _DPK_AdjustRFGain\n"));

	return pRFCalibrateInfo->bDPKFail ? FALSE : TRUE;

}



VOID
_DPK_GainLossToFindTxAGC_PathA(
	IN 	PDM_ODM_T	pDM_Odm	
	)
{
	PODM_RF_CAL_T  pRFCalibrateInfo = &(pDM_Odm->RFCalibrateInfo);
	u4Byte Reg0xD00 = 0;

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("===> _DPK_GainLossToFindTxAGC\n"));
	
	// RF setting
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x00, bRFRegOffsetMask, 0x50bfc);
	
	// reg82c[31] = 1 --> 切到 page C1
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x802083dd); 
	
	// regc20[15:13] = dB sel, 告訴 Gain Loss function 去尋找 dB_sel 所設定的PA gain loss目標所對應的 Tx AGC 為何.
	// dB_sel = b'000 ' 1.0 dB PA gain loss
	// dB_sel = b'001 ' 1.5 dB PA gain loss
	// dB_sel = b'010 ' 2.0 dB PA gain loss
	// dB_sel = b'011 ' 2.5 dB PA gain loss
	// dB_sel = b'100 ' 3.0 dB PA gain loss
	// dB_sel = b'101 ' 3.5 dB PA gain loss
	// dB_sel = b'110 ' 4.0 dB PA gain loss
	ODM_Write4Byte(pDM_Odm, 0xc20, 0x00002000);
	
	// DPK setting
	ODM_Write4Byte(pDM_Odm, 0xc94, 0xf76c9f84);
	ODM_Write4Byte(pDM_Odm, 0xcc8, 0x000c5599);
	ODM_Write4Byte(pDM_Odm, 0xcc4, 0x148b8000);
	
	// 注意page c1的c90在DPK過程中千萬不能被其他thread or process改寫成page c的c90值
	ODM_Write4Byte(pDM_Odm, 0xc90, 0x0401f018);
	
	// one shot
	ODM_Write4Byte(pDM_Odm, 0xcc8, 0x800c5599);
	ODM_Write4Byte(pDM_Odm, 0xcc8, 0x000c5599);
	// delay 50 ms,讓 delay 時間長一點, 確定 PA Scan function 有做完
	delay_ms(50);
	
	// read back Loopback Gain
	// 可以在 d00[3:0] 中讀回, dB_sel 中所設定的 gain loss 會落在哪一個 Tx AGC 設定
	// 讀回d00[3:0] = h'1 ' Tx AGC = h'13
	// 讀回d00[3:0] = h'2 ' Tx AGC = h'14
	// 讀回d00[3:0] = h'3 ' Tx AGC = h'15
	// 讀回d00[3:0] = h'4 ' Tx AGC = h'16
	// 讀回d00[3:0] = h'5 ' Tx AGC = h'17
	// 讀回d00[3:0] = h'6 ' Tx AGC = h'18
	// 讀回d00[3:0] = h'7 ' Tx AGC = h'19
	// 讀回d00[3:0] = h'8 ' Tx AGC = h'1a
	// 讀回d00[3:0] = h'9 ' Tx AGC = h'1b
	// 讀回d00[3:0] = h'a ' Tx AGC = h'1c
	//
	Reg0xD00 = ODM_Read4Byte(pDM_Odm, 0xd00);
	switch (ODM_Read4Byte(pDM_Odm, 0xd00) & (BIT3|BIT2|BIT1|BIT0))
	{
		case 0x0:	pRFCalibrateInfo->DpkTxAGC = 0x50bf0 | 0x2; break;
		case 0x1:	pRFCalibrateInfo->DpkTxAGC = 0x50bf0 | 0x3; break;
		case 0x2:	pRFCalibrateInfo->DpkTxAGC = 0x50bf0 | 0x4; break;
		case 0x3:	pRFCalibrateInfo->DpkTxAGC = 0x50bf0 | 0x5; break;
		case 0x4:	pRFCalibrateInfo->DpkTxAGC = 0x50bf0 | 0x6; break;
		case 0x5:	pRFCalibrateInfo->DpkTxAGC = 0x50bf0 | 0x7; break;
		case 0x6:	pRFCalibrateInfo->DpkTxAGC = 0x50bf0 | 0x8; break;
		case 0x7:	pRFCalibrateInfo->DpkTxAGC = 0x50bf0 | 0x9; break;
		case 0x8:	pRFCalibrateInfo->DpkTxAGC = 0x50bf0 | 0xa; break;
		case 0x9:	pRFCalibrateInfo->DpkTxAGC = 0x50bf0 | 0xb; break;
		case 0xa:	pRFCalibrateInfo->DpkTxAGC = 0x50bf0 | 0xc; break;
	}
	// reg82c[31] = 0 --> 切到 page C
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x002083dd);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x00, bRFRegOffsetMask, 0x33d8d);

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("<=== _DPK_GainLossToFindTxAGC\n"));
}


BOOLEAN
_DPK_AdjustRFGainByFoundTxAGC_PathA
(
	IN 	PDM_ODM_T	pDM_Odm
	)
{
	PODM_RF_CAL_T  pRFCalibrateInfo = &(pDM_Odm->RFCalibrateInfo);	
	s4Byte tmpLBGain = 0;

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("===> _DPK_AdjustRFGainByFoundTxAGC\n"));
	
	// RF setting, 重新設定RF reg00, 舉例用DPK Phase 2得到的 d00[3:0] = 0x6 ' TX AGC= 0x18 ' RF reg00[4:0] = 0x18
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x00, bRFRegOffsetMask, pRFCalibrateInfo->DpkTxAGC);
	// Attn: A mode @ reg64[2:0], G mode @ reg56
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x64, bRFRegOffsetMask, 0x19aac); 
	// PGA gain: RF reg8f[14:13]
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x8f, bRFRegOffsetMask, 0x8a001); 
	
	// reg82c[31] = 1 --> 切到 page C1
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x802083dd); 
	
	// DPK setting
	ODM_Write4Byte(pDM_Odm, 0xc94, 0xf76c9f84);
	ODM_Write4Byte(pDM_Odm, 0xcc8, 0x000c5599);
	ODM_Write4Byte(pDM_Odm, 0xcc4, 0x11838000);
	
	// 注意page c1的c90在DPK過程中千萬不能被其他thread or process改寫成page c的c90值
	ODM_Write4Byte(pDM_Odm, 0xc90, 0x0101f018);
	
	// one shot
	ODM_Write4Byte(pDM_Odm, 0xcc8, 0x800c5599);
	ODM_Write4Byte(pDM_Odm, 0xcc8, 0x000c5599);
	// delay 50 ms,讓 delay 時間長一點, 確定 PA Scan function 有做完
	delay_ms(50);
	
	// reg82c[31] = 0 --> 切到 page C
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x002083dd);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x00, bRFRegOffsetMask, 0x33d8d);
	

	tmpLBGain = _ComputeLoopBackGain_PathA(pDM_Odm);
	
	if (tmpLBGain < 263390)
	{
		// coarse tune loopback gain by RF reg8f[14:13] = 2b'11
		ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x8f, bRFRegOffsetMask, 0x8e001);
		_FineTuneLoopBackGain_PathA(pDM_Odm, pRFCalibrateInfo->DpkTxAGC);
	} 
	else if (tmpLBGain > 661607) 
	{
		// coarse tune loopback gain by RF reg8f[14:13] = 2b'00
		ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x8f, bRFRegOffsetMask, 0x88001);
		_FineTuneLoopBackGain_PathA(pDM_Odm, pRFCalibrateInfo->DpkTxAGC);
	} 
	else
		pRFCalibrateInfo->bDPKFail = FALSE;//Go to DPK Phase 4

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("<=== _DPK_AdjustRFGainByFoundTxAGC\n"));

	return pRFCalibrateInfo->bDPKFail ? FALSE : TRUE;

}


VOID
_DPK_DoAutoDPK_PathA(
	IN 	PDM_ODM_T	pDM_Odm	
	)
{	
	PODM_RF_CAL_T  pRFCalibrateInfo = &(pDM_Odm->RFCalibrateInfo);
	u4Byte tmpLBGain = 0, reg0xD00 = 0;
	
 	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("===> _DPK_DoAutoDPK\n"));
	
	
	// reg82c[31] = 0 --> 切到 page C
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x002083dd);
	// RF setting, 此處RF reg00, 與 DPK Phase 3 一致
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x00, bRFRegOffsetMask, pRFCalibrateInfo->DpkTxAGC);
	// Baseband Data Rate setting
	ODM_Write4Byte(pDM_Odm, 0xc20, 0x3c3c3c3c);
	ODM_Write4Byte(pDM_Odm, 0xc24, 0x3c3c3c3c);
	ODM_Write4Byte(pDM_Odm, 0xc28, 0x3c3c3c3c);
	ODM_Write4Byte(pDM_Odm, 0xc2c, 0x3c3c3c3c);
	ODM_Write4Byte(pDM_Odm, 0xc30, 0x3c3c3c3c);
	ODM_Write4Byte(pDM_Odm, 0xc34, 0x3c3c3c3c);
	ODM_Write4Byte(pDM_Odm, 0xc38, 0x3c3c3c3c);
	ODM_Write4Byte(pDM_Odm, 0xc3c, 0x3c3c3c3c);
	ODM_Write4Byte(pDM_Odm, 0xc40, 0x3c3c3c3c);
	ODM_Write4Byte(pDM_Odm, 0xc44, 0x3c3c3c3c);
	ODM_Write4Byte(pDM_Odm, 0xc48, 0x3c3c3c3c);
	ODM_Write4Byte(pDM_Odm, 0xc4c, 0x3c3c3c3c);

	// reg82c[31] = 1 --> 切到 page C1
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x802083dd); 

	// DPK setting
	ODM_Write4Byte(pDM_Odm, 0xc94, 0xf76C9f84);
	ODM_Write4Byte(pDM_Odm, 0xcc8, 0x400C5599);
	ODM_Write4Byte(pDM_Odm, 0xcc4, 0x11938080);    //0xcc4[9:4]= DPk fail threshold

	if ( 36 <= *(pDM_Odm->pChannel) && *(pDM_Odm->pChannel) <= 53)//Channelchannel at low band)
		// r_agc
		ODM_Write4Byte(pDM_Odm, 0xcbc, 0x00022a1f);
	else
		// r_agc
		ODM_Write4Byte(pDM_Odm, 0xcbc, 0x00009dbf);

	// 注意page c1的c90在DPK過程中千萬不能被其他thread or process改寫成page c的c90值
	ODM_Write4Byte(pDM_Odm, 0xc90, 0x0101f018); // TODO: 0xC90(rA_LSSIWrite_Jaguar) can not be overwritten.

	// one shot
	ODM_Write4Byte(pDM_Odm, 0xcc8, 0xc00c5599);
	ODM_Write4Byte(pDM_Odm, 0xcc8, 0x400c5599);
	// delay 50 ms,讓 delay 時間長一點, 確定 PA Scan function 有做完
	delay_ms(50);

	// reg82c[31] = 0 --> 切到 page C
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x002083dd);
	// T-meter RFReg42[17] = 1 to enable read T-meter, [15:10] ' T-meter value
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x42, BIT17, 1);
	pRFCalibrateInfo->DpkThermal[ODM_RF_PATH_A] = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x42, 0xFC00);					// 讀出42[15:10] 值並存到變數TMeter
	DbgPrint("pRFCalibrateInfo->DpkThermal[ODM_RF_PATH_A] = 0x%X\n", pRFCalibrateInfo->DpkThermal[ODM_RF_PATH_A]);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x00, bRFRegOffsetMask, 0x33D8D);

	// reg82c[31] = 1 --> 切到 page C1
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x802083dd);
	// read back dp_fail report
	ODM_Write4Byte(pDM_Odm, 0xcb8, 0x00000000);
	//dp_fail這個bit在d00[6], 當 d00[6] = 1, 表示calibration失敗.
	reg0xD00 = ODM_Read4Byte(pDM_Odm, 0xd00);

	if ((reg0xD00 & BIT6) == BIT6) {
		//printf('DPK fail')
		pRFCalibrateInfo->bDPKFail = TRUE;
		//Go to DPK Phase 5
	} else {
		//printf('DPK success')
	}

	
	// read back
	ODM_Write4Byte(pDM_Odm, 0xc90, 0x0201f01f);
	ODM_Write4Byte(pDM_Odm, 0xcb8, 0x0c000000);

	// reg82c[31] = 1 --> 切到 page C1
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x002083dd);

	// 計算tmpGainLoss 用到的function hex2dec() 是將 twos complement的十六進位轉成十進位, 十六進位的格式為s11.9 
	// 舉例1, d00[10:0] = h'3ff, 經過 hex2dec(h'3ff) = 1023,
	// 舉例2, d00[10:0] = h'400, 經過 hex2dec(h'400) = -1024,
	// 舉例3, d00[10:0] = h'0A9, 經過 hex2dec(h'0A9) = 169,
	// 舉例4, d00[10:0] = h'54c, 經過 hex2dec(h'54c) = -692,
	// 舉例4, d00[10:0] = h'7ff, 經過 hex2dec(h'7ff) = -1,
	tmpLBGain = _ComputeLoopBackGain_PathA(pDM_Odm);


	// Gain Scaling
    if (227007 < tmpLBGain) 
	    pRFCalibrateInfo->DpkGain = 0x43ca43ca;
    else if (214309 < tmpLBGain && tmpLBGain <= 227007) 
	    pRFCalibrateInfo->DpkGain = 0x45c545c5;
    else if (202321 < tmpLBGain && tmpLBGain <= 214309) 
	    pRFCalibrateInfo->DpkGain = 0x47cf47cf;
    else if (191003 < tmpLBGain && tmpLBGain <= 202321) 
	    pRFCalibrateInfo->DpkGain = 0x49e749e7;
    else if (180318 < tmpLBGain && tmpLBGain <= 191003) 
	    pRFCalibrateInfo->DpkGain = 0x4c104c10;
    else if (170231 < tmpLBGain && tmpLBGain <= 180318) 
	    pRFCalibrateInfo->DpkGain = 0x4e494e49;
    else if (160709 < tmpLBGain && tmpLBGain <= 170231) 
	    pRFCalibrateInfo->DpkGain = 0x50925092;
    else if (151719 < tmpLBGain && tmpLBGain <= 160709) 
	    pRFCalibrateInfo->DpkGain = 0x52ec52ec;
    else if (151719 <= tmpLBGain) 
	    pRFCalibrateInfo->DpkGain = 0x55585558;

 	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("<=== _DPK_DoAutoDPK\n"));	
}


VOID	
_DPK_EnableDP_PathA(
	IN 	PDM_ODM_T	pDM_Odm
	)
{
	PODM_RF_CAL_T  pRFCalibrateInfo = &(pDM_Odm->RFCalibrateInfo);

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("===> _DPK_EnableDP\n"));

	// [31] = 1 --> switch to page C1
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x802083dd);

	// enable IQC matrix --> 因為 BB 信號會先經過 predistortion module, 才經過 IQC matrix 到 DAC 打出去
	// 所以要先 enable predistortion module {c90[7] = 1 (enable_predis) cc4[18] = 1 (確定讓 IQK/DPK module 有clock), cc8[29] = 1 (一個IQK/DPK module 裡的mux, 確認 data path 走IQK/DPK)}
	ODM_Write4Byte(pDM_Odm, 0xc90, 0x0000f098);
	ODM_Write4Byte(pDM_Odm, 0xc94, 0x776d9f84);
	ODM_Write4Byte(pDM_Odm, 0xcc8, 0x20000000);
	ODM_Write4Byte(pDM_Odm, 0xc8c, 0x3c000000);
	// r_bnd
	ODM_Write4Byte(pDM_Odm, 0xcb8, 0x000fffff);
	
	if (pRFCalibrateInfo->bDPKFail)
	{
		// cc4[29] = 1 (bypass DP LUT)
		ODM_Write4Byte(pDM_Odm, 0xc98, 0x40004000);
		ODM_Write4Byte(pDM_Odm, 0xcc4, 0x28840000);
	}
	else
	{
		ODM_Write4Byte(pDM_Odm, 0xc98, pRFCalibrateInfo->DpkGain);
		ODM_Write4Byte(pDM_Odm, 0xcc4, 0x08840000);

		// PWSF
		// 寫PWSF table in 1st SRAM for PA = 11 use
		ODM_Write4Byte(pDM_Odm, 0xc20, 0x00000800);

		//*******************************************************
		//0xce4[0]是write enable，0xce4[7:1]是address，0xce4[15:8]和0xce4[23:16]對應到TX index，
		//若位置為0(0xce4[7:1] = 0x0)此時0xce4[15:8]對應到的TX RF index 是0x1f,，
		//0xce4[23:16]對應到的是0x1e，若位置為1(0xce4[7:1] = 0x1)，此時0xce4 [15:8]對應到0x1d，
		//0xce4[23:16]對應0x1c，其他依此類推，每個data欄位都是相差1dB。若gainloss找到的RF TX index=0x18，
		//則在0xce4 address對應到0x18的地方要填0x40(也就是0dB)，其他則照下面table的順序每格1dB依序排列
		//將所有的0xce4c的data欄位都填入相對應的值。
		//*************************************************************//

		{
			const s4Byte LEN = 25;
		    u4Byte baseIdx = 6; // 0 dB: 0x40			
		    u4Byte TablePWSF[] = {
		        0xff, 0xca, 0xa1, 0x80, 0x65, 0x51, 0x40/* 0 dB */, 
		        0x33, 0x28, 0x20, 0x19, 0x14, 0x10, 
		        0x0d, 0x0a, 0x08, 0x06, 0x05, 0x04, 
		        0x03, 0x03, 0x02, 0x02, 0x01, 0x01
		    };

		    u4Byte centerTxIdx = pRFCalibrateInfo->DpkTxAGC & 0x1F;
		    u4Byte centerAddr = (0x1F-centerTxIdx) / 2;
		    s4Byte i = 0, j = 0, value = 0, startIdx = 0;

			// Upper
		    startIdx = (((0x1F-centerTxIdx)%2 == 0) ? baseIdx+1 : baseIdx);
			
		    for (i = startIdx, j = 0; (centerAddr-j+1) >= 1; i -= 2, j++) {
		        if (i-1 < 0) 
		            value = (TablePWSF[0] << 16) | (TablePWSF[0] << 8) | ((centerAddr-j) << 1) | 0x1;
		        else 
		            value = (TablePWSF[i] << 16) | (TablePWSF[i-1] << 8) | ((centerAddr-j) << 1) | 0x1;
		        
				ODM_Write4Byte(pDM_Odm, 0xce4, value);
				ODM_Write1Byte(pDM_Odm, 0xce4, 0x0);		//write disable
		    }

		    // Lower
		    startIdx = (((0x1F-centerTxIdx)%2 == 0) ? baseIdx+2 : baseIdx+1);
		    centerAddr++; // Skip centerTxIdx
		    for (i = startIdx, j = 0; (centerAddr+j) < 16; i += 2, j++) { // Total: 16*2 = 32 values (Upper+Lower)
		        if (i+1 >= LEN) 
		            value = (TablePWSF[LEN-1] << 16) | (TablePWSF[LEN-1] << 8) | ((centerAddr+j) << 1) | 0x1;
		        else
		            value = (TablePWSF[i+1] << 16) | (TablePWSF[i] << 8) | ((centerAddr+j) << 1) | 0x1;
				ODM_Write4Byte(pDM_Odm, 0xce4, value);
				ODM_Write1Byte(pDM_Odm, 0xce4, 0x0);		//write disable
		    }
		}
	}
	// [31] = 0 --> switch to page C
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x002083dd); 

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("<=== _DPK_EnableDP\n"));

}





VOID	
phy_DPCalibrate_PathA_8812A(
	IN 	PDM_ODM_T	pDM_Odm
	)
{	
    u4Byte backupMacBBRegAddrs[] = {
        0xC60, 0xC64, 0xC68, 0x82C, 0x838, 0x90C, 0x522, 0xC00, 0xC20, 0xC24, // 10
	    0xC28, 0xC2C, 0xC30, 0xC34, 0xC38, 0xC3C, 0xC40, 0xC44, 0xC48, 0xC4C, // 20
	    0xC94, 0xCB0, 0xCB4
	};	
    u4Byte backupRFRegAddrs[] = {0x00, 0x64, 0x8F};

    u4Byte backupMacBBRegData[sizeof(backupMacBBRegAddrs)/sizeof(u4Byte)];
    u4Byte backupRFRegData_A[sizeof(backupRFRegAddrs)/sizeof(u4Byte)];
    //u4Byte backupRFRegData_B[sizeof(backupRFRegAddrs)/sizeof(u4Byte)];	


	_DPK_MacBBBackup_PathA(pDM_Odm, backupMacBBRegAddrs, backupMacBBRegData, sizeof(backupMacBBRegAddrs)/sizeof(u4Byte));
	_DPK_RFBackup_PathA(pDM_Odm, backupRFRegAddrs, ODM_RF_PATH_A, backupRFRegData_A, sizeof(backupRFRegAddrs)/sizeof(u4Byte));

	_DPK_Init_PathA(pDM_Odm);

	if (_DPK_AdjustRFGain_PathA(pDM_Odm)) {               // Phase 1
	    _DPK_GainLossToFindTxAGC_PathA(pDM_Odm);          // Phase 2
	    if (_DPK_AdjustRFGainByFoundTxAGC_PathA(pDM_Odm)) // Phase 3
	        _DPK_DoAutoDPK_PathA(pDM_Odm);                // Phase 4
	}
    _DPK_EnableDP_PathA(pDM_Odm);                         // Phase 5

	_DPK_MacBBRestore_PathA(pDM_Odm, backupMacBBRegAddrs, backupMacBBRegData, sizeof(backupMacBBRegAddrs)/sizeof(u4Byte));
	_DPK_RFRestore_PathA(pDM_Odm, backupRFRegAddrs, ODM_RF_PATH_A, backupRFRegData_A, sizeof(backupRFRegAddrs)/sizeof(u4Byte));	
}                                                        

														 
VOID
_DPK_MacBBBackup_PathB(
	IN 		PDM_ODM_T	pDM_Odm,
	IN  	pu4Byte		BackupRegAddr,
	IN OUT  pu4Byte		BackupRegData, 
	IN  	u4Byte		Number
	)
{
	u4Byte i;
	
	for (i = 0; i < Number; i++)
		BackupRegData[i] = ODM_Read4Byte(pDM_Odm, BackupRegAddr[i]);
}

VOID
_DPK_MacBBRestore_PathB(
	IN 	PDM_ODM_T	pDM_Odm,
	IN  pu4Byte		BackupRegAddr,
	IN  pu4Byte		BackupRegData, 
	IN  u4Byte		Number
	)
{
	u4Byte i;
	
	for (i = 0; i < Number; i++)
		ODM_Write4Byte(pDM_Odm, BackupRegAddr[i], BackupRegData[i]);
}


VOID
_DPK_RFBackup_PathB(
	IN 		PDM_ODM_T	        pDM_Odm,
	IN  	pu4Byte				BackupRegAddr,
	IN		ODM_RF_RADIO_PATH_E	RFPath,
	IN OUT  pu4Byte				BackupRegData, 
	IN  	u4Byte		        Number
	)
{
	u4Byte i;
	
	for (i = 0; i < Number; i++)
		BackupRegData[i] = ODM_GetRFReg(pDM_Odm, RFPath, BackupRegAddr[i], bRFRegOffsetMask);
}

VOID
_DPK_RFRestore_PathB(
	IN 	PDM_ODM_T	        pDM_Odm,
	IN  pu4Byte				BackupRegAddr,
	IN	ODM_RF_RADIO_PATH_E	RFPath,
	IN  pu4Byte		        BackupRegData, 
	IN  u4Byte		        Number
	)
{
	u4Byte i;
	
	for (i = 0; i < Number; i++)
		ODM_SetRFReg(pDM_Odm, RFPath, BackupRegAddr[i], bRFRegOffsetMask, BackupRegData[i]);
}



s4Byte
_ComputeLoopBackGain_PathB(
	IN	PDM_ODM_T			pDM_Odm
)
{
	// compute loopback gain
	// 計算tmpLBGain 用到的function hex2dec() 是將 twos complement的十六進位轉成十進位, 十六進位的格式為s11.10 
    // 舉例1, d00[10:0] = h'3ff, 經過 hex2dec(h'3ff) = 1023,
    // 舉例2, d00[10:0] = h'400, 經過 hex2dec(h'400) = -1024,
    // 舉例3, d00[10:0] = h'0A9, 經過 hex2dec(h'0A9) = 169,
    // 舉例4, d00[10:0] = h'54c, 經過 hex2dec(h'54c) = -692,
    // 舉例5, d00[10:0] = h'7ff, 經過 hex2dec(h'7ff) = -1,
    u4Byte reg0xD40_26_16 = ODM_GetBBReg(pDM_Odm, 0xD40, 0x7FF0000);
    u4Byte reg0xD40_10_0  = ODM_GetBBReg(pDM_Odm, 0xD40, 0x7FF);
	
    s4Byte tmpLBGain = _Sign(reg0xD40_26_16)*_Sign(reg0xD40_26_16) + _Sign(reg0xD40_10_0)*_Sign(reg0xD40_10_0);

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("<=== _ComputeLoopBackGain_PathB, tmpLBGain = 0x%X\n", tmpLBGain));

	return tmpLBGain;
}


BOOLEAN
_FineTuneLoopBackGain_PathB(
	IN		PDM_ODM_T			pDM_Odm, 
	IN		u4Byte 				DpkTxAGC	
	)
{
	PODM_RF_CAL_T  pRFCalibrateInfo = &(pDM_Odm->RFCalibrateInfo);	
	s4Byte tmpLBGain = 0;

    u4Byte rf0x64_orig = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x64, 0x7);
    u4Byte rf0x64_new = rf0x64_orig;

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("===> _FineTuneLoopBackGain_PathB\n"));

    do {
        // RF setting
        ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x00, bRFRegOffsetMask, DpkTxAGC);
		ODM_Write4Byte(pDM_Odm, 0x82c, 0x802083dd);
        // 注意page c1的c90在DPK過程中千萬不能被其他thread or process改寫成page c的c90值
        ODM_Write4Byte(pDM_Odm, 0xe90, 0x0101f018);
        
        // one shot
        ODM_Write4Byte(pDM_Odm, 0xec8, 0x800c5599);
        ODM_Write4Byte(pDM_Odm, 0xec8, 0x000c5599);
        // delay 50 ms,讓 delay 時間長一點, 確定 PA Scan function 有做完
        delay_ms(50);
        
        // reg82c[31] = 0 --> 切到 page C
        ODM_Write4Byte(pDM_Odm, 0x82c, 0x002083dd);
        ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x00, bRFRegOffsetMask, 0x33d8d);	

		tmpLBGain = _ComputeLoopBackGain_PathB(pDM_Odm);
		
	    if ((rf0x64_new == (BIT2|BIT1|BIT0)) || (rf0x64_new == 0)) {
	        //printf('DPK Phase 1 failed');
	        pRFCalibrateInfo->bDPKFail = TRUE;
			break;
	        //Go to DPK Phase 5
	    } else {
	        if (tmpLBGain < 263390) {// fine tune loopback path gain: newReg64[2:0] = reg64[2:0] - 3b'001
	        	rf0x64_new -= 1;
	            ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x64, 0x7, rf0x64_new);

	        } else if (tmpLBGain > 661607) {// fine tune loopback path gain: newReg64 [2:0] = reg64[2:0] + 3b'001
	        	rf0x64_new += 1;
	            ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x64, 0x7, rf0x64_new);

	        } else {
				pRFCalibrateInfo->bDPKFail = FALSE;
	        }
	    }

	} while (tmpLBGain < 263390 || 661607 < tmpLBGain);

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("<=== _FineTuneLoopBackGain_PathB\n"));

	return pRFCalibrateInfo->bDPKFail ? FALSE : TRUE;
            
}



VOID
_DPK_Init_PathB(
	IN 	PDM_ODM_T	pDM_Odm	
	)
{
	PODM_RF_CAL_T  pRFCalibrateInfo = &(pDM_Odm->RFCalibrateInfo);

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("===> _DPK_Init\n"));
	
	//TX pause
	ODM_Write1Byte(pDM_Odm, 0x522, 0x7f); 
	
	// reg82c[31] = b'0, 切換到 page C
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x002083dd);
	
	// AFE setting
	ODM_Write4Byte(pDM_Odm, 0xc68, 0x59791979);
	ODM_Write4Byte(pDM_Odm, 0xe60, 0x77777777);
	ODM_Write4Byte(pDM_Odm, 0xe64, 0x77777777);
	
	// external TRSW 切到 T
	ODM_Write4Byte(pDM_Odm, 0xeb0, 0x77777777);
	ODM_Write4Byte(pDM_Odm, 0xeb4, 0x01000077);
	
	// hardware 3-wire off
	ODM_Write4Byte(pDM_Odm, 0xe00, 0x00000004);
	
	// CCA off
	ODM_Write4Byte(pDM_Odm, 0x838, 0x16C89B4c);
	 
	//90c[15]: dac fifo reset by CSWU
	ODM_Write4Byte(pDM_Odm, 0x90c, 0x00008000);
	
	// r_gothrough_iqkdpk
	ODM_Write4Byte(pDM_Odm, 0xe94, 0x0100005D);
	
	// reg82c[31] = 1 --> 切到 page C1
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x802083dd);
	
	// IQK Amp off
	ODM_Write4Byte(pDM_Odm, 0xe8c, 0x3c000000);
	
	// tx_amp
	ODM_Write4Byte(pDM_Odm, 0xe98, 0x41382e21);
	ODM_Write4Byte(pDM_Odm, 0xe9c, 0x5b554f48);
	ODM_Write4Byte(pDM_Odm, 0xea0, 0x6f6b6661);
	ODM_Write4Byte(pDM_Odm, 0xea4, 0x817d7874);
	ODM_Write4Byte(pDM_Odm, 0xea8, 0x908c8884);
	ODM_Write4Byte(pDM_Odm, 0xeac, 0x9d9a9793);
	ODM_Write4Byte(pDM_Odm, 0xeb0, 0xaaa7a4a1);
	ODM_Write4Byte(pDM_Odm, 0xeb4, 0xb6b3b0ad);
	
	// tx_inverse
	ODM_Write4Byte(pDM_Odm, 0xe40, 0x02ce03e9);
	ODM_Write4Byte(pDM_Odm, 0xe44, 0x01fd0249);
	ODM_Write4Byte(pDM_Odm, 0xe48, 0x01a101c9);
	ODM_Write4Byte(pDM_Odm, 0xe4c, 0x016a0181);
	ODM_Write4Byte(pDM_Odm, 0xe50, 0x01430155);
	ODM_Write4Byte(pDM_Odm, 0xe54, 0x01270135);
	ODM_Write4Byte(pDM_Odm, 0xe58, 0x0112011c);
	ODM_Write4Byte(pDM_Odm, 0xe5c, 0x01000108);
	ODM_Write4Byte(pDM_Odm, 0xe60, 0x00f100f8);
	ODM_Write4Byte(pDM_Odm, 0xe64, 0x00e500eb);
	ODM_Write4Byte(pDM_Odm, 0xe68, 0x00db00e0);
	ODM_Write4Byte(pDM_Odm, 0xe6c, 0x00d100d5);
	ODM_Write4Byte(pDM_Odm, 0xe70, 0x00c900cd);
	ODM_Write4Byte(pDM_Odm, 0xe74, 0x00c200c5);
	ODM_Write4Byte(pDM_Odm, 0xe78, 0x00bb00be);
	ODM_Write4Byte(pDM_Odm, 0xe7c, 0x00b500b8);
	
	// reg82c[31] = b'0, 切換到 page C
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x002083dd); 
	
	pRFCalibrateInfo->bDPKFail = FALSE;

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("<=== _DPK_Init\n"));

}


BOOLEAN
_DPK_AdjustRFGain_PathB(
	IN 	PDM_ODM_T	pDM_Odm
	)
{
	PODM_RF_CAL_T  pRFCalibrateInfo = &(pDM_Odm->RFCalibrateInfo);	
	s4Byte tmpLBGain = 0;

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("===> _DPK_AdjustRFGain\n"));
	
	// RF setting
    ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x00, bRFRegOffsetMask, 0x50bfc);
    // Attn: A mode @ reg64[2:0], G mode @ reg56
    ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x64, bRFRegOffsetMask, 0x19aac); 
    // PGA gain: RF reg8f[14:13]
    ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x8f, bRFRegOffsetMask, 0x8a001); 
    
    // reg82c[31] = 1 --> 切到 page C1
    ODM_Write4Byte(pDM_Odm, 0x82c, 0x802083dd);
    
    // DPK setting
    ODM_Write4Byte(pDM_Odm, 0xe94, 0xf76c9f84);
    ODM_Write4Byte(pDM_Odm, 0xec8, 0x000c5599);
    ODM_Write4Byte(pDM_Odm, 0xec4, 0x11838000);
    ODM_SetBBReg(pDM_Odm, 0xed4, 0xFFF000, 0x100);  // 將cd4[23:12] 改成 h'100, 其它位元請保留原值不要寫到
    // 注意page c1的c90在DPK過程中千萬不能被其他thread or process改寫成page c的c90值
    ODM_Write4Byte(pDM_Odm, 0xe90, 0x0101f018);
    
    // one shot
    ODM_Write4Byte(pDM_Odm, 0xec8, 0x800c5599);
    ODM_Write4Byte(pDM_Odm, 0xec8, 0x000c5599);
    // delay 50 ms,讓 delay 時間長一點, 確定 PA Scan function 有做完
    delay_ms(50);
    
    // read back Loopback Gain
    ODM_Write4Byte(pDM_Odm, 0xeb8, 0x09000000);

    // reg82c[31] = 0 --> 切到 page C
    ODM_Write4Byte(pDM_Odm, 0x82c, 0x002083dd);
    ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x00, bRFRegOffsetMask, 0x33d8d);
    
    tmpLBGain = _ComputeLoopBackGain_PathB(pDM_Odm);
    
    // coarse tune loopback gain by RF reg8f[14:13] = 2b'11
    if (tmpLBGain < 263390) 
    {
        ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x8f, bRFRegOffsetMask, 0x8e001);
        _FineTuneLoopBackGain_PathB(pDM_Odm, 0x50bfc);

    } 
    else if (tmpLBGain > 661607)
    {
        // coarse tune loopback gain by RF reg8f[14:13] = 2b'00
    	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x8f, bRFRegOffsetMask, 0x88001);
		_FineTuneLoopBackGain_PathB(pDM_Odm, 0x50bfc);
    }
    else
    	pRFCalibrateInfo->bDPKFail = FALSE;

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("<=== _DPK_AdjustRFGain\n"));

	return pRFCalibrateInfo->bDPKFail ? FALSE : TRUE;

}



VOID
_DPK_GainLossToFindTxAGC_PathB(
	IN 	PDM_ODM_T	pDM_Odm	
	)
{
	PODM_RF_CAL_T  pRFCalibrateInfo = &(pDM_Odm->RFCalibrateInfo);
	u4Byte Reg0xD40 = 0;

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("===> _DPK_GainLossToFindTxAGC\n"));
	
	// RF setting
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x00, bRFRegOffsetMask, 0x50bfc);
	
	// reg82c[31] = 1 --> 切到 page C1
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x802083dd); 
	
	// regc20[15:13] = dB sel, 告訴 Gain Loss function 去尋找 dB_sel 所設定的PA gain loss目標所對應的 Tx AGC 為何.
	// dB_sel = b'000 ' 1.0 dB PA gain loss
	// dB_sel = b'001 ' 1.5 dB PA gain loss
	// dB_sel = b'010 ' 2.0 dB PA gain loss
	// dB_sel = b'011 ' 2.5 dB PA gain loss
	// dB_sel = b'100 ' 3.0 dB PA gain loss
	// dB_sel = b'101 ' 3.5 dB PA gain loss
	// dB_sel = b'110 ' 4.0 dB PA gain loss
	ODM_Write4Byte(pDM_Odm, 0xe20, 0x00002000);
	
	// DPK setting
	ODM_Write4Byte(pDM_Odm, 0xe94, 0xf76c9f84);
	ODM_Write4Byte(pDM_Odm, 0xec8, 0x000c5599);
	ODM_Write4Byte(pDM_Odm, 0xec4, 0x148b8000);
	
	// 注意page c1的c90在DPK過程中千萬不能被其他thread or process改寫成page c的c90值
	ODM_Write4Byte(pDM_Odm, 0xe90, 0x0401f018);
	
	// one shot
	ODM_Write4Byte(pDM_Odm, 0xec8, 0x800c5599);
	ODM_Write4Byte(pDM_Odm, 0xec8, 0x000c5599);
	// delay 50 ms,讓 delay 時間長一點, 確定 PA Scan function 有做完
	delay_ms(50);
	
	// read back Loopback Gain
	// 可以在 d40[3:0] 中讀回, dB_sel 中所設定的 gain loss 會落在哪一個 Tx AGC 設定
	// 讀回d00[3:0] = h'1 ' Tx AGC = h'13
	// 讀回d00[3:0] = h'2 ' Tx AGC = h'14
	// 讀回d00[3:0] = h'3 ' Tx AGC = h'15
	// 讀回d00[3:0] = h'4 ' Tx AGC = h'16
	// 讀回d00[3:0] = h'5 ' Tx AGC = h'17
	// 讀回d00[3:0] = h'6 ' Tx AGC = h'18
	// 讀回d00[3:0] = h'7 ' Tx AGC = h'19
	// 讀回d00[3:0] = h'8 ' Tx AGC = h'1a
	// 讀回d00[3:0] = h'9 ' Tx AGC = h'1b
	// 讀回d00[3:0] = h'a ' Tx AGC = h'1c
	//
	Reg0xD40 = ODM_Read4Byte(pDM_Odm, 0xd40);
	switch (ODM_Read4Byte(pDM_Odm, 0xd40) & (BIT3|BIT2|BIT1|BIT0))
	{
		case 0x0:	pRFCalibrateInfo->DpkTxAGC = 0x50bf0 | 0x2; break;
		case 0x1:	pRFCalibrateInfo->DpkTxAGC = 0x50bf0 | 0x3; break;
		case 0x2:	pRFCalibrateInfo->DpkTxAGC = 0x50bf0 | 0x4; break;
		case 0x3:	pRFCalibrateInfo->DpkTxAGC = 0x50bf0 | 0x5; break;
		case 0x4:	pRFCalibrateInfo->DpkTxAGC = 0x50bf0 | 0x6; break;
		case 0x5:	pRFCalibrateInfo->DpkTxAGC = 0x50bf0 | 0x7; break;
		case 0x6:	pRFCalibrateInfo->DpkTxAGC = 0x50bf0 | 0x8; break;
		case 0x7:	pRFCalibrateInfo->DpkTxAGC = 0x50bf0 | 0x9; break;
		case 0x8:	pRFCalibrateInfo->DpkTxAGC = 0x50bf0 | 0xa; break;
		case 0x9:	pRFCalibrateInfo->DpkTxAGC = 0x50bf0 | 0xb; break;
		case 0xa:	pRFCalibrateInfo->DpkTxAGC = 0x50bf0 | 0xc; break;
	}
	// reg82c[31] = 0 --> 切到 page C
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x002083dd);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x00, bRFRegOffsetMask, 0x33d8d);

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("<=== _DPK_GainLossToFindTxAGC\n"));
}


BOOLEAN
_DPK_AdjustRFGainByFoundTxAGC_PathB
(
	IN 	PDM_ODM_T	pDM_Odm
	)
{
	PODM_RF_CAL_T  pRFCalibrateInfo = &(pDM_Odm->RFCalibrateInfo);	
	s4Byte tmpLBGain = 0;

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("===> _DPK_AdjustRFGainByFoundTxAGC\n"));
	
	// RF setting, 重新設定RF reg00, 舉例用DPK Phase 2得到的 d40[3:0] = 0x6 ' TX AGC= 0x18 ' RF reg00[4:0] = 0x18
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x00, bRFRegOffsetMask, pRFCalibrateInfo->DpkTxAGC);
	// Attn: A mode @ reg64[2:0], G mode @ reg56
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x64, bRFRegOffsetMask, 0x19aac); 
	// PGA gain: RF reg8f[14:13]
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x8f, bRFRegOffsetMask, 0x8a001); 
	
	// reg82c[31] = 1 --> 切到 page C1
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x802083dd); 
	
	// DPK setting
	ODM_Write4Byte(pDM_Odm, 0xe94, 0xf76c9f84);
	ODM_Write4Byte(pDM_Odm, 0xec8, 0x000c5599);
	ODM_Write4Byte(pDM_Odm, 0xec4, 0x11838000);
	
	// 注意page c1的c90在DPK過程中千萬不能被其他thread or process改寫成page c的c90值
	ODM_Write4Byte(pDM_Odm, 0xe90, 0x0101f018);
	
	// one shot
	ODM_Write4Byte(pDM_Odm, 0xec8, 0x800c5599);
	ODM_Write4Byte(pDM_Odm, 0xec8, 0x000c5599);
	// delay 50 ms,讓 delay 時間長一點, 確定 PA Scan function 有做完
	delay_ms(50);
	
	// reg82c[31] = 0 --> 切到 page C
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x002083dd);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x00, bRFRegOffsetMask, 0x33d8d);
	

	tmpLBGain = _ComputeLoopBackGain_PathB(pDM_Odm);
	
	if (tmpLBGain < 263390)
	{
		// coarse tune loopback gain by RF reg8f[14:13] = 2b'11
		ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x8f, bRFRegOffsetMask, 0x8e001);
		_FineTuneLoopBackGain_PathB(pDM_Odm, pRFCalibrateInfo->DpkTxAGC);
	} 
	else if (tmpLBGain > 661607) 
	{
		// coarse tune loopback gain by RF reg8f[14:13] = 2b'00
		ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x8f, bRFRegOffsetMask, 0x88001);
		_FineTuneLoopBackGain_PathB(pDM_Odm, pRFCalibrateInfo->DpkTxAGC);
	} 
	else
		pRFCalibrateInfo->bDPKFail = FALSE;//Go to DPK Phase 4

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("<=== _DPK_AdjustRFGainByFoundTxAGC\n"));

	return pRFCalibrateInfo->bDPKFail ? FALSE : TRUE;

}


VOID
_DPK_DoAutoDPK_PathB(
	IN 	PDM_ODM_T	pDM_Odm	
	)
{	
	PODM_RF_CAL_T  pRFCalibrateInfo = &(pDM_Odm->RFCalibrateInfo);
	u4Byte tmpLBGain = 0, reg0xD40 = 0;
	
 	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("===> _DPK_DoAutoDPK\n"));
	
	
	// reg82c[31] = 0 --> 切到 page C
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x002083dd);
	// RF setting, 此處RF reg00, 與 DPK Phase 3 一致
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x00, bRFRegOffsetMask, pRFCalibrateInfo->DpkTxAGC);
	// Baseband Data Rate setting
	ODM_Write4Byte(pDM_Odm, 0xe20, 0x3c3c3c3c);
	ODM_Write4Byte(pDM_Odm, 0xe24, 0x3c3c3c3c);
	ODM_Write4Byte(pDM_Odm, 0xe28, 0x3c3c3c3c);
	ODM_Write4Byte(pDM_Odm, 0xe2c, 0x3c3c3c3c);
	ODM_Write4Byte(pDM_Odm, 0xe30, 0x3c3c3c3c);
	ODM_Write4Byte(pDM_Odm, 0xe34, 0x3c3c3c3c);
	ODM_Write4Byte(pDM_Odm, 0xe38, 0x3c3c3c3c);
	ODM_Write4Byte(pDM_Odm, 0xe3c, 0x3c3c3c3c);
	ODM_Write4Byte(pDM_Odm, 0xe40, 0x3c3c3c3c);
	ODM_Write4Byte(pDM_Odm, 0xe44, 0x3c3c3c3c);
	ODM_Write4Byte(pDM_Odm, 0xe48, 0x3c3c3c3c);
	ODM_Write4Byte(pDM_Odm, 0xe4c, 0x3c3c3c3c);

	// reg82c[31] = 1 --> 切到 page C1
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x802083dd); 

	// DPK setting
	ODM_Write4Byte(pDM_Odm, 0xe94, 0xf76C9f84);
	ODM_Write4Byte(pDM_Odm, 0xec8, 0x400C5599);
	ODM_Write4Byte(pDM_Odm, 0xec4, 0x11938080);    //0xec4[9:4]= DPk fail threshold

	if ( 36 <= *(pDM_Odm->pChannel) && *(pDM_Odm->pChannel) <= 53)//Channelchannel at low band)
		// r_agc
		ODM_Write4Byte(pDM_Odm, 0xebc, 0x00022a1f);
	else
		// r_agc
		ODM_Write4Byte(pDM_Odm, 0xebc, 0x00009dbf);

	// 注意page c1的c90在DPK過程中千萬不能被其他thread or process改寫成page c的c90值
	ODM_Write4Byte(pDM_Odm, 0xe90, 0x0101f018); // TODO: 0xe90(rA_LSSIWrite_Jaguar) can not be overwritten.

	// one shot
	ODM_Write4Byte(pDM_Odm, 0xec8, 0xc00c5599);
	ODM_Write4Byte(pDM_Odm, 0xec8, 0x400c5599);
	// delay 50 ms,讓 delay 時間長一點, 確定 PA Scan function 有做完
	delay_ms(50);

	// reg82c[31] = 0 --> 切到 page C
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x002083dd);
	// T-meter RFReg42[17] = 1 to enable read T-meter, [15:10] ' T-meter value
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x42, BIT17, 1);
	pRFCalibrateInfo->DpkThermal[ODM_RF_PATH_B] = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x42, 0xFC00);					// 讀出42[15:10] 值並存到變數TMeter
	DbgPrint("pRFCalibrateInfo->DpkThermal[ODM_RF_PATH_B] = 0x%X\n", pRFCalibrateInfo->DpkThermal[ODM_RF_PATH_B]);	
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x00, bRFRegOffsetMask, 0x33D8D);

	// reg82c[31] = 1 --> 切到 page C1
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x802083dd);
	// read back dp_fail report
	ODM_Write4Byte(pDM_Odm, 0xeb8, 0x00000000);
	//dp_fail這個bit在d40[6], 當 d40[6] = 1, 表示calibration失敗.
	reg0xD40 = ODM_Read4Byte(pDM_Odm, 0xd40);

	if ((reg0xD40 & BIT6) == BIT6) {
		//printf('DPK fail')
		pRFCalibrateInfo->bDPKFail = TRUE;
		//Go to DPK Phase 5
	} else {
		//printf('DPK success')
	}

	
	// read back
	ODM_Write4Byte(pDM_Odm, 0xe90, 0x0201f01f);
	ODM_Write4Byte(pDM_Odm, 0xeb8, 0x0c000000);

	// reg82c[31] = 1 --> 切到 page C1
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x002083dd);

	// 計算tmpGainLoss 用到的function hex2dec() 是將 twos complement的十六進位轉成十進位, 十六進位的格式為s11.9 
	// 舉例1, d00[10:0] = h'3ff, 經過 hex2dec(h'3ff) = 1023,
	// 舉例2, d00[10:0] = h'400, 經過 hex2dec(h'400) = -1024,
	// 舉例3, d00[10:0] = h'0A9, 經過 hex2dec(h'0A9) = 169,
	// 舉例4, d00[10:0] = h'54c, 經過 hex2dec(h'54c) = -692,
	// 舉例4, d00[10:0] = h'7ff, 經過 hex2dec(h'7ff) = -1,
	tmpLBGain = _ComputeLoopBackGain_PathB(pDM_Odm);


	// Gain Scaling
    if (227007 < tmpLBGain) 
	    pRFCalibrateInfo->DpkGain = 0x43ca43ca;
    else if (214309 < tmpLBGain && tmpLBGain <= 227007) 
	    pRFCalibrateInfo->DpkGain = 0x45c545c5;
    else if (202321 < tmpLBGain && tmpLBGain <= 214309) 
	    pRFCalibrateInfo->DpkGain = 0x47cf47cf;
    else if (191003 < tmpLBGain && tmpLBGain <= 202321) 
	    pRFCalibrateInfo->DpkGain = 0x49e749e7;
    else if (180318 < tmpLBGain && tmpLBGain <= 191003) 
	    pRFCalibrateInfo->DpkGain = 0x4c104c10;
    else if (170231 < tmpLBGain && tmpLBGain <= 180318) 
	    pRFCalibrateInfo->DpkGain = 0x4e494e49;
    else if (160709 < tmpLBGain && tmpLBGain <= 170231) 
	    pRFCalibrateInfo->DpkGain = 0x50925092;
    else if (151719 < tmpLBGain && tmpLBGain <= 160709) 
	    pRFCalibrateInfo->DpkGain = 0x52ec52ec;
    else if (151719 <= tmpLBGain) 
	    pRFCalibrateInfo->DpkGain = 0x55585558;

 	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("<=== _DPK_DoAutoDPK\n"));	
}


VOID	
_DPK_EnableDP_PathB(
	IN 	PDM_ODM_T	pDM_Odm
	)
{
	PODM_RF_CAL_T  pRFCalibrateInfo = &(pDM_Odm->RFCalibrateInfo);

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("===> _DPK_EnableDP\n"));

	// [31] = 1 --> switch to page C1
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x802083dd);

	// enable IQC matrix --> 因為 BB 信號會先經過 predistortion module, 才經過 IQC matrix 到 DAC 打出去
	// 所以要先 enable predistortion module {c90[7] = 1 (enable_predis) cc4[18] = 1 (確定讓 IQK/DPK module 有clock), cc8[29] = 1 (一個IQK/DPK module 裡的mux, 確認 data path 走IQK/DPK)}
	ODM_Write4Byte(pDM_Odm, 0xe90, 0x0000f098);
	ODM_Write4Byte(pDM_Odm, 0xe94, 0x776d9f84);
	ODM_Write4Byte(pDM_Odm, 0xec8, 0x20000000);
	ODM_Write4Byte(pDM_Odm, 0xe8c, 0x3c000000);
	// r_bnd
	ODM_Write4Byte(pDM_Odm, 0xeb8, 0x000fffff);
	
	if (pRFCalibrateInfo->bDPKFail)
	{
		// cc4[29] = 1 (bypass DP LUT)
		ODM_Write4Byte(pDM_Odm, 0xe98, 0x40004000);
		ODM_Write4Byte(pDM_Odm, 0xec4, 0x28840000);
	}
	else
	{
		ODM_Write4Byte(pDM_Odm, 0xe98, pRFCalibrateInfo->DpkGain);
		ODM_Write4Byte(pDM_Odm, 0xec4, 0x08840000);

		// PWSF
		// 寫PWSF table in 1st SRAM for PA = 11 use
		ODM_Write4Byte(pDM_Odm, 0xe20, 0x00000800);

		//*******************************************************
		//0xee4[0]是write enable，0xee4[7:1]是address，0xee4[15:8]和0xee4[23:16]對應到TX index，
		//若位置為0(0xee4[7:1] = 0x0)此時0xee4[15:8]對應到的TX RF index 是0x1f,，
		//0xee4[23:16]對應到的是0x1e，若位置為1(0xee4[7:1] = 0x1)，此時0xee4 [15:8]對應到0x1d，
		//0xee4[23:16]對應0x1c，其他依此類推，每個data欄位都是相差1dB。若gainloss找到的RF TX index=0x18，
		//則在0xee4 address對應到0x18的地方要填0x40(也就是0dB)，其他則照下面table的順序每格1dB依序排列
		//將所有的0xee4c的data欄位都填入相對應的值。
		//*************************************************************//

		{
			const s4Byte LEN = 25;
		    u4Byte baseIdx = 6; // 0 dB: 0x40			
		    u4Byte TablePWSF[] = {
		        0xff, 0xca, 0xa1, 0x80, 0x65, 0x51, 0x40/* 0 dB */, 
		        0x33, 0x28, 0x20, 0x19, 0x14, 0x10, 
		        0x0d, 0x0a, 0x08, 0x06, 0x05, 0x04, 
		        0x03, 0x03, 0x02, 0x02, 0x01, 0x01
		    };

		    u4Byte centerTxIdx = pRFCalibrateInfo->DpkTxAGC & 0x1F;
		    u4Byte centerAddr = (0x1F-centerTxIdx) / 2;
		    s4Byte i = 0, j = 0, value = 0, startIdx = 0;

			// Upper
		    startIdx = (((0x1F-centerTxIdx)%2 == 0) ? baseIdx+1 : baseIdx);
			
		    for (i = startIdx, j = 0; (centerAddr-j+1) >= 1; i -= 2, j++) {
		        if (i-1 < 0) 
		            value = (TablePWSF[0] << 16) | (TablePWSF[0] << 8) | ((centerAddr-j) << 1) | 0x1;
		        else 
		            value = (TablePWSF[i] << 16) | (TablePWSF[i-1] << 8) | ((centerAddr-j) << 1) | 0x1;
		        
				ODM_Write4Byte(pDM_Odm, 0xee4, value);
				ODM_Write1Byte(pDM_Odm, 0xee4, 0x0);		//write disable
		    }

		    // Lower
		    startIdx = (((0x1F-centerTxIdx)%2 == 0) ? baseIdx+2 : baseIdx+1);
		    centerAddr++; // Skip centerTxIdx
		    for (i = startIdx, j = 0; (centerAddr+j) < 16; i += 2, j++) { // Total: 16*2 = 32 values (Upper+Lower)
		        if (i+1 >= LEN) 
		            value = (TablePWSF[LEN-1] << 16) | (TablePWSF[LEN-1] << 8) | ((centerAddr+j) << 1) | 0x1;
		        else
		            value = (TablePWSF[i+1] << 16) | (TablePWSF[i] << 8) | ((centerAddr+j) << 1) | 0x1;
				ODM_Write4Byte(pDM_Odm, 0xee4, value);
				ODM_Write1Byte(pDM_Odm, 0xee4, 0x0);		//write disable
		    }
		}
	}
	// [31] = 0 --> switch to page C
	ODM_Write4Byte(pDM_Odm, 0x82c, 0x002083dd); 

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("<=== _DPK_EnableDP\n"));

}





VOID	
phy_DPCalibrate_PathB_8812A(
	IN 	PDM_ODM_T	pDM_Odm
	)
{	
    u4Byte backupMacBBRegAddrs[] = {
        0xE60, 0xE64, 0xC68, 0x82C, 0x838, 0x90C, 0x522, 0xE00, 0xE20, 0xE24, // 10
	    0xE28, 0xE2C, 0xE30, 0xE34, 0xE38, 0xE3C, 0xE40, 0xE44, 0xE48, 0xE4C, // 20
	    0xE94, 0xEB0, 0xEB4
	};	
    u4Byte backupRFRegAddrs[] = {0x00, 0x64, 0x8F};

    u4Byte backupMacBBRegData[sizeof(backupMacBBRegAddrs)/sizeof(u4Byte)];
    u4Byte backupRFRegData_A[sizeof(backupRFRegAddrs)/sizeof(u4Byte)];
    //u4Byte backupRFRegData_B[sizeof(backupRFRegAddrs)/sizeof(u4Byte)];	


	_DPK_MacBBBackup_PathB(pDM_Odm, backupMacBBRegAddrs, backupMacBBRegData, sizeof(backupMacBBRegAddrs)/sizeof(u4Byte));
	_DPK_RFBackup_PathB(pDM_Odm, backupRFRegAddrs, ODM_RF_PATH_B, backupRFRegData_A, sizeof(backupRFRegAddrs)/sizeof(u4Byte));

	_DPK_Init_PathB(pDM_Odm);

	if (_DPK_AdjustRFGain_PathB(pDM_Odm)) {               // Phase 1
	    _DPK_GainLossToFindTxAGC_PathB(pDM_Odm);          // Phase 2
	    if (_DPK_AdjustRFGainByFoundTxAGC_PathB(pDM_Odm)) // Phase 3
	        _DPK_DoAutoDPK_PathB(pDM_Odm);                // Phase 4
	}
    _DPK_EnableDP_PathB(pDM_Odm);                         // Phase 5

	_DPK_MacBBRestore_PathB(pDM_Odm, backupMacBBRegAddrs, backupMacBBRegData, sizeof(backupMacBBRegAddrs)/sizeof(u4Byte));
	_DPK_RFRestore_PathB(pDM_Odm, backupRFRegAddrs, ODM_RF_PATH_B, backupRFRegData_A, sizeof(backupRFRegAddrs)/sizeof(u4Byte));	
}                                                        
VOID	                                                 
PHY_DPCalibrate_8812A(                                   
	IN 	PDM_ODM_T	pDM_Odm                             
	)                                                    
{   
	pDM_Odm->DPK_Done = TRUE;
	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("===> PHY_DPCalibrate_8812A\n"));
	phy_DPCalibrate_PathA_8812A(pDM_Odm);
	phy_DPCalibrate_PathB_8812A(pDM_Odm);	
	ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("<=== PHY_DPCalibrate_8812A\n"));
}                                               
                                   
                                                          
                                                          
                                                          
                                                          

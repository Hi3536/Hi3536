/******************************************************************************
  Some simple Hisilicon Hi3536 system functions.
     This Source File TO PLAY IPC stream Frame (NVR);
	 Set some Var to config SYS....................
  Copyright (C), 2010-2011,  Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2018-6 Created
******************************************************************************/
#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include "NVR_comm.h"


/* g_s32VBSource: 0 to module common vb, 1 to private vb, 2 to user vb 
   And don't forget to set the value of VBSource file "load3535" */
HI_S32 g_s32VBSource = 0;
VB_POOL g_ahVbPool[VB_MAX_POOLS] = {[0 ... (VB_MAX_POOLS-1)] = VB_INVALID_POOLID};
//VB_POOL g_ahVbPool[VB_MAX_POOLS] = {VB_INVALID_POOLID};
#define PRINTF_VDEC_CHN_STATE(Chn, stChnStat) \
    do{\
            printf(" chn:%2d,  bStart:%2d,	DecodeFrames:%4d,  LeftPics:%3d,  LeftBytes:%10d,  LeftFrames:%4d,	RecvFrames:%6d\n",\
                Chn,\
                stChnStat.bStartRecvStream,\
                stChnStat.u32DecodeStreamFrames,\
                stChnStat.u32LeftPics,\
                stChnStat.u32LeftStreamBytes,\
                stChnStat.u32LeftStreamFrames,\
                stChnStat.u32RecvStreamFrames);\
        }while(0) 


HI_VOID	NVR_COMM_VDEC_Sysconf(VB_CONF_S *pstVbConf, SIZE_S *pstSize)
{
    memset(pstVbConf, 0, sizeof(VB_CONF_S));	  
    pstVbConf->u32MaxPoolCnt = VB_MAX_POOLS ;
    pstVbConf->astCommPool[0].u32BlkSize = (pstSize->u32Width * pstSize->u32Height * 3)>>1;
    pstVbConf->astCommPool[0].u32BlkCnt	 = 10;

#ifndef HI_FPGA
    pstVbConf->astCommPool[1].u32BlkSize = 3840*2160;
    pstVbConf->astCommPool[1].u32BlkCnt	 = 5;
#endif
}

HI_S32 NVR_COMM_SYS_Init(VB_CONF_S *pstVbConf)
{
	MPP_SYS_CONF_S stSysConf = {0};
	HI_S32 s32Ret = HI_FAILURE;
	HI_S32 i;

	HI_MPI_SYS_Exit();

	for(i=0;i<VB_MAX_USER;i++)
	{
		HI_MPI_VB_ExitModCommPool(i);
	}
	for(i=0; i<VB_MAX_POOLS; i++)
	{
		HI_MPI_VB_DestroyPool(i);
	}
	HI_MPI_VB_Exit();

	if (NULL == pstVbConf)
	{
		SAMPLE_PRT("input parameter is null, it is invaild!\n");
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VB_SetConf(pstVbConf);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_VB_SetConf failed!\n");
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VB_Init();
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_VB_Init failed!\n");
		return HI_FAILURE;
	}

	stSysConf.u32AlignWidth = SAMPLE_SYS_ALIGN_WIDTH;
	s32Ret = HI_MPI_SYS_SetConf(&stSysConf);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_SYS_SetConf failed\n");
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_SYS_Init();
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_SYS_Init failed!\n");
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

HI_S32 NVR_COMM_VPSS_Start(HI_S32 s32GrpCnt, SIZE_S *pstSize, HI_S32 s32ChnCnt,VPSS_GRP_ATTR_S *pstVpssGrpAttr)
{
	VPSS_GRP VpssGrp;
	VPSS_CHN VpssChn;
	VPSS_GRP_ATTR_S stGrpAttr = {0};
	VPSS_CHN_ATTR_S stChnAttr = {0};
	VPSS_GRP_PARAM_S stVpssParam = {0};
	HI_S32 s32Ret;
	HI_S32 i, j;

	/*** Set Vpss Grp Attr ***/

//	if(NULL == pstVpssGrpAttr)
//	{
		stGrpAttr.u32MaxW = pstSize->u32Width;
		stGrpAttr.u32MaxH = pstSize->u32Height;
		stGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;

		stGrpAttr.bIeEn = HI_FALSE;
		stGrpAttr.bNrEn = HI_TRUE;
		stGrpAttr.bDciEn = HI_FALSE;
		stGrpAttr.bHistEn = HI_FALSE;
		stGrpAttr.bEsEn = HI_FALSE;
		stGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
//	}
//	else
//	{
//		memcpy(&stGrpAttr,pstVpssGrpAttr,sizeof(VPSS_GRP_ATTR_S));
//	}


	for(i=0; i<s32GrpCnt; i++)
	{
		VpssGrp = i;
		/*** create vpss group ***/
		s32Ret = HI_MPI_VPSS_CreateGrp(VpssGrp, &stGrpAttr);
		if (s32Ret != HI_SUCCESS)
		{
                        printf("This %d VPSS GRP ERROR...\r\n",VpssGrp);
			SAMPLE_PRT("HI_MPI_VPSS_CreateGrp failed with %#x!\n", s32Ret);
			return HI_FAILURE;
		}

		/*** set vpss param ***/
		s32Ret = HI_MPI_VPSS_GetGrpParam(VpssGrp, &stVpssParam);
		if (s32Ret != HI_SUCCESS)
		{
			SAMPLE_PRT("failed with %#x!\n", s32Ret);
			return HI_FAILURE;
		}

		stVpssParam.u32IeStrength = 0;
		s32Ret = HI_MPI_VPSS_SetGrpParam(VpssGrp, &stVpssParam);
		if (s32Ret != HI_SUCCESS)
		{
			SAMPLE_PRT("failed with %#x!\n", s32Ret);
			return HI_FAILURE;
		}

		/*** enable vpss chn, with frame ***/
		for(j=0; j<s32ChnCnt; j++)
		{
			VpssChn = j;
			/* Set Vpss Chn attr */
			stChnAttr.bSpEn = HI_FALSE;
			stChnAttr.bUVInvert = HI_FALSE;
			stChnAttr.bBorderEn = HI_TRUE;
			stChnAttr.stBorder.u32Color = 0xff00;
			stChnAttr.stBorder.u32LeftWidth = 2;
			stChnAttr.stBorder.u32RightWidth = 2;
			stChnAttr.stBorder.u32TopWidth = 2;
			stChnAttr.stBorder.u32BottomWidth = 2;

			s32Ret = HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn, &stChnAttr);
			if (s32Ret != HI_SUCCESS)
			{
				SAMPLE_PRT("HI_MPI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
				return HI_FAILURE;
			}

			s32Ret = HI_MPI_VPSS_EnableChn(VpssGrp, VpssChn);
			if (s32Ret != HI_SUCCESS)
			{
				SAMPLE_PRT("HI_MPI_VPSS_EnableChn failed with %#x\n", s32Ret);
				return HI_FAILURE;
			}
		}

		/*** start vpss group ***/
		s32Ret = HI_MPI_VPSS_StartGrp(VpssGrp);
		if (s32Ret != HI_SUCCESS)
		{
			SAMPLE_PRT("HI_MPI_VPSS_StartGrp failed with %#x\n", s32Ret);
			return HI_FAILURE;
		}

	}
	return HI_SUCCESS;
}

HI_S32 NVR_COMM_VO_StartDev(VO_DEV VoDev, VO_PUB_ATTR_S *pstPubAttr)
{
	HI_S32 s32Ret = HI_SUCCESS;

	s32Ret = HI_MPI_VO_SetPubAttr(VoDev, pstPubAttr);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VO_Enable(VoDev);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	return s32Ret;
}

static HI_VOID NVR_COMM_VO_HdmiConvertSync(VO_INTF_SYNC_E enIntfSync,
	HI_HDMI_VIDEO_FMT_E *penVideoFmt)
{
	switch (enIntfSync)
	{
	case VO_OUTPUT_PAL:
		*penVideoFmt = HI_HDMI_VIDEO_FMT_PAL;
		break;
	case VO_OUTPUT_NTSC:
		*penVideoFmt = HI_HDMI_VIDEO_FMT_NTSC;
		break;
	case VO_OUTPUT_1080P24:
		*penVideoFmt = HI_HDMI_VIDEO_FMT_1080P_24;
		break;
	case VO_OUTPUT_1080P25:
		*penVideoFmt = HI_HDMI_VIDEO_FMT_1080P_25;
		break;
	case VO_OUTPUT_1080P30:
		*penVideoFmt = HI_HDMI_VIDEO_FMT_1080P_30;
		break;
	case VO_OUTPUT_720P50:
		*penVideoFmt = HI_HDMI_VIDEO_FMT_720P_50;
		break;
	case VO_OUTPUT_720P60:
		*penVideoFmt = HI_HDMI_VIDEO_FMT_720P_60;
		break;
	case VO_OUTPUT_1080I50:
		*penVideoFmt = HI_HDMI_VIDEO_FMT_1080i_50;
		break;
	case VO_OUTPUT_1080I60:
		*penVideoFmt = HI_HDMI_VIDEO_FMT_1080i_60;
		break;
	case VO_OUTPUT_1080P50:
		*penVideoFmt = HI_HDMI_VIDEO_FMT_1080P_50;
		break;
	case VO_OUTPUT_1080P60:
		*penVideoFmt = HI_HDMI_VIDEO_FMT_1080P_60;
		break;
	case VO_OUTPUT_576P50:
		*penVideoFmt = HI_HDMI_VIDEO_FMT_576P_50;
		break;
	case VO_OUTPUT_480P60:
		*penVideoFmt = HI_HDMI_VIDEO_FMT_480P_60;
		break;
	case VO_OUTPUT_800x600_60:
		*penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_800X600_60;
		break;
	case VO_OUTPUT_1024x768_60:
		*penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1024X768_60;
		break;
	case VO_OUTPUT_1280x1024_60:
		*penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1280X1024_60;
		break;
	case VO_OUTPUT_1366x768_60:
		*penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1366X768_60;
		break;
	case VO_OUTPUT_1440x900_60:
		*penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1440X900_60;
		break;
	case VO_OUTPUT_1280x800_60:
		*penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1280X800_60;
		break;
	case VO_OUTPUT_1920x1200_60:
		*penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1920X1200_60;
		break;    
	case VO_OUTPUT_3840x2160_30:
		*penVideoFmt = HI_HDMI_VIDEO_FMT_3840X2160P_30;
		break;
	case VO_OUTPUT_3840x2160_60:
		*penVideoFmt = HI_HDMI_VIDEO_FMT_3840X2160P_60;    
		break;
	default :
		SAMPLE_PRT("Unkonw VO_INTF_SYNC_E value!\n");
		break;
	}

	return;
}

HI_S32 NVR_COMM_VO_HdmiStart(VO_INTF_SYNC_E enIntfSync)
{
	HI_HDMI_ATTR_S      stAttr;
	HI_HDMI_VIDEO_FMT_E enVideoFmt;
	HI_HDMI_EDID_S stEdidData;

	NVR_COMM_VO_HdmiConvertSync(enIntfSync, &enVideoFmt);
	HI_MPI_HDMI_Init();

	HI_MPI_HDMI_Open(HI_HDMI_ID_0);

	HI_MPI_HDMI_Force_GetEDID(0,&stEdidData);
	HI_MPI_HDMI_GetAttr(HI_HDMI_ID_0, &stAttr);

	stAttr.bEnableHdmi = HI_TRUE;

	stAttr.bEnableVideo = HI_TRUE;
	stAttr.enVideoFmt = enVideoFmt;

	stAttr.enVidOutMode = HI_HDMI_VIDEO_MODE_YCBCR444;
	stAttr.enDeepColorMode = HI_HDMI_DEEP_COLOR_OFF;
	stAttr.bxvYCCMode = HI_FALSE;
	stAttr.enDefaultMode = HI_HDMI_FORCE_HDMI;

	stAttr.bEnableAudio = HI_FALSE;
	stAttr.enSoundIntf = HI_HDMI_SND_INTERFACE_I2S;
	stAttr.bIsMultiChannel = HI_FALSE;

	stAttr.enBitDepth = HI_HDMI_BIT_DEPTH_16;

	stAttr.bEnableAviInfoFrame = HI_TRUE;
	stAttr.bEnableAudInfoFrame = HI_TRUE;
	stAttr.bEnableSpdInfoFrame = HI_FALSE;
	stAttr.bEnableMpegInfoFrame = HI_FALSE;

	stAttr.bDebugFlag = HI_FALSE;          
	stAttr.bHDCPEnable = HI_FALSE;

	stAttr.b3DEnable = HI_FALSE;

	HI_MPI_HDMI_SetAttr(HI_HDMI_ID_0, &stAttr);

	HI_MPI_HDMI_Start(HI_HDMI_ID_0);

	printf("HDMI start success.\n");
	return HI_SUCCESS;
}

HI_VOID	NVR_COMM_VDEC_ModCommPoolConf(VB_CONF_S *pstModVbConf, \
    PAYLOAD_TYPE_E enType, SIZE_S *pstSize, HI_S32 s32ChnNum)
{
    HI_S32 PicSize, PmvSize;
	
    memset(pstModVbConf, 0, sizeof(VB_CONF_S));
    pstModVbConf->u32MaxPoolCnt = 80;
	
    VB_PIC_BLK_SIZE(pstSize->u32Width, pstSize->u32Height, enType, PicSize);	
    pstModVbConf->astCommPool[0].u32BlkSize = PicSize;
    pstModVbConf->astCommPool[0].u32BlkCnt  = 10*s32ChnNum;
	

	//user support decode B frame Mode Buffer define
	VB_PMV_BLK_SIZE(pstSize->u32Width, pstSize->u32Height, enType, PmvSize);
	pstModVbConf->astCommPool[1].u32BlkSize = PmvSize;
	pstModVbConf->astCommPool[1].u32BlkCnt  = 5*s32ChnNum;
    /* NOTICE: 			   
    1. if the VDEC channel is H264 channel and support to decode B frame, then you should allocate PmvBuffer 
    2. if the VDEC channel is MPEG4 channel, then you should allocate PmvBuffer.
    */
    if(PT_H265 == enType)
    {
        VB_PMV_BLK_SIZE(pstSize->u32Width, pstSize->u32Height, enType, PmvSize);
        
        pstModVbConf->astCommPool[1].u32BlkSize = PmvSize;
        pstModVbConf->astCommPool[1].u32BlkCnt  = 5*s32ChnNum;
    }
}

HI_S32 NVR_COMM_VO_GetWH(VO_INTF_SYNC_E enIntfSync, HI_U32 *pu32W,HI_U32 *pu32H, HI_U32 *pu32Frm)
{
	switch (enIntfSync)
	{
	case VO_OUTPUT_PAL       :  *pu32W = 720;  *pu32H = 576;  *pu32Frm = 25; break;
	case VO_OUTPUT_NTSC      :  *pu32W = 720;  *pu32H = 480;  *pu32Frm = 30; break;        
	case VO_OUTPUT_576P50    :  *pu32W = 720;  *pu32H = 576;  *pu32Frm = 50; break;
	case VO_OUTPUT_480P60    :  *pu32W = 720;  *pu32H = 480;  *pu32Frm = 60; break;
	case VO_OUTPUT_800x600_60:  *pu32W = 800;  *pu32H = 600;  *pu32Frm = 60; break;
	case VO_OUTPUT_720P50    :  *pu32W = 1280; *pu32H = 720;  *pu32Frm = 50; break;
	case VO_OUTPUT_720P60    :  *pu32W = 1280; *pu32H = 720;  *pu32Frm = 60; break;        
	case VO_OUTPUT_1080I50   :  *pu32W = 1920; *pu32H = 1080; *pu32Frm = 50; break;
	case VO_OUTPUT_1080I60   :  *pu32W = 1920; *pu32H = 1080; *pu32Frm = 60; break;
	case VO_OUTPUT_1080P24   :  *pu32W = 1920; *pu32H = 1080; *pu32Frm = 24; break;        
	case VO_OUTPUT_1080P25   :  *pu32W = 1920; *pu32H = 1080; *pu32Frm = 25; break;
	case VO_OUTPUT_1080P30   :  *pu32W = 1920; *pu32H = 1080; *pu32Frm = 30; break;
	case VO_OUTPUT_1080P50   :  *pu32W = 1920; *pu32H = 1080; *pu32Frm = 50; break;
	case VO_OUTPUT_1080P60   :  *pu32W = 1920; *pu32H = 1080; *pu32Frm = 60; break;
	case VO_OUTPUT_1024x768_60:  *pu32W = 1024; *pu32H = 768;  *pu32Frm = 60; break;
	case VO_OUTPUT_1280x1024_60: *pu32W = 1280; *pu32H = 1024; *pu32Frm = 60; break;
	case VO_OUTPUT_1366x768_60:  *pu32W = 1366; *pu32H = 768;  *pu32Frm = 60; break;
	case VO_OUTPUT_1440x900_60:  *pu32W = 1440; *pu32H = 900;  *pu32Frm = 60; break;
	case VO_OUTPUT_1280x800_60:  *pu32W = 1280; *pu32H = 800;  *pu32Frm = 60; break;        
	case VO_OUTPUT_1600x1200_60: *pu32W = 1600; *pu32H = 1200; *pu32Frm = 60; break;
	case VO_OUTPUT_1680x1050_60: *pu32W = 1680; *pu32H = 1050; *pu32Frm = 60; break;
	case VO_OUTPUT_1920x1200_60: *pu32W = 1920; *pu32H = 1200; *pu32Frm = 60; break;
	case VO_OUTPUT_3840x2160_30: *pu32W = 3840; *pu32H = 2160; *pu32Frm = 30; break;
	case VO_OUTPUT_3840x2160_60: *pu32W = 3840; *pu32H = 2160; *pu32Frm = 60; break;
	case VO_OUTPUT_USER    :     *pu32W = 720;  *pu32H = 576;  *pu32Frm = 25; break;
	default: 
		SAMPLE_PRT("vo enIntfSync not support!\n");
		return HI_FAILURE;
	}
	return HI_SUCCESS;
}

HI_S32 NVR_COMM_VO_StartLayer(VO_LAYER VoLayer,const VO_VIDEO_LAYER_ATTR_S *pstLayerAttr)
{
	HI_S32 s32Ret = HI_SUCCESS;
	s32Ret = HI_MPI_VO_SetVideoLayerAttr(VoLayer, pstLayerAttr);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VO_EnableVideoLayer(VoLayer);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	return s32Ret;
}

HI_S32 NVR_COMM_VO_StartChn(VO_LAYER VoLayer, SAMPLE_VO_MODE_E enMode)
{
	HI_S32 i;
	HI_S32 s32Ret = HI_SUCCESS;
	HI_U32 u32WndNum = 0;
	HI_U32 u32Square = 0;
	HI_U32 u32Width = 0;
	HI_U32 u32Height = 0;
	VO_CHN_ATTR_S stChnAttr;
	VO_VIDEO_LAYER_ATTR_S stLayerAttr;

	switch (enMode)
	{
	case VO_MODE_1MUX:
		u32WndNum = 1;
		u32Square = 1;
		break;
	case VO_MODE_4MUX:
		u32WndNum = 4;
		u32Square = 2;
		break;
	case VO_MODE_9MUX:
		u32WndNum = 9;
		u32Square = 3;
		break;
	case VO_MODE_16MUX:
		u32WndNum = 16;
		u32Square = 4;
		break;            
	case VO_MODE_25MUX:
		u32WndNum = 25;
		u32Square = 5;
		break;
	case VO_MODE_36MUX:
		u32WndNum = 36;
		u32Square = 6;
		break;
	case VO_MODE_64MUX:
		u32WndNum = 64;
		u32Square = 8;
		break;
	default:
		SAMPLE_PRT("failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	if((VoLayer == SAMPLE_VO_LAYER_VHD1) && (u32WndNum == 36))
		u32WndNum = 32;

	if((VoLayer ==VO_CAS_DEV_1+1)||(VoLayer ==VO_CAS_DEV_2+1))
	{
		s32Ret = HI_MPI_VO_GetVideoLayerAttr(SAMPLE_VO_DEV_DHD0, &stLayerAttr);
	}
	else
	{        
		s32Ret = HI_MPI_VO_GetVideoLayerAttr(VoLayer, &stLayerAttr);
	}
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}
	u32Width = stLayerAttr.stImageSize.u32Width;
	u32Height = stLayerAttr.stImageSize.u32Height;
	printf("u32Width:%d, u32Square:%d\n", u32Width, u32Square);
	for (i=0; i<u32WndNum; i++)
	{
		stChnAttr.stRect.s32X       = ALIGN_BACK((u32Width/u32Square) * (i%u32Square), 2);
		stChnAttr.stRect.s32Y       = ALIGN_BACK((u32Height/u32Square) * (i/u32Square), 2);
		stChnAttr.stRect.u32Width   = ALIGN_BACK(u32Width/u32Square, 2);
		stChnAttr.stRect.u32Height  = ALIGN_BACK(u32Height/u32Square, 2);
		stChnAttr.u32Priority       = 0;
		stChnAttr.bDeflicker        = HI_FALSE;

		s32Ret = HI_MPI_VO_SetChnAttr(VoLayer, i, &stChnAttr);
		if (s32Ret != HI_SUCCESS)
		{
			printf("%s(%d):failed with %#x!\n",\
				__FUNCTION__,__LINE__,  s32Ret);
			return HI_FAILURE;
		}

		s32Ret = HI_MPI_VO_EnableChn(VoLayer, i);
		if (s32Ret != HI_SUCCESS)
		{
			SAMPLE_PRT("failed with %#x!\n", s32Ret);
			return HI_FAILURE;
		}
	}
	return HI_SUCCESS;
}

HI_S32 NVR_COMM_VO_BindVpss(VO_LAYER VoLayer,VO_CHN VoChn,VPSS_GRP VpssGrp,VPSS_CHN VpssChn)
{
	HI_S32 s32Ret = HI_SUCCESS;
	MPP_CHN_S stSrcChn;
	MPP_CHN_S stDestChn;

	stSrcChn.enModId = HI_ID_VPSS;
	stSrcChn.s32DevId = VpssGrp;
	stSrcChn.s32ChnId = VpssChn;

	stDestChn.enModId = HI_ID_VOU;
	stDestChn.s32DevId = VoLayer;
	stDestChn.s32ChnId = VoChn;

	s32Ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	return s32Ret;
}

HI_S32 NVR_COMM_VO_UnBindVpss(VO_LAYER VoLayer,VO_CHN VoChn,VPSS_GRP VpssGrp,VPSS_CHN VpssChn)
{
	HI_S32 s32Ret = HI_SUCCESS;
	MPP_CHN_S stSrcChn;
	MPP_CHN_S stDestChn;

	stSrcChn.enModId = HI_ID_VPSS;
	stSrcChn.s32DevId = VpssGrp;
	stSrcChn.s32ChnId = VpssChn;

	stDestChn.enModId = HI_ID_VOU;
	stDestChn.s32DevId = VoLayer;
	stDestChn.s32ChnId = VoChn;

	s32Ret = HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}
	return s32Ret;
}

HI_S32 NVR_COMM_VO_StopChn(VO_LAYER VoLayer, SAMPLE_VO_MODE_E enMode)
{
	HI_S32 i;
	HI_S32 s32Ret = HI_SUCCESS;
	HI_U32 u32WndNum = 0;

	switch (enMode)
	{
	case VO_MODE_1MUX:
		{
			u32WndNum = 1;
			break;
		}

	case VO_MODE_4MUX:
		{
			u32WndNum = 4;
			break;
		}

	case VO_MODE_9MUX:
		{
			u32WndNum = 9;
			break;
		}

	case VO_MODE_16MUX:
		{
			u32WndNum = 16;
			break;
		}

	case VO_MODE_25MUX:
		{
			u32WndNum = 25;
			break;
		}

	case VO_MODE_36MUX:
		{
			u32WndNum = 36;
			break;
		}

	case VO_MODE_64MUX:
		{
			u32WndNum = 64;
			break;
		}

	default:
		SAMPLE_PRT("failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}


	for (i=0; i<u32WndNum; i++)
	{
		s32Ret = HI_MPI_VO_DisableChn(VoLayer, i);
		if (s32Ret != HI_SUCCESS)
		{
			SAMPLE_PRT("failed with %#x!\n", s32Ret);
			return HI_FAILURE;
		}
	}    
	return s32Ret;
}

HI_S32 NVR_COMM_VO_StopLayer(VO_LAYER VoLayer)
{
	HI_S32 s32Ret = HI_SUCCESS;

	s32Ret = HI_MPI_VO_DisableVideoLayer(VoLayer);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}
	return s32Ret;
}

HI_S32 NVR_COMM_VPSS_Stop(HI_S32 s32GrpCnt, HI_S32 s32ChnCnt)
{
	HI_S32 i, j;
	HI_S32 s32Ret = HI_SUCCESS;
	VPSS_GRP VpssGrp;
	VPSS_CHN VpssChn;

	for(i=0; i<s32GrpCnt; i++)
	{
		VpssGrp = i;
		s32Ret = HI_MPI_VPSS_StopGrp(VpssGrp);
		if (s32Ret != HI_SUCCESS)
		{
			SAMPLE_PRT("failed with %#x!\n", s32Ret);
			return HI_FAILURE;
		}
		for(j=0; j<s32ChnCnt; j++)
		{
			VpssChn = j;
			s32Ret = HI_MPI_VPSS_DisableChn(VpssGrp, VpssChn);
			if (s32Ret != HI_SUCCESS)
			{
				SAMPLE_PRT("failed with %#x!\n", s32Ret);
				return HI_FAILURE;
			}
		}

		s32Ret = HI_MPI_VPSS_DestroyGrp(VpssGrp);
		if (s32Ret != HI_SUCCESS)
		{
			SAMPLE_PRT("failed with %#x!\n", s32Ret);
			return HI_FAILURE;
		}
	}

	return HI_SUCCESS;
}

HI_S32 NVR_COMM_VO_HdmiStop(HI_VOID)
{
	HI_MPI_HDMI_Stop(HI_HDMI_ID_0);
	HI_MPI_HDMI_Close(HI_HDMI_ID_0);
	HI_MPI_HDMI_DeInit();

	return HI_SUCCESS;
}

HI_S32 NVR_COMM_VO_StopDev(VO_DEV VoDev)
{
	HI_S32 s32Ret = HI_SUCCESS;

	s32Ret = HI_MPI_VO_Disable(VoDev);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}
	return s32Ret;
}

HI_VOID	NVR_COMM_VDEC_ChnAttr(HI_S32 s32ChnNum, \
    VDEC_CHN_ATTR_S *pstVdecChnAttr, PAYLOAD_TYPE_E enType, SIZE_S *pstSize)
{
    HI_S32 i;

    for(i=0; i<s32ChnNum; i++)
    {
        pstVdecChnAttr[i].enType       = enType;
        pstVdecChnAttr[i].u32BufSize   = 1.5 * pstSize->u32Width * pstSize->u32Height;
        pstVdecChnAttr[i].u32Priority  = 5;
        pstVdecChnAttr[i].u32PicWidth  = pstSize->u32Width;
        pstVdecChnAttr[i].u32PicHeight = pstSize->u32Height;
        if (PT_H264 == enType || PT_MP4VIDEO == enType)
        { 
            pstVdecChnAttr[i].stVdecVideoAttr.enMode=VIDEO_MODE_STREAM;
			//参考帧
            pstVdecChnAttr[i].stVdecVideoAttr.u32RefFrameNum = 5;
            pstVdecChnAttr[i].stVdecVideoAttr.bTemporalMvpEnable = 0;//0不解码B帧，不分配pmv池，1支持
		
		
        }
        else if (PT_JPEG == enType || PT_MJPEG == enType)
        {
            pstVdecChnAttr->stVdecJpegAttr.enMode = VIDEO_MODE_FRAME;
            pstVdecChnAttr->stVdecJpegAttr.enJpegFormat = JPG_COLOR_FMT_YCBCR420;
        }
        else if(PT_H265 == enType)
        {
            pstVdecChnAttr[i].stVdecVideoAttr.enMode=VIDEO_MODE_STREAM;
            pstVdecChnAttr[i].stVdecVideoAttr.u32RefFrameNum = 2;
            pstVdecChnAttr[i].stVdecVideoAttr.bTemporalMvpEnable = HI_TRUE;
        }    
    }
}

HI_VOID	NVR_COMM_VDEC_VpssGrpAttr(HI_S32 s32ChnNum, VPSS_GRP_ATTR_S *pstVpssGrpAttr, SIZE_S *pstSize)
{
    HI_S32 i;

    for(i=0; i<s32ChnNum; i++)
    {
        pstVpssGrpAttr->enDieMode = VPSS_DIE_MODE_NODIE;
        pstVpssGrpAttr->bIeEn     = HI_FALSE;
        pstVpssGrpAttr->bDciEn    = HI_FALSE;
        pstVpssGrpAttr->bNrEn     = HI_TRUE;
        pstVpssGrpAttr->bHistEn   = HI_FALSE;
        pstVpssGrpAttr->enPixFmt  = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
        pstVpssGrpAttr->u32MaxW   = ALIGN_UP(pstSize->u32Width,  16);
        pstVpssGrpAttr->u32MaxH   = ALIGN_UP(pstSize->u32Height, 16);
    }
}


HI_VOID	NVR_COMM_VDEC_VoAttr(HI_S32 s32ChnNum, VO_DEV VoDev ,VO_PUB_ATTR_S *pstVoPubAttr, VO_VIDEO_LAYER_ATTR_S *pstVoLayerAttr)
{
    HI_S32 u32Width, u32Height;

    /*********** set the pub attr of VO ****************/
    if (0 == VoDev)
    {
        pstVoPubAttr->enIntfSync = VO_OUTPUT_720P50;
        pstVoPubAttr->enIntfType = VO_INTF_BT1120 | VO_INTF_VGA;
    }
    else if (1 == VoDev)
    {
        pstVoPubAttr->enIntfSync = VO_OUTPUT_720P50;
        pstVoPubAttr->enIntfType = VO_INTF_VGA;
    }
    else if (VoDev>=2 && VoDev <=3)
    {
        pstVoPubAttr->enIntfSync = VO_OUTPUT_PAL;
        pstVoPubAttr->enIntfType = VO_INTF_CVBS;
    }
    pstVoPubAttr->u32BgColor = VO_BKGRD_BLUE;


    /***************** set the layer attr of VO  *******************/
    if(pstVoPubAttr->enIntfSync == VO_OUTPUT_720P50)
    {
        u32Width  = 1280;
        u32Height = 720;
    }
    else if (pstVoPubAttr->enIntfSync == VO_OUTPUT_PAL)
    {
        u32Width  = 720;
        u32Height = 576;
    }	
    pstVoLayerAttr->stDispRect.s32X		  = 0;
    pstVoLayerAttr->stDispRect.s32Y		  = 0;
    pstVoLayerAttr->stDispRect.u32Width	  = u32Width;
    pstVoLayerAttr->stDispRect.u32Height  = u32Height;
    pstVoLayerAttr->stImageSize.u32Width  = u32Width;
    pstVoLayerAttr->stImageSize.u32Height = u32Height;
    pstVoLayerAttr->bDoubleFrame		  = HI_FALSE;
    pstVoLayerAttr->bClusterMode          = HI_FALSE;
    pstVoLayerAttr->u32DispFrmRt		  = 60 ;
    pstVoLayerAttr->enPixFormat		 = PIXEL_FORMAT_YUV_SEMIPLANAR_420;		
}

HI_S32	NVR_COMM_VDEC_InitModCommVb(VB_CONF_S *pstModVbConf)
{
    HI_S32 i;
    HI_S32 s32Ret;

    HI_MPI_VB_ExitModCommPool(VB_UID_VDEC);

    if(0 == g_s32VBSource)
    {
        CHECK_RET(HI_MPI_VB_SetModPoolConf(VB_UID_VDEC, pstModVbConf), "HI_MPI_VB_SetModPoolConf");
        CHECK_RET(HI_MPI_VB_InitModCommPool(VB_UID_VDEC), "HI_MPI_VB_InitModCommPool");
    }
    else if (2 == g_s32VBSource)
    {
        if (pstModVbConf->u32MaxPoolCnt > VB_MAX_POOLS)
        {
            printf("vb pool num(%d) is larger than VB_MAX_POOLS. \n", pstModVbConf->u32MaxPoolCnt);
            return HI_FAILURE;
        }
        for (i = 0; i < pstModVbConf->u32MaxPoolCnt; i++)
        {
            if (pstModVbConf->astCommPool[i].u32BlkSize && pstModVbConf->astCommPool[i].u32BlkCnt)
            {
                g_ahVbPool[i] = HI_MPI_VB_CreatePool(pstModVbConf->astCommPool[i].u32BlkSize, 
                    pstModVbConf->astCommPool[i].u32BlkCnt, NULL);
                if (VB_INVALID_POOLID == g_ahVbPool[i])
                    goto fail;
            }
        }
        return HI_SUCCESS;               

    fail:
        for (;i>=0;i--)
        {   
            if (VB_INVALID_POOLID != g_ahVbPool[i])
            {
                s32Ret = HI_MPI_VB_DestroyPool(g_ahVbPool[i]);
                HI_ASSERT(HI_SUCCESS == s32Ret);
                g_ahVbPool[i] = VB_INVALID_POOLID;
            }
        }
        return HI_FAILURE;
    }
    
    return HI_SUCCESS;
}
//This head file comm NVR_live_Client.h
int  LIVE_COMM_VDEC_SendStream_NVR( int chn ,unsigned char* Addr, unsigned int Len, double u64pts ,int s32Sec )
{
	VDEC_STREAM_S stStream;
	HI_S32 s32Ret ;
	
		stStream.u64PTS  = u64pts ;
		stStream.pu8Addr = Addr;
		stStream.u32Len  = Len; 

		//	stStream.bEndOfFrame  = (pstVdecThreadParam->s32StreamMode==VIDEO_MODE_FRAME)? HI_TRU//E: HI_FALSE;
		//		stStream.bEndOfStream = HI_FALSE;       
		s32Ret=HI_MPI_VDEC_SendStream(chn, &stStream,s32Sec);
		if (HI_SUCCESS != s32Ret)
		{

			printf("SEND Stream ERROR !\r\n");
			return -1;
		}
	
	return  0 ;
}
/*
HI_VOID * NVR_COMM_VDEC_GetChnLuma(HI_VOID *pArgs)
{
    VDEC_CHN_LUM_S stLumaPix;
    VdecThreadParam *pstVdecThreadParam =(VdecThreadParam *)pArgs;
    HI_BOOL bRunFlag = HI_TRUE;
    HI_S32 s32Ret;
    FILE *fpLuma = HI_NULL;
    HI_CHAR FileName[128];
    HI_CHAR acString[128];

    snprintf(FileName, 128, "LumaPixChn%d.txt", pstVdecThreadParam->s32ChnId);		
    fpLuma=fopen(FileName, "w+");
    if(fpLuma == NULL)
    {
        printf("SAMPLE_TEST:can't open file %s in get luma thread:%d\n", pstVdecThreadParam->cFileName, pstVdecThreadParam->s32ChnId);
        return (HI_VOID *)(HI_FAILURE);
    }	
	
    while(1)
    {
        switch(pstVdecThreadParam->eCtrlSinal)
        {
            case VDEC_CTRL_START:
                s32Ret = HI_MPI_VDEC_GetChnLuma(pstVdecThreadParam->s32ChnId, &stLumaPix);	
                if (HI_SUCCESS == s32Ret)
                {
                    memset(acString, 0, 128);
                    sprintf(acString, "chn %d,  Pts = %lld,  LumPixSum = %lld,  LumPixAverage=%d!     \n",
                    pstVdecThreadParam->s32ChnId, stLumaPix.u64Pts, stLumaPix.u64LumPixSum,stLumaPix.u32LumPixAverage);
                    fprintf(fpLuma, "%s\n", acString);
                }
                break;

            case VDEC_CTRL_PAUSE:
                sleep(MIN2(pstVdecThreadParam->s32IntervalTime,1000));
                break;

            case VDEC_CTRL_STOP:
                bRunFlag=HI_FALSE;
                break;

            default:
                printf("SAMPLE_TEST:unknow command in get luma thread %d\n", pstVdecThreadParam->s32ChnId);
                bRunFlag=HI_FALSE;
                break;
        }
        usleep(40000);
        if(bRunFlag==HI_FALSE)
        {
            break;
        }
    }   
    printf("SAMPLE_TEST:get LumaPix thread %d return ...\n", pstVdecThreadParam->s32ChnId);
    fclose(fpLuma);	
	
    return (HI_VOID *)HI_SUCCESS;
}
/*
HI_VOID SAMPLE_COMM_VDEC_CmdCtrl(HI_S32 s32ChnNum,VdecThreadParam *pstVdecSend)
{
    HI_S32 i;
    VDEC_CHN_STAT_S stStat;
    char c=0;

    while(1)    
    {
        printf("\nSAMPLE_TEST:press 'e' to exit; 'p' to pause; 'r' to resume; 'q' to query!\n"); 
        c = getchar();
        getchar();
        if (c == 'e') 
            break;
        else if (c == 'p') 
        {            
            for (i=0; i<s32ChnNum; i++)           
            pstVdecSend[i].eCtrlSinal = VDEC_CTRL_PAUSE; 
        } 
        else if (c == 'r')        
        {           
            for (i=0; i<s32ChnNum; i++)      
            pstVdecSend[i].eCtrlSinal = VDEC_CTRL_START;    
        }
        else if (c == 'q')        
        {           
            for (i=0; i<s32ChnNum; i++)  
            {              
                HI_MPI_VDEC_Query(pstVdecSend[i].s32ChnId, &stStat);     
                PRINTF_VDEC_CHN_STATE(pstVdecSend[i].s32ChnId, stStat);
            }
        }
    }
}
*/
/*
HI_VOID NVR_COMM_VDEC_StartSendStream(HI_S32 s32ChnNum, NVRThreadParam *pstVdecSend, pthread_t *pVdecThread)
{
 
        pthread_create(&pVdecThread[s32ChnNum], 0, pstRTSPThread, (HI_VOID *)&pstVdecSend[s32ChnNum]);
    
}
*/
/*
HI_VOID SAMPLE_COMM_VDEC_StopSendStream(HI_S32 s32ChnNum, VdecThreadParam *pstVdecSend, pthread_t *pVdecThread)
{
    HI_S32  i;

    for(i=0; i<s32ChnNum; i++)
    {	
        pstVdecSend[i].eCtrlSinal=VDEC_CTRL_STOP;	
        pthread_join(pVdecThread[i], HI_NULL);
    }
}

HI_VOID SAMPLE_COMM_VDEC_StartGetLuma(HI_S32 s32ChnNum, VdecThreadParam *pstVdecSend, pthread_t *pVdecThread)
{
    HI_S32  i;

    for(i=0; i<s32ChnNum; i++)
    {
        pthread_create(&pVdecThread[i+VDEC_MAX_CHN_NUM], 0, SAMPLE_COMM_VDEC_GetChnLuma, (HI_VOID *)&pstVdecSend[i]);	
    }
}

HI_VOID SAMPLE_COMM_VDEC_StopGetLuma(HI_S32 s32ChnNum, VdecThreadParam *pstVdecSend, pthread_t *pVdecThread)
{
    HI_S32  i;

    for(i=0; i<s32ChnNum; i++)
    {	
        pstVdecSend[i].eCtrlSinal = VDEC_CTRL_STOP;
        pthread_join(pVdecThread[i+VDEC_MAX_CHN_NUM], HI_NULL);     
    }
}
*/
HI_S32 NVR_COMM_VDEC_Start(HI_S32 s32ChnNum, VDEC_CHN_ATTR_S *pstAttr)
{
    HI_S32  i;
    HI_U32 u32BlkCnt = 10;
    VDEC_CHN_POOL_S stPool;

    for(i=0; i<s32ChnNum; i++)
    {	
        if(1 == g_s32VBSource)
        {
            CHECK_CHN_RET(HI_MPI_VDEC_SetChnVBCnt(i, u32BlkCnt), i, "HI_MPI_VDEC_SetChnVBCnt");				
        }		
        CHECK_CHN_RET(HI_MPI_VDEC_CreateChn(i, &pstAttr[i]), i, "HI_MPI_VDEC_CreateChn");
        if (2 == g_s32VBSource)
        {
            stPool.hPicVbPool = g_ahVbPool[0];
            stPool.hPmvVbPool = -1;
            CHECK_CHN_RET(HI_MPI_VDEC_AttachVbPool(i, &stPool), i, "HI_MPI_VDEC_AttachVbPool");
        }
        CHECK_CHN_RET(HI_MPI_VDEC_StartRecvStream(i), i, "HI_MPI_VDEC_StartRecvStream");
    }

    return HI_SUCCESS;
}

HI_S32 NVR_COMM_VDEC_SetChnPraram(HI_U32 s32ChnNum,VDEC_CHN VdeChn , PAYLOAD_TYPE_E entype, VDEC_CHN_PARAM_S *pstparam)
{
	HI_U32 i =0 ; HI_S32 flage= HI_SUCCESS ;
  if (entype == PT_JPEG || PT_MJPEG == entype )
  {
	  printf("The type Noset Chnpraram\n");
	  return flage ;
  }
  if (s32ChnNum > (i+1)){
	VDEC_CHN_PARAM_S SetChnPram[s32ChnNum-1] ;
	for(i=0;i<s32ChnNum;i++)
	{
	    SetChnPram[i].s32DisplayFrameNum = pstparam->s32DisplayFrameNum ; //通道显示帧数
	    SetChnPram[i].s32ChanErrThr = pstparam->s32ChanErrThr ; //通道错误率阈值
 	    SetChnPram[i].s32ChanStrmOFThr = pstparam->s32ChanStrmOFThr; //设置解码前丢帧阈值
		SetChnPram[i].s32DecMode = pstparam->s32DecMode; //解码模式
		SetChnPram[i].s32DecOrderOutput = pstparam->s32DecOrderOutput; //解码图像输出顺序
		SetChnPram[i].enVideoFormat = pstparam->enVideoFormat; //解码图像数据格式
		SetChnPram[i].enCompressMode = pstparam->enCompressMode; //解码图像压缩模式
		flage = HI_MPI_VDEC_SetChnParam(i,&SetChnPram[i]);
		if (flage != HI_SUCCESS)
		{
			printf("Set1:%S ChnPraram ERROR!\n",i);
				return flage ;
		}
	}
  }else if(s32ChnNum ==(i+1))
  {
	  VDEC_CHN_PARAM_S SetChnPram ;
	  SetChnPram.s32DisplayFrameNum = pstparam->s32DisplayFrameNum ; //通道显示帧数
	  SetChnPram.s32ChanErrThr = pstparam->s32ChanErrThr ; //通道错误率阈值
	  SetChnPram.s32ChanStrmOFThr = pstparam->s32ChanStrmOFThr; //设置解码前丢帧阈值
	  SetChnPram.s32DecMode = pstparam->s32DecMode; //解码模式
	  SetChnPram.s32DecOrderOutput = pstparam->s32DecOrderOutput; //解码图像输出顺序
	  SetChnPram.enVideoFormat = pstparam->enVideoFormat; //解码图像数据格式
	  SetChnPram.enCompressMode = pstparam->enCompressMode; //解码图像压缩模式
	  flage = HI_MPI_VDEC_SetChnParam(VdeChn,&SetChnPram);
		  if (flage != HI_SUCCESS)
		  {
			  printf("Set2:%S ChnPraram ERROR!\n",VdeChn);
			  return flage ;
		  }
  }
  return flage ;
}

HI_S32 NVR_COMM_VDEC_Stop(HI_S32 s32ChnNum)
{
    HI_S32 i;	

    for(i=0; i<s32ChnNum; i++)
    {
        CHECK_CHN_RET(HI_MPI_VDEC_StopRecvStream(i), i, "HI_MPI_VDEC_StopRecvStream");       
        CHECK_CHN_RET(HI_MPI_VDEC_DestroyChn(i), i, "HI_MPI_VDEC_DestroyChn");
    }

    return HI_SUCCESS;
}

HI_VOID NVR_COMM_SYS_Exit(void)
{

	HI_S32 i;

	HI_MPI_SYS_Exit();
	for(i=0;i<VB_MAX_USER;i++)
	{
		HI_MPI_VB_ExitModCommPool(i);
	}

	for(i=0; i<VB_MAX_POOLS; i++)
	{
		HI_MPI_VB_DestroyPool(i);
	}	

	HI_MPI_VB_Exit();
	return;
}

HI_S32 NVR_COMM_VDEC_BindVpss(VDEC_CHN VdChn, VPSS_GRP VpssGrp)
{
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId = HI_ID_VDEC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = VdChn;

    stDestChn.enModId = HI_ID_VPSS;
    stDestChn.s32DevId = VpssGrp;
    stDestChn.s32ChnId = 0;

    CHECK_RET(HI_MPI_SYS_Bind(&stSrcChn, &stDestChn), "HI_MPI_SYS_Bind");

    return HI_SUCCESS;
}

HI_S32 NVR_COMM_VDEC_BindVo(VDEC_CHN VdChn, VO_LAYER VoLayer, VO_CHN VoChn)
{
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId = HI_ID_VDEC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = VdChn;

    stDestChn.enModId = HI_ID_VOU;
    stDestChn.s32DevId = VoLayer;
    stDestChn.s32ChnId = VoChn;

    CHECK_RET(HI_MPI_SYS_Bind(&stSrcChn, &stDestChn), "HI_MPI_SYS_Bind");

    return HI_SUCCESS;
}

HI_S32 NVR_COMM_VDEC_UnBindVpss(VDEC_CHN VdChn, VPSS_GRP VpssGrp)
{
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId = HI_ID_VDEC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = VdChn;

    stDestChn.enModId = HI_ID_VPSS;
    stDestChn.s32DevId = VpssGrp;
    stDestChn.s32ChnId = 0;

    CHECK_RET(HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn), "HI_MPI_SYS_UnBind");

    return HI_SUCCESS;
}

HI_S32 NVR_COMM_VDEC_UnBindVo(VDEC_CHN VdChn, VO_LAYER VoLayer, VO_CHN VoChn)
{
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId = HI_ID_VDEC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = VdChn;

    stDestChn.enModId = HI_ID_VOU;
    stDestChn.s32DevId = VoLayer;
    stDestChn.s32ChnId = VoChn;
    CHECK_RET(HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn), "HI_MPI_SYS_UnBind");
    return HI_SUCCESS;
}

/******************************************************************************
* function : Set system memory location
******************************************************************************/
HI_S32 NVR_COMM_VDEC_MemConfig(HI_VOID)
{
    HI_S32 i = 0;
    HI_S32 s32Ret = HI_SUCCESS;

    HI_CHAR * pcMmzName;
    MPP_CHN_S stMppChnVDEC;

    /* VDEC chn max is 80*/
    for(i=0; i<80; i++)
    {
        stMppChnVDEC.enModId = HI_ID_VDEC;
        stMppChnVDEC.s32DevId = 0;
        stMppChnVDEC.s32ChnId = i;
        
        if(0 == (i%2))
        {
            pcMmzName = NULL;  
        }
        else
        {
            pcMmzName = "ddr1";
        }

        s32Ret = HI_MPI_SYS_SetMemConf(&stMppChnVDEC,pcMmzName);
        if (s32Ret)
        {
            SAMPLE_PRT("HI_MPI_SYS_SetMemConf ERR !\n");
            return HI_FAILURE;
        }
    }  

    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

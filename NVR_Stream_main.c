/******************MPP MODE VDEC BIND VO MODE **************************
*******************user sample define var*******************************
*******************V1.0*************************************************/

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
#include "NVR_live_Client.h"
#include "NVR_comm.h"
#include "comeonvif_client.h"
#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif
pthread_t   VdecThread[2*VDEC_MAX_CHN_NUM];
HI_VOID NVR_VDEC_HandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTSTP == signo)
    {
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }

    exit(0);
}

HI_S32 NVR_INIT_VDEC(HI_U32 vdcnt ,HI_U32 desflag )
{
    VB_CONF_S stVbConf, stModVbConf;
    HI_S32 i, s32Ret = HI_SUCCESS;
    VDEC_CHN_ATTR_S stVdecChnAttr[VDEC_MAX_CHN_NUM];
    VPSS_GRP_ATTR_S stVpssGrpAttr[VDEC_MAX_CHN_NUM];
    SIZE_S stSize, stRotateSize;
    VO_DEV VoDev;
    VO_LAYER VoLayer;
    VO_PUB_ATTR_S stVoPubAttr;
    VO_VIDEO_LAYER_ATTR_S stVoLayerAttr;
    ROTATE_E enRotate;
	//设置通道数、Vpss组个数
    HI_U32 u32VdCnt = vdcnt;
    HI_U32 u32GrpCnt = vdcnt;
 
    stSize.u32Width = HD_WIDTH;
    stSize.u32Height = HD_HEIGHT;

	if (desflag ==1)
	{
		printf("Go To Destory Decoder pram ....... \r\n");
		goto END6 ;
	}
	
    /************************************************
    step1:  init SYS and common VB 配置及初始化系统公共缓存池 
    *************************************************/
    NVR_COMM_VDEC_Sysconf(&stVbConf, &stSize);
    s32Ret = NVR_COMM_SYS_Init(&stVbConf);
    if(s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("init sys fail for %#x!\n", s32Ret);
        goto END1;
    }

    /************************************************
    step2:  init mod common VB  配置解码模块视频缓存池
    *************************************************/
    NVR_COMM_VDEC_ModCommPoolConf(&stModVbConf, PT_H264, &stSize, u32VdCnt);	
    s32Ret = NVR_COMM_VDEC_InitModCommVb(&stModVbConf);
    if(s32Ret != HI_SUCCESS)
    {	    	
        SAMPLE_PRT("init mod common vb fail for %#x!\n", s32Ret);
        goto END1;
    }

    /************************************************
    step3:  start VDEC
    *************************************************/
    NVR_COMM_VDEC_ChnAttr(u32VdCnt, &stVdecChnAttr[0],PT_H264, &stSize);
	
    s32Ret = NVR_COMM_VDEC_Start(u32VdCnt, &stVdecChnAttr[0]);
    if(s32Ret != HI_SUCCESS)
    {	
        SAMPLE_PRT("start VDEC fail for %#x!\n", s32Ret);
        goto END2;
    }
	VDEC_CHN_PARAM_S itsparam ;
	itsparam.s32DecMode = 0 ;
	itsparam.s32DecOrderOutput = 0 ;
	itsparam.s32ChanErrThr = 50 ;
	itsparam.s32ChanStrmOFThr = 0;
      itsparam.s32DisplayFrameNum = 60 ;
        itsparam.enVideoFormat = VIDEO_FORMAT_TILE ;
	itsparam.enCompressMode = COMPRESS_MODE_NONE ;
//	s32Ret = NVR_COMM_VDEC_SetChnPraram(u32VdCnt,i=0,PT_H264,&itsparam);
	//s32Ret = HI_MPI_VDEC_SetChnParam(0,&itsparam);
//	if(s32Ret != HI_SUCCESS)
//	{	
//		SAMPLE_PRT("start VDEC fail for %#x!\n", s32Ret);
//		goto END2;
//	}
    /************************************************
    step4:  start VPSS
    *************************************************/
    stRotateSize.u32Width = stRotateSize.u32Height = MAX2(stSize.u32Width, stSize.u32Height);
    NVR_COMM_VDEC_VpssGrpAttr(u32GrpCnt, &stVpssGrpAttr[0], &stRotateSize);
    s32Ret = NVR_COMM_VPSS_Start(u32GrpCnt, &stRotateSize, 1, &stVpssGrpAttr[0]);
    if(s32Ret != HI_SUCCESS)
    {	    
        SAMPLE_PRT("start VPSS fail for %#x!\n", s32Ret);
        goto END3;
    }
	
    /************************************************
    step5:  start VO
    *************************************************/	
    VoDev = SAMPLE_VO_DEV_DHD0;
    VoLayer = SAMPLE_VO_LAYER_VHD0;
    
#ifdef HI_FPGA
    stVoPubAttr.enIntfSync = VO_OUTPUT_1080P30;
    stVoPubAttr.enIntfType = VO_INTF_VGA;
#else
    stVoPubAttr.enIntfSync = VO_OUTPUT_1080P60;
    stVoPubAttr.enIntfType = VO_INTF_HDMI;
#endif
    s32Ret = NVR_COMM_VO_StartDev(VoDev, &stVoPubAttr);
    if(s32Ret != HI_SUCCESS)
    {		
        SAMPLE_PRT("vdec bind vpss fail for %#x!\n", s32Ret);
        goto END4_1;
    }
	

#ifndef HI_FPGA
    if (HI_SUCCESS != NVR_COMM_VO_HdmiStart(stVoPubAttr.enIntfSync))
    {
        SAMPLE_PRT("Start NVR_COMM_VO_HdmiStart failed!\n");
        goto END4_2;
    }
#endif

    stVoLayerAttr.bClusterMode = HI_FALSE;
    stVoLayerAttr.bDoubleFrame = HI_FALSE;
    stVoLayerAttr.enPixFormat = SAMPLE_PIXEL_FORMAT; 
	//获取通道实际显示显示尺寸（VO尺寸）
    s32Ret = NVR_COMM_VO_GetWH(stVoPubAttr.enIntfSync, \
        &stVoLayerAttr.stDispRect.u32Width, &stVoLayerAttr.stDispRect.u32Height, &stVoLayerAttr.u32DispFrmRt);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        goto  END4_2;
    }
	//设置各个通道拼接后要输出图像的尺寸
    stVoLayerAttr.stImageSize.u32Width = stVoLayerAttr.stDispRect.u32Width;
    stVoLayerAttr.stImageSize.u32Height = stVoLayerAttr.stDispRect.u32Height;
	stVoLayerAttr.u32DispFrmRt = 60 ;
    s32Ret = NVR_COMM_VO_StartLayer(VoLayer, &stVoLayerAttr);
    if(s32Ret != HI_SUCCESS)
    {		
        SAMPLE_PRT("NVR_COMM_VO_StartLayer fail for %#x!\n", s32Ret);
        goto END4_3;
    }	
	 SAMPLE_VO_MODE_E enmod = VO_MODE_BUTT ;
	 if (vdcnt == 1){ enmod = VO_MODE_1MUX; };
	 if (1<vdcnt && vdcnt<=4 ){ enmod =VO_MODE_4MUX; };
	 if(4<vdcnt && vdcnt<=9 ) {enmod = VO_MODE_9MUX; };
	 if( 9<vdcnt && vdcnt<= 16 ){ enmod = VO_MODE_16MUX;};
	 if(16<vdcnt && vdcnt<=25) {enmod =VO_MODE_25MUX;};
	 if(25<vdcnt && vdcnt<=36) {enmod =VO_MODE_36MUX;} ;
	 if(36<vdcnt && vdcnt<=64) {enmod =VO_MODE_64MUX;} ;
	 //设置图层排列方式
    s32Ret = NVR_COMM_VO_StartChn(VoLayer, enmod);
    if(s32Ret != HI_SUCCESS)
    {		
        SAMPLE_PRT("vdec bind vpss fail for %#x!\n", s32Ret);
        goto END4_4;
    }

    /************************************************
    step6:  VDEC bind VPSS
    *************************************************/	
    for(i=0; i<u32GrpCnt; i++)
    {
    	s32Ret = NVR_COMM_VDEC_BindVpss(i, i);
    	if(s32Ret != HI_SUCCESS)
    	{	    
            SAMPLE_PRT("vdec bind vpss fail for %#x!\n", s32Ret);
            goto END5;
    	}	
    }
    	
    /************************************************
    step7:  VPSS bind VO
    *************************************************/
    for(i=0; i<u32GrpCnt; i++)
    {
        s32Ret = NVR_COMM_VO_BindVpss(VoLayer, i, i, VPSS_CHN0);
        if(s32Ret != HI_SUCCESS)
        {	    
            SAMPLE_PRT("vpss bind vo fail for %#x!\n", s32Ret);
            goto END6;
        }	
    }	

    /************************************************
    step8:  send stream to VDEC
    *************************************************/
 
goto  END0 ;

END6:

    for(i=0; i<u32GrpCnt; i++)
    {
        s32Ret = NVR_COMM_VO_UnBindVpss(VoLayer, i, i, VPSS_CHN0);
        if(s32Ret != HI_SUCCESS)
        {	    
            SAMPLE_PRT("vpss unbind vo fail for %#x!\n", s32Ret);
        }	
    }		

END5:
    for(i=0; i<u32GrpCnt; i++)
    {
        s32Ret = NVR_COMM_VDEC_UnBindVpss(i, i);
        if(s32Ret != HI_SUCCESS)
        {	    
            SAMPLE_PRT("vdec unbind vpss fail for %#x!\n", s32Ret);
        }	
    }

END4_4:
    NVR_COMM_VO_StopChn(VoLayer, VO_MODE_4MUX);	
END4_3: 
    NVR_COMM_VO_StopLayer(VoLayer);
END4_2: 
#ifndef HI_FPGA
    NVR_COMM_VO_HdmiStop();
#endif
END4_1:
    NVR_COMM_VO_StopDev(VoDev);

END3:
    NVR_COMM_VPSS_Stop(u32GrpCnt, VPSS_CHN0);

END2:
    NVR_COMM_VDEC_Stop(u32VdCnt);		
	
END1:
    NVR_COMM_SYS_Exit();	

END0:
    return s32Ret;
}


int main(int argc, char *argv[])
{ 
    HI_S32 s32Ret = HI_SUCCESS;
    int ch;
    HI_BOOL bExit = HI_FALSE;
    ps = (NVRThreadParam*)malloc(sizeof(NVRThreadParam)*64) ;
	TOURI_S  *devlist ;
    signal(SIGINT, NVR_VDEC_HandleSig);
    signal(SIGTERM, NVR_VDEC_HandleSig);
	// Search IPC addrs .........
	devlist =  GetRTSPURI();
	printf("FIND : %d IPC\r\n",devlist->devnum);
	printf("Create Menu Number : [1 - 64] \r\n");
	scanf("%d",&ch);

int i ;
for (i=0 ;i <ch ; i++)
{ printf("THIS %d Group : \r\n ",i+1);
	int chs = 0 ;
	int chp = 0 ;
	int usr = 0 ;
	printf("IF Using USER URI Please Enter 1 , Using IPC URI please Enter 0 ,  Loss This menu please Enter 3  default 0 \r\n");
	scanf("%d",&usr);
	if ( usr == 1 )
	{
		char usuri[254]={'\0'};
		printf("Please enter uri :\r\n");
		scanf("%s",usuri);
		memset((ps+i)->uri,'\0',254);
		strcpy((ps+i)->uri,usuri);
		printf("User Uri : %s \r\n",(ps+i)->uri);
		(ps+i)->s32ChnId = i ;
                (ps+i)->s32MilliSec = 0;
                (ps+i)->u64PtsInit = 0 ;
	} 
	else if(usr == 0 )
	{
	printf("choose Dev number: [1]-[%d]\r\n",devlist->devnum);
	scanf("%d",&chs);
	if (chs<1 || chs > devlist->devnum)
	{
		printf("choose Dev number: [1]-[%d]\r\n",devlist->devnum);
		scanf("%d ",&chs);
	}
	printf("THIS DEV Have %d Profile stream : \r\n ",(devlist+chs-1)->profilenum);
	for (int cd=0 ; cd <(devlist+chs-1)->profilenum ; cd++)
	{
		printf("Stream NAME : %s \r\n",(devlist+chs-1)->profile_name[cd]);
	}
	printf("Choose Profile Number: [1]-[%d]\r\n",(devlist+chs-1)->profilenum);
	scanf("%d",&chp);
	if (chp <1 || chp >(devlist+chs-1)->profilenum )
	{
		printf("Choose Profile Number: [1]-[%d]\r\n",(devlist+chs-1)->profilenum);
		scanf("%d",&chp);
	}
	int lop =8 ;
	do {
	  lop ++ ;
	if ( (devlist+chs-1)->profile_uri[chp-1][lop] ==':')
	{
		memset((ps+i)->uri,'\0',254);
		strcpy((ps+i)->uri , (devlist+chs-1)->profile_uri[chp-1]);
		printf("Code %d  Group uri : %s \r\n",i+1,(ps+i)->uri);
		(ps+i)->s32ChnId = i ; //default display menu quest group set number  
		//default other  Dcoder Set pram .....
		(ps+i)->s32MilliSec = 0;
		(ps+i)->u64PtsInit = 0 ;
		break;
	}
	if ((devlist+chs-1)->profile_uri[chp-1][lop] == '/' )
	{
		char tmpaddr[254];
		memset(tmpaddr,'\0',254);
		char port[6];
		memset(port,'\0',6);
		port[0]=':';
		printf("The URI: %s No port!!!!!!!\r\n",(devlist+chs-1)->profile_uri[chp-1]);
		printf("Please  input IPC Stream Port:[Default 554....]!\r\n");
		scanf("%s",&port[1]);
		memcpy(tmpaddr,(devlist+chs-1)->profile_uri[chp-1],lop);
		memcpy(tmpaddr+(lop),port,strlen(port));
		memcpy(tmpaddr+((lop)+(strlen(port))),(devlist+chs-1)->profile_uri[chp-1]+lop,strlen((devlist+chs-1)->profile_uri[chp-1])-lop);
		strcpy((ps+i)->uri , tmpaddr);
		printf("Code %d  Group uri : %s \r\n",i+1,(ps+i)->uri);
                (ps+i)->s32ChnId = i ; //default display menu quest group set number  ^M
                //default other  Dcoder Set pram .....^M
                (ps+i)->s32MilliSec = 0;
                (ps+i)->u64PtsInit = 0 ;

		break;
	}

  }while(1);
       
             printf("Set Grop %d OK \r\n ",i);

 	}else
	{
		(ps+i)->s32ChnId = -1;
		printf("Next Menu ......\r\n");
	
	}
}
printf("Init DECODE SYS>>>\r\n ");
   s32Ret = NVR_INIT_VDEC(ch , 0);    
//begin start Stream  To decoder choose ....
  pthread_create(&VdecThread[0], 0, pstRTSPThread, (HI_VOID *)&ch);
  pthread_exit(NULL);
    s32Ret = NVR_INIT_VDEC(ch , 1); 

    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

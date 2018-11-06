/********************************定义链接live555 接口函数*********************************************/
#ifndef LINKLive555
#define LINKLive555
#include "pthread.h"
#include<unistd.h>  
#include<fcntl.h>  
#include<sys/types.h>  
#include<sys/stat.h>  
#include<sys/mman.h>  
#include<string.h>  
#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif
// 设置单个帧（I、P、B）最大数据大小（小于等于320K）
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 512000
       pthread_t live555_t ;
       pthread_mutex_t live555_mutex ;
	
	
	typedef struct nvrVdecThreadParam
	{
		char uri[254];
		int s32ChnId;	//要显示的通道
		int s32MilliSec ;
		int s32IntervalTime  ;
		double 	u64PtsInit  ;
	}NVRThreadParam;
NVRThreadParam *ps ;
int  firstFrameflag ;

 unsigned char* pstream ; 
/*** This func is live555 and mpp devc link **************/
 //void pstRTSPThread ( void *IPCparm );
 void pstRTSPThread (void * rt   );
 int LIVE_COMM_VDEC_SendStream_NVR( int chn ,unsigned char* Addr, unsigned int Len, double u64pts ,int s32Sec );
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif

/************************************************************************/
/*        Onvif Port func                                              */
/************************************************************************/
#ifndef  ONVIF_CLIENT
#define ONVIF_CLIENT
typedef struct PROFILESTREAMURI
{   char  IPCaddr[30] ; 
     int   devnum  ;
     int   profilenum  ;
	char  profile_name[5][20] ;
	char  profile_uri[5][254];
} TOURI_S;
/** 
 * @This-> Onvif 处理段
 * @搜索IPC设备及获取rtsp服务地址
 * @Next-> RTSP处理段（Live555）
 * @param char*  RTSPURI 
 * Source File  "getrtspuri.c"
 * @return  
 */ 
TOURI_S * GetRTSPURI(void);

#endif

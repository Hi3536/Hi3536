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
 * @This-> Onvif �����
 * @����IPC�豸����ȡrtsp�����ַ
 * @Next-> RTSP����Σ�Live555��
 * @param char*  RTSPURI 
 * Source File  "getrtspuri.c"
 * @return  
 */ 
TOURI_S * GetRTSPURI(void);

#endif

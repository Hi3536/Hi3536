#include "wsdd.h"  
#include <stdio.h>  
#include <ctype.h>  
#include "sha.h"
#include "soapStub.h"
#include "comeonvif_client.h"  
#include "wsseapi.h"
#include "threads.h"
#include <uuid.h>
/* 设备信息 */  
typedef struct USER_INFO  
{  
    char username[16];  
    char password[32];  
    char IpAddr[22];  
} UserInfo_S;  
  
/* 设备能力地址 */  
typedef struct CAPABILITIES_ADDR  
{  
    char Device[22];  
    char Imaging[22];  
	char Media[22];  
    char PTZ[22];  
} CapabilitiesAddr_S;  
  
/* 设备参数标识 */  
typedef struct PROFILE_TOKEN  
{  
    char *Media;  
    char *PTZConfiguration;  
} ProfileToken_S;  
  
/* 设备Uri信息 */  
typedef struct URI  
{  
    char *StreamUri;  
    char *SnapshotUri;  
} Uri_S;  
  
/** 初始化信息集合 **/  
typedef struct ONVIF_COMPLETE_INFO  
{  
    UserInfo_S *UserInfo;  
    CapabilitiesAddr_S *CapabilitiesAddr;  
    ProfileToken_S *ProfileToken;  
    Uri_S *Uri;  
} OnvifCompleteInfo_S;  

OnvifCompleteInfo_S stOnvifCompleteInfo[254];  
TOURI_S *Thisuri ;   
/** 
 * @brief ONVIF初始化一个soap对象 
 * @param header            SOAP_ENV__HEADER 
 * @param was_To            *wsa_To 
 * @param was_Action        *wsa_Action 
 * @param timeout           超时设置 
 * @return soap结构体地址 
 */

static struct soap* ONVIF_Initsoap(struct SOAP_ENV__Header *header, const char *was_To, const char *was_Action, int timeout, int toAuthenticateflag )  
{  
    //printf("> %s\n> %s\n> %s\n", pUserInfo->username, pUserInfo->password, pUserInfo->IpAddr);  
    struct soap *soap = NULL;  
//    unsigned char macaddr[6];  
//    char _HwId[1024];  
//    unsigned int Flagrand;  
	
    soap = soap_new();  
    if(soap == NULL)  
    {  
        printf("[%d]soap = NULL\n", __LINE__);  
        return NULL;  
    }  
     soap_set_namespaces( soap, namespaces);  
    //超过5秒钟没有数据就退出  
    if (timeout > 0)  
    {  
        soap->recv_timeout = timeout;  
        soap->send_timeout = timeout;  
        soap->connect_timeout = timeout;  
    }  
    else  
    {  
        //如果外部接口没有设备默认超时时间的话，我这里给了一个默认值10s  
        soap->recv_timeout    = 10;  
        soap->send_timeout    = 10;  
        soap->connect_timeout = 10;  
    } 
	 uuid_t uuid;  
	char guid_string[128];  
	uuid_generate(uuid);  
	uuid_unparse(uuid, guid_string);

	//初始化
	soap_default_SOAP_ENV__Header(soap, header);

	//相关参数写入句柄
	header->wsa__MessageID =(char *)soap_malloc(soap,128);
	memset(header->wsa__MessageID,'\0', 128);
	memcpy(header->wsa__MessageID, guid_string, strlen(guid_string));
    printf ("The header->wsa_messageId : %s \r\n",header->wsa__MessageID);
 //if toAuthenticateflag=1  Next to Authenticate //鉴权 
	if( toAuthenticateflag)  
    {  printf("This ssl pram set !\r\n");
        char uname[10]={'a','d','m','i','n','\0'}; 
		char passd[16]={'1','2','3','4','5','6','\0'};
		printf("Goto soap_wsse_add_UsernameTokenDigest\r\n");
	    int	result = soap_wsse_add_UsernameTokenDigest(soap, NULL, "admin", "123456");//返回为0 ，表示正确返回。
		if(SOAP_OK != result)
		{
			return result;
			printf("soap_wsse_add_UsernameTokenDigest Error !\r\n");
		}
         
    }  
    if (was_Action != NULL)  
    {  
        header->wsa__Action =(char *)malloc(1024);  
        memset(header->wsa__Action, '\0', 1024);  
        strncpy(header->wsa__Action, was_Action, 1024);//"http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe";  
    }  
    if (was_To != NULL)  
    {  
        header->wsa__To =(char *)malloc(1024);  
        memset(header->wsa__To, '\0', 1024);  
        strncpy(header->wsa__To,  was_To, 1024);//"urn:schemas-xmlsoap-org:ws:2005:04:discovery";  
    }  
    soap->header = header;

  //  printf("EXIT Init soap ...\r\n"); 
    return soap;  
}  

/************************************************************************/
/* 组播模式搜索IPC地址                                                  */
/************************************************************************/

int ONVIF_ClientDiscovery(int * addrindex )  
{  
	int HasDev = 0;  //记录发现的设备个数
	int retval = SOAP_OK;  
	wsdd__ProbeType req;  
	struct __wsdd__ProbeMatches resp;  
	wsdd__ScopesType sScope;  
	struct SOAP_ENV__Header header;  
	struct soap* soap;  
	

	const char *was_To = "urn:schemas-xmlsoap-org:ws:2005:04:discovery";  
	const char *was_Action = "http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe";  
	//这个就是传递过去的组播的ip地址和对应的端口发送广播信息      
	const char *soap_endpoint = "soap.udp://239.255.255.250:3702/";  

	//这个接口填充一些信息并new返回一个soap对象，本来可以不用额外接口，  
	// 但是后期会作其他操作，此部分剔除出来后面的操作就相对简单了,只是调用接口就好  
	soap = ONVIF_Initsoap(&header, was_To, was_Action, 10, 0);//暂不考虑鉴权   
	soap_default_wsdd__ScopesType(soap, &sScope);  
	sScope.__item = "";  
	soap_default_wsdd__ProbeType(soap, &req);  
	req.Scopes = &sScope;  
	req.Types = "dn:NetworkVideoTransmitter"; //"dn:NetworkVideoTransmitter";  
	retval = soap_send___wsdd__Probe(soap, soap_endpoint, NULL, &req);  
	
	//发送组播消息成功后，开始循环接收各位设备发送过来的消息  
	while (retval == SOAP_OK)  
	{  
           
		retval = soap_recv___wsdd__ProbeMatches(soap, &resp);  
		if (retval == SOAP_OK)  
		{  
			if (soap->error)  
			{  
				printf("[%d]: recv error:%d,%s,%s\n", __LINE__, soap->error, *soap_faultcode(soap), *soap_faultstring(soap));  
				retval = soap->error;  
			}  
			else //成功接收某一个设备的消息  
			{  
				 
				if (resp.wsdd__ProbeMatches->ProbeMatch != NULL && resp.wsdd__ProbeMatches->ProbeMatch->XAddrs != NULL)  
				{  
					printf(" ################  recv  %d devices info #### \n", HasDev+1 );  
					printf("Target Service Address  : %s\r\n", resp.wsdd__ProbeMatches->ProbeMatch->XAddrs);  
					printf("Target EP Address       : %s\r\n", resp.wsdd__ProbeMatches->ProbeMatch->wsa__EndpointReference.Address);  
					printf("Target Type             : %s\r\n", resp.wsdd__ProbeMatches->ProbeMatch->Types);  
					printf("Target Metadata Version : %d\r\n", resp.wsdd__ProbeMatches->ProbeMatch->MetadataVersion);
					//Create pram struct 
					stOnvifCompleteInfo[HasDev].UserInfo = (UserInfo_S *)malloc(sizeof(UserInfo_S));  
					stOnvifCompleteInfo[HasDev].CapabilitiesAddr = (CapabilitiesAddr_S *)malloc(sizeof(CapabilitiesAddr_S));  
					stOnvifCompleteInfo[HasDev].ProfileToken = (ProfileToken_S *)malloc(sizeof(ProfileToken_S));  
					stOnvifCompleteInfo[HasDev].ProfileToken->Media = (char *)malloc(sizeof(char));  
					stOnvifCompleteInfo[HasDev].ProfileToken->PTZConfiguration = (char *)malloc(sizeof(char));  
					
					//Init default pram 
					stOnvifCompleteInfo[HasDev].UserInfo->username[0] = 'a' ;
                    stOnvifCompleteInfo[HasDev].UserInfo->username[1] = 'd' ;
					stOnvifCompleteInfo[HasDev].UserInfo->username[2] = 'm' ;
					stOnvifCompleteInfo[HasDev].UserInfo->username[3] = 'i' ;
					stOnvifCompleteInfo[HasDev].UserInfo->username[4] = 'n' ;
					stOnvifCompleteInfo[HasDev].UserInfo->username[5] = '\0' ;
					stOnvifCompleteInfo[HasDev].UserInfo->password[0] = '\0';  
					stOnvifCompleteInfo[HasDev].CapabilitiesAddr->Device[0] = '\0';  
					stOnvifCompleteInfo[HasDev].CapabilitiesAddr->Imaging[0] = '\0';  
					stOnvifCompleteInfo[HasDev].CapabilitiesAddr->Media[0] = '\0';  
					stOnvifCompleteInfo[HasDev].CapabilitiesAddr->PTZ[0] = '\0';  
					stOnvifCompleteInfo[HasDev].ProfileToken->Media = "";  
					stOnvifCompleteInfo[HasDev].ProfileToken->PTZConfiguration = "";  
					
					char tmp[22]={'\0'};
                                        int p = 0 ;
					//提取IP格式地址XXX.XXX.XXX.XXX
					for(int i=7 ;i<254;i++ ,p++)
					{	//去除设备服务地址描述部分
						if (resp.wsdd__ProbeMatches->ProbeMatch->XAddrs ==NULL || *(resp.wsdd__ProbeMatches->ProbeMatch->XAddrs) == '\0' || \
							*(resp.wsdd__ProbeMatches->ProbeMatch->XAddrs+i) == '/' )
						  {
							  printf("Get Service IP END SOURCE IP: %s\r\n",resp.wsdd__ProbeMatches->ProbeMatch->XAddrs);
							  break;
						
						  }
						   tmp[p]=*(resp.wsdd__ProbeMatches->ProbeMatch->XAddrs+i);
					}
					strcpy(stOnvifCompleteInfo[HasDev].UserInfo->IpAddr,tmp);
					printf("Target%d IP Address  : %s\r\n", HasDev, stOnvifCompleteInfo[HasDev].UserInfo->IpAddr);
					strcpy((Thisuri+HasDev)->IPCaddr,tmp);
				}  
				HasDev ++; 
			}  
		}  
		else if (soap->error)  
		{  
			if (HasDev == 0)  
			{  
				printf("[%s][%d] Thers Device discovery or soap error: %d, %s, %s \n", __func__, __LINE__, soap->error, *soap_faultcode(soap), *soap_faultstring(soap)) ;
					retval = soap->error; 
				
			}  
			else  
			{  
				printf(" [%s]-[%d] Search end! It has Searched %d devices! \n", __func__, __LINE__, HasDev);  
				retval = 0;  
			}  
			break;  
		}  
	}  

	soap_destroy(soap);  
	soap_end(soap);  
	soap_free(soap);
	soap = NULL ;
	printf("Find %d device \r\n Exit ClientDiscovery!!\r\n",HasDev);
	*(addrindex) = HasDev ;
	return retval;  
}  
/************************************************************************/
/*            获取各能力集地址                                          */
/************************************************************************/
int ONVIF_GetCapabilities(int i)
{
	int retval = 0;
	struct soap *soap = NULL;
	struct _tds__GetCapabilities capa_req;  
	struct _tds__GetCapabilitiesResponse capa_resp;  

	struct SOAP_ENV__Header header;  
	char *soap_endpoint = (char *)malloc(256);  
	memset(soap_endpoint, '\0', 256);  
	 //   printf("The st.ipaddrs: %s\r\n",stOnvifCompleteInfo[i].UserInfo->IpAddr);  
	sprintf(soap_endpoint, "http://%s/onvif/device_service",stOnvifCompleteInfo[i].UserInfo->IpAddr);  
        printf("Target:%d :Capabilities soap_endpiont : %s\r\n",i+1,soap_endpoint);
	capa_req.Category = (enum tt__CapabilityCategory *)soap_malloc(soap, sizeof(int));  
	capa_req.__sizeCategory = 1;  
	*(capa_req.Category) = (enum tt__CapabilityCategory)5;  
	//此句也可以不要，因为在接口soap_call___tds__GetCapabilities中判断了，如果此值为NULL,则会给它赋值  
	const char *soap_action = "http://www.onvif.org/ver10/device/wsdl/GetCapabilities";  
             soap = ONVIF_Initsoap(&header, NULL, NULL, 5, 0);
	do  
	{  
		soap_call___tds__GetCapabilities(soap, soap_endpoint, soap_action, &capa_req, &capa_resp);  
		if (soap->error)  
		{  
			printf("[%s][%d]--->>> soap error: %d, %s, %s\n", __func__, __LINE__, soap->error, *soap_faultcode(soap), *soap_faultstring(soap));  
			retval = soap->error;  
			break;  
		}  
		else   //获取参数成功  
		{ 
			int p = 0;
			 char tmp[22] = {'\0'};
				printf("[%s][%d] Get capabilities success !\n", __func__, __LINE__);
				printf(" Media->XAddr=%s \n", capa_resp.Capabilities->Media->XAddr);
				//提取IP格式地址XXX.XXX.XXX.XXX
				for(int i=7 ;i<254;i++ ,p++)
				{	//去除设备服务地址描述部分
					if (capa_resp.Capabilities->Media->XAddr ==NULL || *(capa_resp.Capabilities->Media->XAddr) == '\0' || \
						*(capa_resp.Capabilities->Media->XAddr+i) == '/' )
					{
						//printf("Get Service IP ERROR %s",resp.wsdd__ProbeMatches->ProbeMatch->XAddrs);
						break;

					}
					tmp[p]=*(capa_resp.Capabilities->Media->XAddr+i);
				}
				//stOnvifCompleteInfo[HasDev].UserInfo->IpAddr = tmp ;
				strcpy(stOnvifCompleteInfo[i].CapabilitiesAddr->Media,tmp);
				printf("Target%d Media Service Address  : %s\r\n", i, stOnvifCompleteInfo[i].CapabilitiesAddr->Media);
			
			
				
			}  
			
		  
		printf("Profiles Get Procedure over\n");  


		
	}while(0);  

	free(soap_endpoint);  
	soap_endpoint = NULL;  
	soap_destroy(soap);
	soap_end(soap);  
	soap_free(soap);
	soap =NULL;
	return retval;  
 

}
/************************************************************************/
/*                      程序段内回收堆资源                              */
/************************************************************************/
/*
int DestroyMemSource(int i)
{
	if (i < 0 )
	{
		return -1;
	}
	int p=0;
	for (p;p<=i;p++)
	{
		
		free(stOnvifCompleteInfo[p].ProfileToken->PTZConfiguration);
		stOnvifCompleteInfo[p].ProfileToken->PTZConfiguration = NULL ;
		free(stOnvifCompleteInfo[p].ProfileToken->Media);
		stOnvifCompleteInfo[p].ProfileToken->Media =NULL ;
		free(stOnvifCompleteInfo[p].ProfileToken);
		stOnvifCompleteInfo[p].ProfileToken = NULL ;
		free(stOnvifCompleteInfo[p].CapabilitiesAddr);
		stOnvifCompleteInfo[p].CapabilitiesAddr =NULL ;
		free(stOnvifCompleteInfo[p].UserInfo);
		stOnvifCompleteInfo[p].UserInfo = NULL ;

	}
	return 0;
}
*/


/************************************* 
 * @
 * @brief ONVIF获取视频流地址 
 * @return 成功 0 / 失败 soap_error错误代码
 * @
 *************************************/  
int  ONVIF_GetRTSPStream(int i )  
{   
    int retval = 0;  
    struct soap *soap;  
    char *soap_endpoint = (char *)malloc(256);  
    const char *soap_action = NULL;  
    struct SOAP_ENV__Header header;  
  
    struct _trt__GetProfiles media_GetProfiles;  
    struct _trt__GetProfilesResponse media_GetProfilesResponse;  
    struct _trt__GetStreamUri media_GetStreamUri;  
    struct _trt__GetStreamUriResponse media_GetStreamUriResponse;  
 // printf("get rtsp ip addrs : %s \r\n",stOnvifCompleteInfo[i].CapabilitiesAddr->Media);
    /* 1 GetProfiles */  
    soap = ONVIF_Initsoap(&header, NULL, NULL, 5, 0);  
    memset(soap_endpoint, '\0', 256);  
	sprintf(soap_endpoint, "http://%s/onvif/Media", stOnvifCompleteInfo[i].CapabilitiesAddr->Media);
    int  is = 0; 
    int  ty = 0;
  
  do  {  
        soap_call___trt__GetProfiles(soap, soap_endpoint, soap_action, &media_GetProfiles, &media_GetProfilesResponse);  
		if (soap->error)  
        {  
            printf("[%s][%d]--->>> soap error: %d, %s, %s\n", __func__, __LINE__, soap->error, *soap_faultcode(soap), *soap_faultstring(soap));  
            retval = soap->error;  
            break;  
        }  
        else  
        { ty =media_GetProfilesResponse.__sizeProfiles ;
		(Thisuri+i)->profilenum = ty ;
			while(is<ty)
        {
                 // memset(Thisuri[is].streamnum,0,1);
		 // memset(Thisuri[is].profile_name,'\0',20);
		 // memset(Thisuri[is].profile_uri,'\0',254);
          printf("==== [ Media Profiles Response [\033[1;32m %d\033[0m ] ] ====\n"  
                 "> Name  :  %s\n"   
                 "> token :  %s\n\n"
	         ">size_profiles :  %d\n\n\n ", \  
                   is,(media_GetProfilesResponse.Profiles+is)->Name, \  
                  (media_GetProfilesResponse.Profiles+is)->token,media_GetProfilesResponse.__sizeProfiles);
		sprintf ((Thisuri+i)->profile_name[is],(media_GetProfilesResponse.Profiles+is)->Name,strlen((media_GetProfilesResponse.Profiles+is)->Name));
		
		  is ++ ;
		}	
        }
		
    }while(0); 


   	
/**  GetRtspStreamuri     **/


   soap = ONVIF_Initsoap(&header, NULL, NULL, 5, 0);  
 
 //   memset(soap_endpoint, '\0', 256);  
 //   sprintf(soap_endpoint, "http://%s/onvif/Media", stOnvifCompleteInfo[i].CapabilitiesAddr->Media);  
    is = 0 ; 
	media_GetStreamUri.StreamSetup = (struct tt__StreamSetup *)soap_malloc(soap, sizeof(struct tt__StreamSetup));  
	media_GetStreamUri.StreamSetup->Transport = (struct tt__Transport *)soap_malloc(soap, sizeof(struct tt__Transport)); 
    do  
    {   
		    media_GetStreamUri.StreamSetup->Stream = 0; //0: unMulticast 1:Multicast 
		    media_GetStreamUri.StreamSetup->Transport->Protocol = 0;  //0:UDP 1:TCP 2:RTSP 3:HTTP
		    media_GetStreamUri.StreamSetup->Transport->Tunnel = 0;  
		    media_GetStreamUri.StreamSetup->__size = 1;  
		    media_GetStreamUri.StreamSetup->__any = NULL;  
		    media_GetStreamUri.StreamSetup->__anyAttribute = NULL;  
		    media_GetStreamUri.ProfileToken = (media_GetProfilesResponse.Profiles+is)->token;
 
        soap_call___trt__GetStreamUri(soap, soap_endpoint, soap_action, &media_GetStreamUri, &media_GetStreamUriResponse);  
        if (soap->error)  
        {  
            printf("[%s][%d]--->>> soap error: %d, %s, %s\n", __func__, __LINE__, soap->error, *soap_faultcode(soap), *soap_faultstring(soap));  
            retval = soap->error;  
            break;  
        }  
        else  
		{    
           printf("==== [ [\033[1;32m %s\033[0m ] Stream Uri Response ] ====\n"  
                 "> MediaUri :\n\t%s\n",\   
                (media_GetProfilesResponse.Profiles+is)->Name ,media_GetStreamUriResponse.MediaUri->Uri);  
            printf("[\033[1;32m success\033[0m ] Get Stream Uri!\n");
			sprintf ((Thisuri+i)->profile_uri[is],media_GetStreamUriResponse.MediaUri->Uri,strlen(media_GetStreamUriResponse.MediaUri->Uri));
                   printf ("Thisuri is pram: mame : %s\r\n stream: %s\r\n devnum: %d\r\n",(Thisuri+i)->profile_name[is],(Thisuri+i)->profile_uri[is],(Thisuri+i)->devnum);
			  is ++ ;
		 
#if 0   // 此处可使用ffplay播放视频流, 请将stUserInfo结构体内容替换成相应的信息  
            char *rtspStreamUri = (char*)malloc(sizeof(char));  
            strncpy(rtspStreamUri, media_GetStreamUriResponse.MediaUri->Uri, 7);  
            strcat(rtspStreamUri, stUserInfo->username);  
            strcat(rtspStreamUri, ":");  
            strcat(rtspStreamUri, stUserInfo->password);  
            strcat(rtspStreamUri, "@");  
            strcat(rtspStreamUri, media_GetStreamUriResponse.MediaUri->Uri + 7);  
            printf("> Completing Uri :\n\t%s\n\n", rtspStreamUri);  
  
            char buf[1024];  
            sprintf(buf, "ffplay %s", rtspStreamUri);  
            system(buf);  
#endif 	
        } 
     
    }while(is<ty);
//	 rtspuri = &Thisuri ;
 //       printf ("rtspuri is pram: mame : %s\r\n stream: %s\r\n devnum: %d\r\n",(*rtspuri)->profile_name,(*rtspuri)->profile_uri,(*rtspuri)->streamnum);
	free(soap_endpoint);  
	soap_endpoint = NULL;  
	soap_destroy(soap); 
	soap_end(soap);  
	soap_free(soap);
	soap =NULL ;
    return retval;  
}  
  
/** 
 * @This-> Onvif 处理段
 * @搜索IPC设备及获取rtsp服务地址
 * @Next-> RTSP处理段（Live555）
 * @return  DEV prams
 */  
TOURI_S *  GetRTSPURI(void )
{    
    /* 获取在线IPC设备，选择需要连接的设备 */  
      int desflag = 0;
      int devnums = 0 ;
	  Thisuri =(TOURI_S *)malloc(sizeof(TOURI_S)*64);
	if (ONVIF_ClientDiscovery(&devnums) != 0)
	{
		printf("ERROR Discovery Dives [%s][%d]--->>> get server failed!\n", __func__, __LINE__);
		Thisuri->devnum = 0 ;
                return Thisuri ;
	}
	printf("FIND IPC ID number : [%d]:\r\n",devnums);
	
	for(desflag ; desflag<devnums ; desflag++)
	{
		(Thisuri+desflag)->devnum = devnums ;
	if (ONVIF_GetCapabilities(desflag) != 0)  
	{  
		printf("[%s][%d]--->>> get Capabilities Media addrs failed!\n", __func__, __LINE__);  
	      printf ("DEV: %d Get Capabilities  Address ERROR!!\r\n GOTO NEXT  IPC",desflag);	
		  (Thisuri+desflag)->profilenum = 0 ;
		  (Thisuri+desflag)->profile_name[0][0]='\0';
		  (Thisuri+desflag)->profile_uri[0][0]='\0';
             continue; 
	}
    if (ONVIF_GetRTSPStream(desflag) != 0)  
    {  
        printf("[%s][%d]--->>> get RTSP stream failed!\n", __func__, __LINE__); 
		printf("DEV: %d Get RTSPStream URI Error !\r\n GOTO NEXT IPC",desflag);
		(Thisuri+desflag)->profile_name[0][0]='\0';
		(Thisuri+desflag)->profile_uri[0][0]='\0';
          continue;
    }
	}
 printf ("Exit :Search IPC func........< ONVIF >\r\n");
	return Thisuri;
}

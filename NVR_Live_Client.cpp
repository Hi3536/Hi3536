#include "NVR_live_Client.h"
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "pthread.h"
#include <iostream>
#include <string>
using namespace std ;
//#include "sample_comm.h"
// Begin by setting up our usage environment:(This Func Is Start Live555 Receive Entye)
// firstFrame = true ;
// RTSP 'response handlers':

#define RTSP_CLIENT_VERBOSITY_LEVEL 1// by default, print verbose output from each "RTSPClient"
// A function that outputs a string that identifies each stream (for debugging output). Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const RTSPClient& rtspClient) {
	return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}
// A function that outputs a string that identifies each subsession (for debugging output). Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const MediaSubsession& subsession) {
	return env << subsession.mediumName() << "/" << subsession.codecName();
}
void usage(UsageEnvironment& env, char const* progName) {
	env << "Usage: " << progName << "  ... \n";
	env << "\t(where each  is a \"rtsp://\" URL)\n";
}
void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);

// Other event handler functions:
void subsessionAfterPlaying(void* clientData); // called when a stream's subsession (e.g., audio or video substream) ends
void subsessionByeHandler(void* clientData); // called when a RTCP "BYE" is received for a subsession
void streamTimerHandler(void* clientData);
// called at the end of a stream's expected duration (if the stream has not already signaled its end using a RTCP "BYE")

// The main streaming routine (for each "rtsp://" URL):
//void openURL(UsageEnvironment& env, char const* progName, char const* rtspURL);

// Used to iterate through each stream's 'subsessions', setting up each one:
void setupNextSubsession(RTSPClient* rtspClient);

// Used to shut down and close a stream (including its "RTSPClient" object):
void shutdownStream(RTSPClient* rtspClient, int exitCode = 1);

class StreamClientState {
public:
	StreamClientState();
	virtual ~StreamClientState();

public:
	MediaSubsessionIterator* iter;
	MediaSession* session;
	MediaSubsession* subsession;
	TaskToken streamTimerTask;
	double duration;
};

// If you're streaming just a single stream (i.e., just from a single URL, once), then you can define and use just a single
// "StreamClientState" structure, as a global variable in your application.  However, because - in this demo application - we're
// showing how to play multiple streams, concurrently, we can't do that.  Instead, we have to have a separate "StreamClientState"
// structure for each "RTSPClient".  To do this, we subclass "RTSPClient", and add a "StreamClientState" field to the subclass:

class ourRTSPClient: public RTSPClient {
public:
	   static ourRTSPClient* createNew(UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel = 0,
		char const* applicationName = NULL,
		portNumBits tunnelOverHTTPPortNum = 0);

protected:
	ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);
	// called only by createNew();
	virtual ~ourRTSPClient();

public:
	unsigned int m_nID;
	StreamClientState scs;
	int * setdog;
};

class CRTSPSession 
{
public:
	CRTSPSession();
	virtual ~CRTSPSession();
	int startRTSPClient(char const* progName, char const* rtspURL, int debugLevel,int groups);
	int stopRTSPClient();
	int openURL(UsageEnvironment& env, char const* progName, char const* rtspURL, int debugLevel);
	RTSPClient* m_rtspClient;
	char eventLoopWatchVariable;
	pthread_t tid;
	pthread_t wtid;
	bool m_running;
	string m_rtspUrl;
	string m_progName;
	int m_debugLevel;
	static void *rtsp_thread_fun (void *param);
	void rtsp_fun();
	int  setgroup(int grous);
	int getgroup();
       static void* StartWatch(void * is);
        void streamwatch();
private:
	int group;
	int doglive = 2;
	
};
int CRTSPSession::setgroup(int grous)
{
	this->group = grous ;
}
void* CRTSPSession::StartWatch(void *is)
{
      CRTSPSession * object = (CRTSPSession *)is;
      object->streamwatch();
}
void CRTSPSession::streamwatch()
{
BEGIN:
	this->doglive =2;
	sleep(3);
	while (1)
	{
		this->doglive =this->doglive-1;
	   sleep(1);
	   
	   if(this->doglive <= 0)
	   {  std::cout<<"DOG Done ....."<<"The"<<this->group<<"GROUP Stream Timeout Next Recover The Stream...."<<std::endl;
	     int times =5;
	      while(times)
		     {
				 std::cout<<"After "<<times<<"Second To Recover Stream...."<<std::endl;
				 times--;
				 sleep(1);
		     }
		  pthread_cancel ((this->tid)); 
		  pthread_join((this->tid),NULL);
		  this->startRTSPClient("PGLIVE2",(ps+this->group)->uri,1,this->group);
		  sleep(3);
		  goto BEGIN ;
		  }
	
	 }

}
	


int CRTSPSession::getgroup()
{
	return this->group ;
}
CRTSPSession::CRTSPSession()
{
	m_rtspClient = NULL;
	m_running = false;
	eventLoopWatchVariable = 0;
}
CRTSPSession::~CRTSPSession()
{
}
int CRTSPSession::startRTSPClient(char const* progName, char const* rtspURL, int debugLevel, int groups)
{
	m_progName = progName;
	m_rtspUrl = rtspURL;
	m_debugLevel = debugLevel;
	eventLoopWatchVariable = 0;
	this->group = groups ;
	int r = pthread_create(&tid, NULL, rtsp_thread_fun, this);
	if (r)
	{
		perror ("pthread_create()");
		return -1;
	}
       printf("Create :%d Group stream OK!!!\r\n",groups);
	return 0;
}
int CRTSPSession::stopRTSPClient()
{
	eventLoopWatchVariable = 1;
	return 0;
}
void *CRTSPSession::rtsp_thread_fun(void *param)
{
	CRTSPSession *pThis = (CRTSPSession*)param;
	pThis->rtsp_fun ();
	return NULL;
}
void CRTSPSession::rtsp_fun()
{
	//::startRTSP(m_progName.c_str(), m_rtspUrl.c_str(), m_ndebugLever);
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);
	if (openURL(*env, m_progName.c_str(), m_rtspUrl.c_str(), m_debugLevel) == 0)
	{
		//m_nStatus = 1;
		env->taskScheduler().doEventLoop(&eventLoopWatchVariable);

		m_running = false;
		eventLoopWatchVariable = 0;

		if (m_rtspClient)
		{
			shutdownStream(m_rtspClient,0);
		}
		m_rtspClient = NULL;
	}

	env->reclaim(); 
	env = NULL;
	delete scheduler; 
	scheduler = NULL;
	//m_nStatus = 2;
}
int CRTSPSession::openURL(UsageEnvironment& env, char const* progName, char const* rtspURL, int debugLevel)
{
	m_rtspClient = ourRTSPClient::createNew(env, rtspURL, debugLevel, progName);
	if (m_rtspClient == NULL) 
	{
		env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": " << env.getResultMsg() << "\n";
		return -1;
	}
	((ourRTSPClient*)m_rtspClient)->m_nID = this->getgroup();
	((ourRTSPClient*)m_rtspClient)->setdog = &(this->doglive) ;
	m_rtspClient->sendDescribeCommand(continueAfterDESCRIBE); 
	return 0;
}

// A function that outputs a string that identifies each stream (for debugging output).  Modify this if you wish:
//UsageEnvironment& operator<<(UsageEnvironment& env, const RTSPClient& rtspClient) {
//	return env << "[URL:\"" << rtspClient.url() << "\"]: ";
//}

// A function that outputs a string that identifies each subsession (for debugging output).  Modify this if you wish:
//UsageEnvironment& operator<<(UsageEnvironment& env, const MediaSubsession& subsession) {
	//return env << subsession.mediumName() << "/" << subsession.codecName();
//}

//void usage(UsageEnvironment& env, char const* progName) {
	//env << "Usage: " << progName << " <rtsp-url-1> ... <rtsp-url-N>\n";
//	env << "\t(where each <rtsp-url-i> is a \"rtsp://\" URL)\n";
//}

static unsigned rtspClientCount = 0; // Counts how many streams (i.e., "RTSPClient"s) are currently in use.

void pstRTSPThread(void * rt)
{
	int  i =*((int*)rt); 
	//It's app func point port
   firstFrameflag= 1;
  char eventLoopWatchVariable = 0;
  for (int j=0 ; j<i ; j++)
  { 
	  if ((ps+j)->s32ChnId == -1)
	  {
		  continue;
	  }
  rtspClientCount ++ ;
  CRTSPSession* pRtsp = new CRTSPSession;
  if (pRtsp->startRTSPClient("PGLIVE", (ps+j)->uri,RTSP_CLIENT_VERBOSITY_LEVEL,j))
  {
	  std::cout<<"ERROR!"<<j<<"CreateRtspsession failt !!!!" <<endl;
	  delete pRtsp;
	  pRtsp = NULL;
	  rtspClientCount -- ;

  }
      pthread_create(&(pRtsp->wtid),NULL, (pRtsp->StartWatch),(void*)pRtsp);
     usleep(10000);

  }
  
  // All subsequent activity takes place within the event loop:
  pthread_exit(NULL);
    // This function call does not return, unless, at some point in time, "eventLoopWatchVariable" gets set to something non-zero.

  // If you choose to continue the application past this point (i.e., if you comment out the "return 0;" statement above),
  // and if you don't intend to do anything more with the "TaskScheduler" and "UsageEnvironment" objects,
  // then you can also reclaim the (small) memory used by these objects by uncommenting the following code:
  /*
    env->reclaim(); env = NULL;
    delete scheduler; scheduler = NULL;
  */
// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:
};

// Define a data sink (a subclass of "MediaSink") to receive the data for each subsession (i.e., each audio or video 'substream').
// In practice, this might be a class (or a chain of classes) that decodes and then renders the incoming audio or video.
// Or it might be a "FileSink", for outputting the received data into a file (as is done by the "openRTSP" application).
// In this example code, however, we define a simple 'dummy' sink that receives incoming data, but does nothing with it.

class DummySink: public MediaSink {
public:
  static DummySink* createNew(UsageEnvironment& env, MediaSubsession& subsession,char const* streamId = NULL,int sinkgroup=0,int *setdog=NULL); // identifies the stream itself (optional)
  int  *setdog;
private:
  DummySink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId , int sinkgroup, int *setdog) ;
    // called only by "createNew()"
  virtual ~DummySink();

  static void afterGettingFrame(void* clientData, unsigned frameSize,
                                unsigned numTruncatedBytes,
				struct timeval presentationTime,
                                unsigned durationInMicroseconds);
  void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
			 struct timeval presentationTime, unsigned durationInMicroseconds);
int sinkgroup ;
unsigned char *h264 ;
unsigned  char head[4] = {0x00, 0x00, 0x00, 0x01};
private:
  // redefined virtual functions:
  virtual Boolean continuePlaying();

private:
  u_int8_t* fReceiveBuffer;
  MediaSubsession& fSubsession;
  char* fStreamId;
};
// Implementation of the RTSP 'response handlers':
void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
      delete[] resultString;
      break;
    }

    char* const sdpDescription = resultString;
    env << *rtspClient << "Got a SDP description:\n" << sdpDescription << "\n";

    // Create a media session object from this SDP description:
    scs.session = MediaSession::createNew(env, sdpDescription);
    delete[] sdpDescription; // because we don't need it anymore
    if (scs.session == NULL) {
      env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
      break;
    } else if (!scs.session->hasSubsessions()) {
      env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
      break;
    }

    // Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
    // calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
    // (Each 'subsession' will have its own data source.)
    scs.iter = new MediaSubsessionIterator(*scs.session);
    setupNextSubsession(rtspClient);
    return;
  } while (0);

  // An unrecoverable error occurred with this stream.
  shutdownStream(rtspClient);
}

// By default, we request that the server stream its data using RTP/UDP.
// If, instead, you want to request that the server stream via RTP-over-TCP, change the following to True:
#define REQUEST_STREAMING_OVER_TCP False

void setupNextSubsession(RTSPClient* rtspClient) {
  UsageEnvironment& env = rtspClient->envir(); // alias
  StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias
  
  scs.subsession = scs.iter->next();
  if (scs.subsession != NULL) {
    if (!scs.subsession->initiate()) {
      env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
      setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
    } else {
      env << *rtspClient << "Initiated the \"" << *scs.subsession << "\" subsession (";
      if (scs.subsession->rtcpIsMuxed()) {
	env << "client port " << scs.subsession->clientPortNum();
      } else {
	env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
      }
      env << ")\n";

      // Continue setting up this subsession, by sending a RTSP "SETUP" command:
      rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);
    }
    return;
  }

  // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
  if (scs.session->absStartTime() != NULL) {
    // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
    rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
  } else {
    scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
    rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
  }
}

void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
  do{
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";
      break;
    }

    env << *rtspClient << "Set up the \"" << *scs.subsession << "\" subsession (";
    if (scs.subsession->rtcpIsMuxed()) {
      env << "client port " << scs.subsession->clientPortNum();
    } else {
      env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
    }
    env << ")\n";

    // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
    // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
    // after we've sent a RTSP "PLAY" command.)

    scs.subsession->sink = DummySink::createNew(env, *scs.subsession, rtspClient->url(),((ourRTSPClient*)rtspClient)->m_nID ,((ourRTSPClient*)rtspClient)->setdog);
      // perhaps use your own custom "MediaSink" subclass instead
    if (scs.subsession->sink == NULL) {
      env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession
	  << "\" subsession: " << env.getResultMsg() << "\n";
      break;
    }

    env << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
    scs.subsession->miscPtr = rtspClient; // a hack to let subsession handler functions get the "RTSPClient" from the subsession 
    scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
				       subsessionAfterPlaying, scs.subsession);
    // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
    if (scs.subsession->rtcpInstance() != NULL) {
      scs.subsession->rtcpInstance()->setByeHandler(subsessionByeHandler, scs.subsession);
    }
  } while (0);
  delete[] resultString;

  // Set up the next subsession, if any:
  setupNextSubsession(rtspClient);
}

void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
  Boolean success = False;

  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
      break;
    }
    // Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
    // using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
    // 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
    // (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
    if (scs.duration > 0) {
      unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
      scs.duration += delaySlop;
      unsigned uSecsToDelay = (unsigned)(scs.duration*1000000);
      scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
    }

    env << *rtspClient << "Started playing session";
    if (scs.duration > 0) {
      env << " (for up to " << scs.duration << " seconds)";
    }
    env << "...\n";

    success = True;
  } while (0);
  delete[] resultString;

  if (!success) {
    // An unrecoverable error occurred with this stream.
    shutdownStream(rtspClient);
  }
}
// Implementation of the other event handlers:

void subsessionAfterPlaying(void* clientData) {
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

  // Begin by closing this subsession's stream:
  Medium::close(subsession->sink);
  subsession->sink = NULL;

  // Next, check whether *all* subsessions' streams have now been closed:
  MediaSession& session = subsession->parentSession();
  MediaSubsessionIterator iter(session);
  while ((subsession = iter.next()) != NULL) {
    if (subsession->sink != NULL) return; // this subsession is still active
  }

  // All subsessions' streams have now been closed, so shutdown the client:
  shutdownStream(rtspClient);
}

void subsessionByeHandler(void* clientData) {
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
  UsageEnvironment& env = rtspClient->envir(); // alias

  env << *rtspClient << "Received RTCP \"BYE\" on \"" << *subsession << "\" subsession\n";

  // Now act as if the subsession had closed:
  subsessionAfterPlaying(subsession);
}

void streamTimerHandler(void* clientData) {
  ourRTSPClient* rtspClient = (ourRTSPClient*)clientData;
  StreamClientState& scs = rtspClient->scs; // alias

  scs.streamTimerTask = NULL;

  // Shut down the stream:
  shutdownStream(rtspClient);
}

void shutdownStream(RTSPClient* rtspClient, int exitCode) {
  UsageEnvironment& env = rtspClient->envir(); // alias
  StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

  // First, check whether any subsessions have still to be closed:
  if (scs.session != NULL) { 
    Boolean someSubsessionsWereActive = False;
    MediaSubsessionIterator iter(*scs.session);
    MediaSubsession* subsession;

    while ((subsession = iter.next()) != NULL) {
      if (subsession->sink != NULL) {
	Medium::close(subsession->sink);
	subsession->sink = NULL;

	if (subsession->rtcpInstance() != NULL) {
	  subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
	}

	someSubsessionsWereActive = True;
      }
    }

    if (someSubsessionsWereActive) {
      // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
      // Don't bother handling the response to the "TEARDOWN".
      rtspClient->sendTeardownCommand(*scs.session, NULL);
    }
  }

  env << *rtspClient << "Closing the stream.\n";
  Medium::close(rtspClient);
    // Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.

  if (--rtspClientCount == 0) {
    // The final stream has ended, so exit the application now.
    // (Of course, if you're embedding this code into your own application, you might want to comment this out,
    // and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
    exit(exitCode);
  }
}


// Implementation of "ourRTSPClient":

ourRTSPClient* ourRTSPClient::createNew(UsageEnvironment& env, char const* rtspURL,
					int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum) {
  return new ourRTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

ourRTSPClient::ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
			     int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
  : RTSPClient(env,rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1) {
}

ourRTSPClient::~ourRTSPClient() {

}


// Implementation of "StreamClientState":

StreamClientState::StreamClientState()
  : iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0) {
}

StreamClientState::~StreamClientState() {
  delete iter;
  if (session != NULL) {
    // We also need to delete "session", and unschedule "streamTimerTask" (if set)
    UsageEnvironment& env = session->envir(); // alias

    env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
    Medium::close(session);
  }
}

// Implementation of "DummySink":

// Even though we're not going to be doing anything with the incoming data, we still need to receive it.
// Define the size of the buffer that we'll use:
//#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 320000

DummySink* DummySink::createNew(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId ,int sinkgroup ,int *setdog ) {
  return new DummySink(env, subsession, streamId , sinkgroup ,setdog);
}

DummySink::DummySink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId ,int sinkgroup ,int *setdog)
  : MediaSink(env),
    fSubsession(subsession) {
		this->sinkgroup = sinkgroup ;
		this->setdog =setdog ;
  fStreamId = strDup(streamId);
  fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
  h264 = new unsigned char[DUMMY_SINK_RECEIVE_BUFFER_SIZE+200];
   memset(this->h264 ,'\0',DUMMY_SINK_RECEIVE_BUFFER_SIZE+200);
   memcpy(this->h264 ,this->head , 4);
}

DummySink::~DummySink() {
  delete[] fReceiveBuffer;
  delete[] fStreamId;
  delete[] h264;
}

void DummySink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
				  struct timeval presentationTime, unsigned durationInMicroseconds) {
  DummySink* sink = (DummySink*)clientData;
  sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

// If you don't want to see debugging output for each received frame, then comment out the following line:
//#define DEBUG_PRINT_EACH_RECEIVED_FRAME  

void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
				  struct timeval presentationTime, unsigned /*durationInMicroseconds*/) {
  // We've just received a frame of data.  (Optionally) print out information about it:



#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
  if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
  envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";
  if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
  char uSecsStr[6+1]; // used to output the 'microseconds' part of the presentation time
  sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
  envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;
  if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
    envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
  }
//#ifdef DEBUG_PRINT_NPT
//  envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
//  #endif
  envir() << "\n";
#endif
  //todo one frame  
  //save to file  
 
 if((strcmp(fSubsession.mediumName(), "video") == 0 )/*&& (strcmp(fsubsession->codecName(),"H264") == 0)*/)  
  {       
	//  if(firstFrameflag == 1)  
	 // {
		//  printf("This First Frame ... write the Frame Flie ...\n");
		 // unsigned int num ;  
		 // SPropRecord *sps = parseSPropParameterSets(fSubsession.fmtp_spropparametersets(), num);  
		 // For H.264 video stream, we use a special sink that insert start_codes:  
	//	  struct timeval tv= {0,0};  
	//	  unsigned char start_code[4] = {0x00, 0x00, 0x00, 0x01};  
//  FILE *fp = fopen("./ipc.264", "a+b"); 
                   
//                   
//	  if(fp)  
//	  {  
//		  fwrite(start_code, 4, 1, fp);  
                          
//                        
//  fwrite(sps[0].sPropBytes, sps[0].sPropLength, 1, fp); 
                       
// 	  fwrite(sps[0].sPropBytes, sps[0].sPropLength, 1, fp);  
//		  fwrite(start_code, 4, 1, fp);  
            
//		  fwrite(sps[1].sPropBytes, sps[1].sPropLength, 1, fp);  
//fclose(fp);     
//	  fp = NULL;              
//       fwrite(pstream, ps->streamlen,1 ,fp);

//                        fclose(fp);  
//                       fp = NULL; 
		  //from NAL sps 
	//	  delete [] sps;  
//		  firstFrame = False;
         //         pthread_mutex_unlock(&live555_mutex);
         //         usleep(1500);
           //     loopflag = 1 ;
   //           printf("This First frame stream to vdec \r\n");
    //            memcpy(this->h264,start_code,4);
      //           memcpy((this->h264+4),fReceiveBuffer,frameSize);
		//LIVE_COMM_VDEC_SendStream_NVR((ps+this->sinkgroup)->s32ChnId,this->h264,frameSize+4 ,(ps+this->sinkgroup)->u64PtsInit,(ps+this->sinkgroup)->s32MilliSec );
		 //     usleep(5000);
          //      firstFrameflag = 0 ;
				// memset(this->h264,'\0',DUMMY_SINK_RECEIVE_BUFFER_SIZE+200);
        //        goto Nextframe ;

//	  }  
	//  FILE *fp = fopen("./ipc.264", "a+b");  
//	  if(fp)  
//	  {  
//		  fwrite(head, 4, 1, fp);  
//		  fwrite(fReceiveBuffer, frameSize, 1, fp);  
//	  fclose(fp);  
//		  fp = NULL;    
//		  printf("Write:  h.264 date ok !!!\r\n");    
//       SAMPLE_COMM_VDEC_SendStream_nvr(pstream, ps->streamlen, 0 );
             //    memcpy(this->h264,head,4);
                 memcpy((this->h264+4),fReceiveBuffer,frameSize);
             //    fwrite(h264,frameSize+4,1,fp);
        LIVE_COMM_VDEC_SendStream_NVR((ps+this->sinkgroup)->s32ChnId,this->h264,frameSize+4 ,(ps+this->sinkgroup)->u64PtsInit,(ps+this->sinkgroup)->s32MilliSec );
		*(this->setdog)=2;	 
		usleep(5000);
			 
		//	 memset(h264,'\0',DUMMY_SINK_RECEIVE_BUFFER_SIZE+200);
			 
  }
Nextframe:
  continuePlaying();
}
   
Boolean DummySink::continuePlaying() {
  if (fSource == NULL) return False; // sanity check (should not happen)

  // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
  fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
                        afterGettingFrame, this,
                        onSourceClosure, this);
  return True;
}

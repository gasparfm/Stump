/* @(#)stump.h
 */

#ifndef _STUMP_H
#define _STUMP_H 1

#if defined(__cplusplus) && __cplusplus >= 201103L
  #ifndef __CXX11
    #define __ITS_CXX11
  #endif
#endif

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <sstream>

/* What does C++11 do different?  */
#ifdef __ITS_CXX11
  #include <mutex>
  #include <thread>
  /* #warning "C++11 support not implemented yet" */

  #ifndef __MUTEX_T
#define __MUTEX_T std::mutex
  #endif

  #ifndef __MUTEX_LOCK
#define __MUTEX_LOCK(_mutex) _mutex.lock()
  #endif

  #ifndef __MUTEX_UNLOCK
#define __MUTEX_UNLOCK(_mutex) _mutex.unlock()
  #endif

  #ifndef __MUTEX_INIT
    #define __MUTEX_INIT(_mutex) 
  #endif

  #ifndef __THREAD_T
    #define __THREAD_T std::thread::id
  #endif

  #ifndef __F_THREAD_ID
    #define __F_THREAD_ID() std::this_thread::get_id()
  #endif

  using std::lock_guard;
#else

  /* If we are not using C++11 we must use pthread for mutexes. */
  #include <pthread.h>

  #ifndef __MUTEX_T
    #define __MUTEX_T pthread_mutex_t
  #endif

  #ifndef __MUTEX_LOCK
#define __MUTEX_LOCK(_mutex)  pthread_mutex_lock(&_mutex)
  #endif

  #ifndef __MUTEX_UNLOCK
#define __MUTEX_UNLOCK(_mutex)  pthread_mutex_unlock(&_mutex)
  #endif

  #ifndef __MUTEX_INIT
    #define __MUTEX_INIT(_mutex) pthread_mutex_init(&_mutex, NULL)
  #endif

  #ifndef __THREAD_T
    #define __THREAD_T pthread_t
  #endif

  #ifndef __F_THREAD_ID
    #define __F_THREAD_ID() pthread_self()
  #endif

#endif

class Stump
{
 public:
  typedef void (*TCallbackType1) ( const char *log);

  enum OutputType
  {
    OSTREAM,
    FILE,
    CALLBACK
  };

  struct MessageFormat
  {
    std::string logFormat;
    std::string dateFormat;
    std::string repeatLogFormat;

    MessageFormat(Stump *instance)
    {
      logFormat = instance->defaultLogFormat;
      dateFormat = instance->defaultDateFormat;
      repeatLogFormat = instance->defaultRepeatLogFormat;
    }
  MessageFormat(Stump *instance, std::string logFormat): logFormat(logFormat)
    {
      dateFormat = instance->defaultDateFormat;
      repeatLogFormat = instance->defaultRepeatLogFormat;
    }
  MessageFormat(Stump *instance, std::string logFormat, std::string dateFormat): logFormat(logFormat), dateFormat(dateFormat)
    {
      repeatLogFormat = instance->defaultRepeatLogFormat;
    }
  MessageFormat(Stump *instance, std::string logFormat, std::string dateFormat, std::string repeatLogFormat): logFormat(logFormat), dateFormat(dateFormat), repeatLogFormat(repeatLogFormat)
    {
    }
  };

  struct MessageBuffer
  {
    unsigned bufferId;		/* Mainly for internal use */
    std::string bufferName;	/* For external developers */
    long lastRepeat;
    time_t lastDate;
    std::string lastMessage;
    std::vector<std::string> buffer;
    int bufferSize;

    __MUTEX_T mutex;
    /* Buffer flush to... */
    OutputType output;
    std::ostream &ost;
    std::string file;		/* If apply */
    TCallbackType1 cbt1;

  MessageBuffer(std::ostream &os, int bufferSize=20): bufferSize(bufferSize), bufferId(Stump::bufferId++), ost(os)
    {
      output = OSTREAM;
      bufferName="Stump"+itoa(bufferId);
      __MUTEX_INIT(mutex);
    }
  MessageBuffer(std::ostream &os, std::string bufferName, int bufferSize=20): bufferName(bufferName), bufferSize(bufferSize), bufferId(Stump::bufferId++), ost(os)
    {
      output = OSTREAM;
      __MUTEX_INIT(mutex);
    }
  MessageBuffer(std::string file, int bufferSize=20): bufferSize(bufferSize), bufferId(Stump::bufferId++), file(file), ost(std::cerr) /* cerr won't be used */
    {
      output = FILE;
      bufferName="Stump"+itoa(bufferId);
      __MUTEX_INIT(mutex);
    }
  MessageBuffer(std::string file, std::string bufferName, int bufferSize=20): bufferName(bufferName), bufferSize(bufferSize), bufferId(Stump::bufferId++), file(file), ost(std::cerr)
    {
      output = FILE;
      __MUTEX_INIT(mutex);
    }
  };

  Stump();
  virtual ~Stump();

  void clearBuffers();
  void disableAll();
  void enableAll();
  void enable(std::string messageType);
  void disable(std::string messageType);
  void setStatus(std::string messageType, bool enabled);

  MessageBuffer *createBuffer(std::ostream &os, int bufferSize);
  MessageBuffer *createBuffer(std::ostream &os, std::string bufferName, int bufferSize);
  MessageBuffer *createBuffer(std::string file, int bufferSize);
  MessageBuffer *createBuffer(std::string file, std::string bufferName, int bufferSize);
  /* Create buffer con callback */

  void log(std::string msg, std::string logType="");
  void clearOutputs();
  void addOutput(std::string messageType, std::string typeLabel, MessageFormat messageFormat, int bufferSize=20);

  /* addOutput reusing buffers */
  void addOutput(std::string messageType, std::string typeLabel, MessageFormat messageFormat, std::string bufferName);

  /* ostream interface */
  template <typename T>
    Stump& operator<<(const T& x)
    {
      __MUTEX_LOCK(ostreamDataMutex);
      bool cond = ( (currentLog.tellp()==0) || (streamingThread != __F_THREAD_ID()) )/* Start logging */;
      __MUTEX_UNLOCK(ostreamDataMutex);

      if (cond)
	{
	  __MUTEX_LOCK(ostreamMutex);

	  /* Must store thread id, because several threads may stream logs at the same time. To avoid mixing data */
	  __MUTEX_LOCK(ostreamDataMutex);
	  streamingThread = __F_THREAD_ID();
	  __MUTEX_UNLOCK(ostreamDataMutex);
	}

      /* Adds stream to current log stream buffer */
      __MUTEX_LOCK(ostreamDataMutex);
      currentLog << x;
      long currentSize = currentLog.tellp();
      __MUTEX_UNLOCK(ostreamDataMutex);
      /* Checks current log stream buffer size. If 0 (nothing to write), if >=streamBufferMax (we must cut) */
      if (currentSize==0)
	{
	  __MUTEX_UNLOCK(ostreamMutex);
	}
      else if (currentSize>=streamBufferMax)
	
	(*this)<<Stump::crop<<Stump::endl;

      return *this;
    }

  Stump& operator<<(Stump& (*manipulator)(Stump&))
    {
        // call the function, and return it's value
        return manipulator(*this);
    }

  static Stump& crop(Stump& stump)
  {
    __MUTEX_LOCK(stump.ostreamDataMutex);
    stump.currentLog << stump.streamBufferCropMsg;
    __MUTEX_UNLOCK(stump.ostreamDataMutex);

    return stump;
  }

  static Stump& endl(Stump& stump)
  {
    stump.log(stump.currentLog.str(), stump.workingMessageType);

    /* Cleanup currentlog */
    __MUTEX_LOCK(stump.ostreamDataMutex);
    stump.currentLog.str("");
    __MUTEX_UNLOCK(stump.ostreamDataMutex);

    __MUTEX_UNLOCK(stump.ostreamMutex);
    return stump;
  }

  // this is the type of std::cout
  typedef std::basic_ostream<char, std::char_traits<char> > ostream_type;

  // this is the function signature of std::endl
  typedef ostream_type& (*ostream_manipulator)(ostream_type&);

  // define an operator<< to take in std::endl
  Stump& operator<<(ostream_manipulator manip)
    {
      // call the function, but we cannot return it's value
      if (manip == (ostream_manipulator) std::endl)
	{
	  (*this)<<Stump::endl;
	}
      else
	currentLog<<manip;

      return *this;
    }

  struct logtype
  {
  public:
  logtype(std::string ltype): ltype(ltype)
    {
    }

    virtual Stump& operator()(Stump& out)
    {
      out.workingMessageType=ltype;
      return out;
    }

  protected:
    std::string ltype;
  };

  friend Stump& operator<<(Stump& out, logtype lt)
  {
    return lt(out);
  }

  void internalMessages(bool enable);
  void setWorkingType(std::string wtype)
  {
    this->workingMessageType=wtype;
  }

  void setStreamBufferMax(long val)
  {
    streamBufferMax = val;
  }

  long getStreamBufferMax()
  {
    return streamBufferMax;
  }

  void setStreamBufferCropMsg(std::string msg)
  {
    streamBufferCropMsg = msg;
  }

  std::string getStreamBufferCropMsg()
    {
      return streamBufferCropMsg;
    }

  int totalMessageTypes()
  {
    return messageTypes.size();
  }

  int enabledOutputs()
  {
    int count=0;
    for (std::map<std::string, std::vector<MessageOutput*> >::iterator i=messageTypes.begin(); i!=messageTypes.end(); ++i)
      {
	for (std::vector<MessageOutput*>::iterator j=i->second.begin(); j!=i->second.end(); ++j)
	  {
	    if ((*j)->enabled)
	      count++;
	  }
      }
    return count;
  }
 protected:
  void initialize();

  struct MessageOutput
  {
    MessageFormat messageFormat;
    std::string typeLabel;
    MessageBuffer *buffer;
    bool enabled;
  MessageOutput(/* OutputType output,  */std::string typeLabel, MessageFormat messageFormat, /* std::string file, TCallbackType1 cbt1, */ MessageBuffer *buffer/* , int bufferSize=20 */):
    /* output(output),  */
      typeLabel(typeLabel), 
      messageFormat(messageFormat),
      buffer(buffer)
    {
      enabled=true;
    }
  };

  MessageBuffer *findMessageBuffer(std::string bufferName);
  MessageBuffer *findMessageBuffer(unsigned bufferId);
  void addOutput(std::string messageType, std::string typeLabel, MessageFormat messageFormat, int bufferSize, MessageBuffer* buffer);
  void internalMessage(std::string msg);
  std::string getFormattedMessage(std::string msg, time_t time, std::string typeLabel, std::string logFormat, std::string dateFormat, std::map<std::string,std::string>extra = std::map<std::string, std::string>());
  std::string getFormattedDate(std::string dateFormat, time_t time);

  std::map<std::string, std::vector<MessageOutput*> > messageTypes;
  std::vector<MessageBuffer*> availableBuffers;
  std::string defaultLogFormat;
  std::string defaultDateFormat;
  std::string defaultRepeatLogFormat;
  int defaultBufferSize;
  bool _internalMessages;
  std::string defaultMessageType; /* Used with log() */
  std::string workingMessageType; /* Used with log << */

  /* Tools */
  static std::string itoa(int val);

  static unsigned bufferId;

 private:
  void logString(std::string msg, MessageOutput &output, time_t time);
  void outputBuffer(MessageOutput &output, bool flush=false);
  void outputBufferOStream(std::ostream &os, const Stump::MessageBuffer *output);
  void outputBufferFile(const Stump::MessageBuffer *output);
  std::string strReplaceMap(std::string source, std::map<std::string,std::string>strMap, int offset=0, int times=0);

  /* mutex and streaming thread */
  __MUTEX_T ostreamMutex;
  __MUTEX_T ostreamDataMutex;
  __THREAD_T streamingThread;
  std::stringstream currentLog;
  long streamBufferMax;
  std::string streamBufferCropMsg;
};

#endif /* _STUMP_H */


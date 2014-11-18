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

  /* #warning "C++11 support not implemented yet" */

  #ifndef __MUTEX_T
    #define __MUTEX_T std::mutex
  #endif

  #ifndef __MUTEX_LOCK
    #define __MUTEX_LOCK(mutex) mutex.lock()
  #endif

  #ifndef __MUTEX_UNLOCK
    #define __MUTEX_UNLOCK(mutex) mutex.unlock()
  #endif

  #ifndef __MUTEX_INIT
    #define __MUTEX_INIT(mutex) 
  #endif

  using std::lock_guard;
#else

  /* If we are not using C++11 we must use pthread for mutexes. */
  #include <pthread.h>

  #ifndef __MUTEX_T
    #define __MUTEX_T pthread_mutex_t
  #endif

  #ifndef __MUTEX_LOCK
    #define __MUTEX_LOCK(mutex) pthread_mutex_lock(&mutex)
  #endif

  #ifndef __MUTEX_UNLOCK
    #define __MUTEX_UNLOCK(mutex) pthread_mutex_unlock(&mutex)
  #endif

  #ifndef __MUTEX_INIT
    #define __MUTEX_INIT(mutex) pthread_mutex_init(&mutex, NULL);
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
      if (currentLog.tellp()==0) /* Start logging */
	{
	  __MUTEX_LOCK(ostreamMutex);
	}

      currentLog << x;

      return *this;
    }

  Stump& operator<<(Stump& (*manipulator)(Stump&))
    {
        // call the function, and return it's value
        return manipulator(*this);
    }

  static Stump& endl(Stump& stump)
  {
    stump.log(stump.currentLog.str(), stump.workingMessageType);
    stump.currentLog.str("");
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
	(*this)<<Stump::endl;
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
    /* OutputType output; */
    /* std::string file;		/\* If apply *\/ */
    /* TCallbackType1 cbt1; */

    MessageFormat messageFormat;
    /* std::string logFormat; */
    /* std::string dateFormat; */
    /* std::string repeatLogFormat; */
    std::string typeLabel;
    MessageBuffer *buffer;
    bool enabled;
  MessageOutput(/* OutputType output,  */std::string typeLabel, MessageFormat messageFormat, /* std::string file, TCallbackType1 cbt1, */ MessageBuffer *buffer/* , int bufferSize=20 */):
    /* output(output),  */
      typeLabel(typeLabel), 
      messageFormat(messageFormat),
      /* logFormat(logFormat),  */
      /* dateFormat(dateFormat),  */
      /* file(file),  */
      /* cbt1(cbt1), */
      /* repeatLogFormat(repeatLogFormat),  */
      buffer(buffer)/* , bufferSize(bufferSize) */
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
  __MUTEX_T ostreamMutex;
  std::stringstream currentLog;
};

/* static inline std::ostream &operator<<( std::ostream &os, const Stump &v ){ */
/*   std::cout << "METO: \""<<os<<"\""<<std::endl; */
/*   return os; */
/* } */


#endif /* _STUMP_H */


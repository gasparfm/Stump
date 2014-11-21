/**
*************************************************************
* @file stump.cpp
* @brief Breve descripción
* Another logging tool for our C++ projects
*
*
* @author Gaspar Fernández <blakeyed@totaki.com>
* @version
* @date 12 abr 2014
* Changelog:
*  - 20140918 : Enable and disable logs
*  - 20141105 : Prevent logging empty strings
*  - 20141118 : finished stream operator to log with <<
*               comments cleanup
*  - 20141120 : fixed bug that mixes streams output when inserting many 
*               streams from several threads at a time.
*  - 20141121 : fixed bug that aborts or segfaults when writing with streams 
*               from several threads. New ostreamDataMutex.
*
* To-do:
*  - Clear one sigle output
*  - Clear_buffer must check if the buffers are not being used
*  - Clear_buffers must free buffers
*  - Clear one single user
*
*************************************************************/

#include "stump.h"
#include <iostream>
#include <pthread.h>
#include <ctime>

// Default settings
#define _LOG_FORMAT "%time% (%type%) : %message%"
#define _DATE_FORMAT "%d/%m/%Y %H:%M:%S"
#define _REPEAT_LOG_FORMAT "%time% - %timelast% (%type%) : %message% x%times%times."
#define _BUFFER_SIZE 20

unsigned Stump::bufferId = 0;

Stump::Stump()
{
  _internalMessages=true;
  workingMessageType="";
  __MUTEX_INIT(ostreamMutex);
  __MUTEX_INIT(ostreamDataMutex);
  streamBufferMax = 1024;
  streamBufferCropMsg = "[CROPPED]";
  // __MUTEX_INIT(writingMutex);
  initialize();
}

Stump::~Stump()
{
  for (std::map<std::string, std::vector<MessageOutput*> >::iterator i=messageTypes.begin(); i!=messageTypes.end(); ++i)
    {
      for (std::vector<MessageOutput*>::iterator j = i->second.begin(); j != i->second.end(); ++j)
	{
	  outputBuffer(**j, true);
	  delete *j;
	}
      // To-do: delete pending structures
    }
  internalMessage("Destroying...");
}

void Stump::internalMessages(bool enable)
{
  this->_internalMessages = enable;
}

void Stump::internalMessage(std::string msg)
{
  if (!_internalMessages)
    return;

  std::cerr << "[STUMP] "<<msg<<std::endl;
}

void Stump::clearBuffers()
{
  availableBuffers.clear();
}

void Stump::disableAll()
{
  for (std::map<std::string, std::vector<MessageOutput*> >::iterator i = messageTypes.begin(); i != messageTypes.end(); ++i)
    {
      for (std::vector<MessageOutput*>::iterator j = i->second.begin(); j != i->second.end(); ++j)
	(*j)->enabled=false;
    }
}

void Stump::enableAll()
{
  for (std::map<std::string, std::vector<MessageOutput*> >::iterator i = messageTypes.begin(); i != messageTypes.end(); ++i)
    {
      for (std::vector<MessageOutput*>::iterator j = i->second.begin(); j != i->second.end(); ++j)
	(*j)->enabled=true;
    }
}

Stump::MessageBuffer *Stump::createBuffer(std::ostream &os, int bufferSize)
{
  MessageBuffer *buffer = new  MessageBuffer(os, bufferSize);
  // Buscar buffers repetidos
  availableBuffers.push_back(buffer);

  return buffer;
}

Stump::MessageBuffer *Stump::createBuffer(std::ostream &os, std::string bufferName, int bufferSize)
{
  MessageBuffer *buffer = new  MessageBuffer(os, bufferName, bufferSize);
  availableBuffers.push_back(buffer);

  return buffer;
}

Stump::MessageBuffer *Stump::createBuffer(std::string file, int bufferSize)
{
  MessageBuffer *buffer = new  MessageBuffer(file, bufferSize);
  availableBuffers.push_back(buffer);

  return buffer;
}

Stump::MessageBuffer *Stump::createBuffer(std::string file, std::string bufferName, int bufferSize)
{
  MessageBuffer *buffer = new  MessageBuffer(file, bufferName, bufferSize);
  availableBuffers.push_back(buffer);

  return buffer;
}


// FALTAN FUNCIONES PARA MODIFICAR PARAMETROS DE UNA SALIDA
// COLORES PARA CONSOLA
// ESCRIBIR EN ARCHIVO
void Stump::addOutput(std::string messageType, std::string typeLabel, MessageFormat messageFormat, int bufferSize)
{
  addOutput(messageType, typeLabel, messageFormat, bufferSize);
}

void Stump::addOutput(std::string messageType, std::string typeLabel, MessageFormat messageFormat, std::string bufferName)
{
  MessageBuffer *buffer = findMessageBuffer(bufferName);
  if (buffer==NULL)
    this->internalMessage("Could not find buffer "+bufferName);
  else
    addOutput(messageType, typeLabel, messageFormat, 0, buffer);
}

void Stump::clearOutputs()
{
  messageTypes.clear();
}

void Stump::addOutput(std::string messageType, std::string typeLabel, MessageFormat messageFormat, int bufferSize, MessageBuffer* buffer)
{
  if (buffer==NULL)
    {
      internalMessage("buffer can't be null");
      return;
    }
  
  messageTypes[messageType].push_back(new MessageOutput(// output, 
							typeLabel, 
							messageFormat,
							// file, 
							// cbt1, 
							buffer)
				      );
  if (defaultMessageType=="")
    defaultMessageType=messageType;
}

void Stump::initialize()
{
  this->defaultLogFormat = _LOG_FORMAT;
  this->defaultDateFormat = _DATE_FORMAT;
  this->defaultRepeatLogFormat = _REPEAT_LOG_FORMAT;
  this->defaultBufferSize = _BUFFER_SIZE;

  MessageBuffer *buf = createBuffer(std::cout, "logBuffer", 2);
  MessageBuffer *buf2 = createBuffer("mylog.txt", "fichero", 2);

  addOutput("info", "Information", 
	    MessageFormat(this),
	    "logBuffer"
	    );
  addOutput("info", "Info", 
	    MessageFormat(this),
	    "fichero"
	    );
  addOutput("warning", "Warning", 
	    MessageFormat(this),
	    "logBuffer"
	    );
  addOutput("error", "Error", 
	    MessageFormat(this),
	    "logBuffer"
	    );
  addOutput("fatal", "Fatal Error", 
	    MessageFormat(this),
	    "logBuffer"
	    );
  addOutput("other", "Other Error", 
	    MessageFormat(this),
	    "logBuffer"
	    );
}

void Stump::log(std :: string msg, std::string logType)
{
  if (!msg.length())
    return;

  if (logType=="")
    logType=this->defaultMessageType;

  if (messageTypes.size() == 0)
	return;

  std::map<std::string, std::vector<MessageOutput*> >::iterator mtype = messageTypes.find(logType);
  if (mtype==messageTypes.end())
    {
      this->internalMessage("No message types found");
      return;
    }

  time_t time = std::time(NULL);
  for (std::vector<MessageOutput*>::iterator i = mtype->second.begin(); i != mtype->second.end(); ++i)
    {
      this->logString(msg, **i, time);
    }
}

std::string Stump::getFormattedDate(std::string dateFormat, time_t time)
{
  char timeBuffer[300];
  if (std::strftime(timeBuffer, sizeof(timeBuffer), dateFormat.c_str(), std::localtime(&time)))
    return std::string (timeBuffer);

  internalMessage("date couldn't be written");
  return "DATE-ERROR";
}

std::string Stump::getFormattedMessage(std::string msg, time_t time, std::string typeLabel, std::string logFormat, std::string dateFormat, std::map<std::string,std::string> extra)
{
  std::map<std::string,std::string>strMap = extra;
  strMap["%time%"] = getFormattedDate(dateFormat, time);
  strMap["%type%"] = typeLabel;
  strMap["%message%"] = msg;

  return strReplaceMap(logFormat, strMap);
}

void Stump::logString(std :: string msg, MessageOutput &output, time_t time)
{
  if (!output.enabled)
    return;

  __MUTEX_LOCK(output.buffer->mutex);

  if ( (output.messageFormat.repeatLogFormat != "") && (output.buffer->lastMessage==msg) )
    {
      std::map<std::string,std::string> repeatData;
      repeatData["%times%"] = itoa(++output.buffer->lastRepeat);
      repeatData["%timelast%"] = getFormattedDate(output.messageFormat.dateFormat, output.buffer->lastDate);
      output.buffer->buffer.pop_back();
      // Let's do something with repeating messages!
      output.buffer->buffer.push_back(getFormattedMessage(msg, 
							  time,
							  output.typeLabel,
							  output.messageFormat.repeatLogFormat,
							  output.messageFormat.dateFormat,
							  repeatData));
    }
  else
    {
      // if (output.buffer.size() < output.bufferSize)
      output.buffer->lastDate = time;
      output.buffer->lastRepeat = 1;
      output.buffer->buffer.push_back(getFormattedMessage(msg, 
							  time,
							  output.typeLabel,
							  output.messageFormat.logFormat,
							  output.messageFormat.dateFormat));
      output.buffer->lastMessage=msg;
      outputBuffer(output);
    }
  __MUTEX_UNLOCK(output.buffer->mutex);

}

std::string Stump::strReplaceMap(std::string source, std::map<std::string,std::string>strMap, int offset, int times)
{
  int total = 0;
  std::string::size_type pos=offset;
  std::string::size_type newPos;
  std::string::size_type lowerPos;

  do
    {
      std::string rep;
      for (std::map<std::string, std::string>::iterator i=strMap.begin(); i!=strMap.end(); ++i)
	{
	  std::string fromStr = i->first;

	  newPos = source.find(fromStr, pos);
	  if ( (i==strMap.begin()) || (newPos<lowerPos) )
	    {
	      rep = fromStr;
	      lowerPos = newPos;
	    }
	}

      pos = lowerPos;
      if (pos == std::string::npos)
	break;

      std::string toStr = strMap[rep];

      source.replace(pos, rep.length(), toStr);
      pos+=toStr.size();

    } while ( (times==0) || (++total<times) );

  return source;
}

void Stump::outputBuffer(MessageOutput &output, bool flush)
{
  if ( (output.buffer->buffer.size()>=output.buffer->bufferSize) || (flush) )
    {
      switch (output.buffer->output)
	{
	case OSTREAM:
	  outputBufferOStream(output.buffer->ost, output.buffer);
	  break;
	case FILE:
	  outputBufferFile(output.buffer);
	  break;
	}

      // buffers debug
      // for (unsigned i=0; i<output.buffer->buffer.size(); ++i)
      // 	{
      // 	  std::cout << output.buffer->buffer[i] << std::endl;
      // 	}
      output.buffer->buffer.clear();
      output.buffer->lastMessage="";	// Avoid log repeating after flush
    }
}

void Stump::outputBufferOStream(std::ostream &os, const Stump::MessageBuffer *output)
{
  for (unsigned i=0; i<output->buffer.size(); ++i)
    {
      os << output->buffer[i] << std::endl;
    }

}

void Stump::outputBufferFile(const Stump::MessageBuffer *output)
{
  std::fstream f;
  f.open(output->file.c_str(), std::fstream::out | std::fstream::app);
  if (!f.is_open())
    {
      internalMessage("Cannot open file "+output->file);
      return;
    }
  outputBufferOStream(f, output);
  f.close();
}

Stump::MessageBuffer* Stump::findMessageBuffer(std::string bufferName)
{
  for (std::vector<MessageBuffer*>::iterator i=availableBuffers.begin(); i!=availableBuffers.end(); ++i)
    {
      if ((*i)->bufferName == bufferName)
	return (*i);
    }
  return NULL;
}

Stump::MessageBuffer* Stump::findMessageBuffer(unsigned bufferId)
{
  for (std::vector<MessageBuffer*>::iterator i=availableBuffers.begin(); i!=availableBuffers.end(); ++i)
    {
      if ((*i)->bufferId == bufferId)
	return (*i);
    }
  return NULL;
}

std::string Stump::itoa(int val)
{
  std::string out;

  for (; val ; val /= 10)
    out = "0123456789"[val % 10] + out;

  return out;
}

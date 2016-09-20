/**
*************************************************************
* @file sample01.cpp
* @brief Brief description
* Pequeña documentación del archivo
*
*
*
*
*
* @author Gaspar Fernández <blakeyed@totaki.com>
* @version
* @date 12 abr 2014
* Changelog:
*
*
*
*
*
*
*
*************************************************************/

#include <iostream>
#include "stump.h"
#include <cstdlib>
#include <pthread.h>
#include <unistd.h>

Stump *log = new Stump();

void *task(void*)
{
  for (unsigned i=0; i<1000; ++i)
    {
      (*log) << "Logs with streams" << std::endl;
    }

  pthread_exit(NULL);
}

void *task2(void*)
{
  for (unsigned i=0; i<1000; ++i)
    {
      log->log("Logging fatal errors", "fatal");
    }

  pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
  pthread_t thread;
  int rc = pthread_create(&thread, NULL, task, NULL);
  pthread_t thread2;
  rc = pthread_create(&thread2, NULL, task2, NULL);

  log->log("Hello, I'm logging");
  log->log("Hello, I'm logging");
  log->log("Hello, I'm logging");
  log->log("Hello, I'm logging");
  log->log("This is a fatal error log", "fatal");
  log->log("Hello, I'm logging");
  log->log("Hello, I'm logging");
  log->log("Hello, I'm logging");
  log->log("Hello, I'm logging");
  log->log("Hello, I'm logging");
  log->log("Hello, I'm logging");
  log->log("Hello, I'm logging");
  log->log("Hello, I'm logging");
  log->log("Hello, I'm logging");
  log->log("Hello, I'm logging");
  log->log("Hello, I'm logging");
  log->log("Hello, I'm logging");
  log->log("This is the last log with log()");
  (*log) << "Ok, log with" << "streams from main()" << 123 << Stump::endl;
  (*log) << "Last log inside main()" << 456 << std::endl;

  usleep(1000000);
  delete log;
  pthread_exit(NULL);

  return EXIT_SUCCESS;
}


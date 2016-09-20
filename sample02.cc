/**
*************************************************************
* @file sample02.cpp
* @brief More testing
* Testing with lots of threads 
*
* @author Gaspar Fernández <blakeyed@totaki.com>
* @version
* @date 12 jul 2014
* Changelog:
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

int threads = 0;
pthread_mutex_t threadsmutex;

void *task(void*)
{
  pthread_mutex_lock(&threadsmutex);
  threads++;
  pthread_mutex_unlock(&threadsmutex);

  for (unsigned i=0; i<100000; ++i)
    {
      (*log) << "Logs with streams " << i << "has thread id: "<<pthread_self() << std::endl;
    }

  pthread_mutex_lock(&threadsmutex);
  threads--;
  pthread_mutex_unlock(&threadsmutex);

  pthread_exit(NULL);
}

void *task2(void*)
{
  pthread_mutex_lock(&threadsmutex);
  threads++;
  pthread_mutex_unlock(&threadsmutex);

  for (unsigned i=0; i<100000; ++i)
    {
      log->log("Logging fatal errors", "fatal");
    }

  pthread_mutex_lock(&threadsmutex);
  threads--;
  pthread_mutex_unlock(&threadsmutex);

  pthread_exit(NULL);
}


int main(int argc, char *argv[])
{
  //  log->disableAll();
  int rc;
  pthread_t thread;
  (*log) << std::endl;
  rc  = pthread_create(&thread, NULL, task, NULL);
  pthread_t thread2;
  rc = pthread_create(&thread2, NULL, task2, NULL);
  pthread_t thread20;
  rc = pthread_create(&thread20, NULL, task2, NULL);
  pthread_t thread21;
  rc = pthread_create(&thread21, NULL, task2, NULL);
  pthread_t thread22;
  rc = pthread_create(&thread22, NULL, task2, NULL);
  pthread_t thread23;
  rc = pthread_create(&thread23, NULL, task2, NULL);
  pthread_t thread3;
  rc = pthread_create(&thread3, NULL, task, NULL);
  pthread_t thread4;
  rc = pthread_create(&thread4, NULL, task, NULL);
  pthread_t thread5;
  rc = pthread_create(&thread5, NULL, task, NULL);
  (*log) << std::endl;
  (*log) << std::endl;
  (*log) << std::endl;
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

  while (threads>0)
    {
      std::cout << "Waiting... " <<threads<< std::endl;
      usleep(1000000);
    }

  delete log;
  pthread_exit(NULL);

  return EXIT_SUCCESS;
}


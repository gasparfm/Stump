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

int main(int argc, char *argv[])
{
  (*log) << "Ok, log with" << "streams from main()" << 123 << Stump::endl;
  (*log) << "Last log inside main()" << 456 << std::endl;
  (*log) << std::endl;

  (*log) << "OH LA LA" << Stump::logtype("other") << std::endl;
  delete log;

  return EXIT_SUCCESS;
}


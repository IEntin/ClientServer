/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Ad.h"
#include "Chronometer.h"
#include "FifoServer.h"
#include "MemoryPool.h"
#include "ServerOptions.h"
#include "TaskController.h"
#include "TcpServer.h"
#include <csignal>
#include <iostream>

void signalHandler(int signal) {}

int main() {
  signal(SIGPIPE, SIG_IGN);
  std::signal(SIGINT, signalHandler);
  sigset_t set;
  sigemptyset(&set);
  if (sigaddset(&set, SIGINT) == -1)
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ' ' << strerror(errno) << std::endl;
  ServerOptions options;
  MemoryPool::setup(options._bufferSize);
  // optionally record elapsed times
  Chronometer chronometer(options._timingEnabled, __FILE__, __LINE__);
  if (!Ad::load(options._adsFileName))
    return 1;
  TaskControllerPtr taskController = TaskController::instance(options._numberWorkThreads,
							      options._processRequest);
  tcp::TcpServerPtr tcpServer =
    std::make_shared<tcp::TcpServer>(taskController,
				     options._expectedTcpConnections,
				     options._tcpPort,
				     options._tcpTimeout,
				     options._compressor);
  if (!tcpServer->start())
    return 2;
  fifo::FifoServerPtr fifoServer =
    std::make_shared<fifo::FifoServer>(taskController,
				       options._fifoDirectoryName,
				       options._fifoBaseNames,
				       options._compressor);
  if (!fifoServer->start())
    return 3;
  int sig = 0;
  if (sigwait(&set, &sig) != SIGINT)
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ' ' << strerror(errno) << std::endl;
  fifoServer->stop();
  tcpServer->stop();
  int ret = fcloseall();
  assert(ret == 0);
  return 0;
}

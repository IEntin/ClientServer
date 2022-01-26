/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Chronometer.h"
#include "Echo.h"
#include "FifoServer.h"
#include "ProgramOptions.h"
#include "TaskThread.h"
#include "TcpServer.h"
#include "Transaction.h"
#include <csignal>
#include <iostream>

volatile std::atomic<bool> stopFlag = false;

void signalHandler(int signal) {
}

int main() {
  signal(SIGPIPE, SIG_IGN);
  std::signal(SIGINT, signalHandler);
  sigset_t set;
  sigemptyset(&set);
  if (sigaddset(&set, SIGINT) == -1)
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ' ' << strerror(errno) << std::endl;
  const bool timing = ProgramOptions::get("Timing", false);
  Chronometer chronometer(timing, __FILE__, __LINE__);
  ProcessRequest processRequest;
  std::string method = ProgramOptions::get("ProcessRequestMethod", std::string());
  if (method == "Transaction")
    processRequest = Transaction::processRequest;
  else if (method == "Echo")
    processRequest = echo::processRequest;
  else {
    std::cerr << "No valid processRequest definition provided" << std::endl;
    return 1;
  }
  if (!fifo::FifoServer::startThreads())
    return 1;
  tcp::TcpServer::startServer();
  if (!TaskThread::startThreads(processRequest))
    return 1;
  int sig = 0;
  if (sigwait(&set, &sig) != SIGINT)
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ' ' << strerror(errno) << std::endl;
  stopFlag.store(true);
  fifo::FifoServer::joinThreads();
  tcp::TcpServer::stopServer();
  TaskThread::joinThreads();
  int ret = fcloseall();
  assert(ret == 0);
  return 0;
}

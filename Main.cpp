/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Ad.h"
#include "Chronometer.h"
#include "Echo.h"
#include "FifoServer.h"
#include "ProgramOptions.h"
#include "TaskThread.h"
#include "Transaction.h"
#include <csignal>
#include <iostream>

const bool timing = ProgramOptions::get("Timing", false);
Chronometer chronometer(timing, __FILE__, __LINE__);

void signalHandler(int signal) {
  fifo::FifoServer::stop();
}

int main() {
  signal(SIGPIPE, SIG_IGN);
  std::signal(SIGINT, signalHandler);
  ProcessRequest processRequest;
  std::string method = ProgramOptions::get("ProcessRequestMethod", std::string());
  if (method == "Transaction") {
    if (!Ad::load())
      return 1;
    processRequest = Transaction::processRequest;
  }
  else if (method == "Echo")
    processRequest = echo::processRequest;
  else {
    std::cerr << "No valid processRequest definition provided" << std::endl;
    return 1;
  }
  if (!fifo::FifoServer::startThreads())
    return 1;
  if (!TaskThread::startThreads(processRequest))
    return 1;
  fifo::FifoServer::joinThreads();
  TaskThread::joinThreads();
  int ret = fcloseall();
  assert(ret == 0);
  return 0;
}

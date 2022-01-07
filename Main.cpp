#include "Chronometer.h"
#include "Echo.h"
#include "FifoRunnable.h"
#include "ProgramOptions.h"
#include "TaskRunnable.h"
/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Ad.h"
#include "Transaction.h"
#include <csignal>
#include <iostream>

const bool timing = ProgramOptions::get("Timing", false);
Chronometer chronometer(timing, __FILE__, __LINE__);

void signalHandler(int signal) {
  fifo::FifoRunnable::stop();
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
  if (!fifo::FifoRunnable::startThreads())
    return 1;
  if (!TaskRunnable::startThreads(processRequest))
    return 1;
  fifo::FifoRunnable::joinThreads();
  TaskRunnable::joinThreads();
  Ad::destroy();
  int ret = fcloseall();
  assert(ret == 0);
  return 0;
}

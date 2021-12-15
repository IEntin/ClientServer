#include "Chronometer.h"
#include "Echo.h"
#include "FifoRunnable.h"
#include "Log.h"
#include "ProgramOptions.h"
#include "TaskRunnable.h"
#include "Test.h"
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
  if (method == "Log") {
    if (!Log::init())
	return 1;
    processRequest = Log::processRequest;
  }
  else if (method == "Test")
    processRequest = test::processRequest;
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
  int ret = fcloseall();
  assert(ret == 0);
  return 0;
}

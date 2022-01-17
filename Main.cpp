/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Ad.h"
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

namespace {

std::promise<void> stopPromise;

} // end of anonimous namespace

void stop() {
  try {
    stopPromise.set_value();
  }
  catch (std::future_error& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << "-exception:" << e.what() << std::endl;
  }
}

void signalHandler(int signal) {
  stopFlag.store(true);
  stop();
}

int main() {
  signal(SIGPIPE, SIG_IGN);
  std::signal(SIGINT, signalHandler);
  const bool timing = ProgramOptions::get("Timing", false);
  Chronometer chronometer(timing, __FILE__, __LINE__);
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
  tcp::TcpServer::startServers();
  if (!TaskThread::startThreads(processRequest))
    return 1;
  auto future = stopPromise.get_future();
  future.get();
  fifo::FifoServer::joinThreads();
  tcp::TcpServer::joinThread();
  TaskThread::joinThreads();
  int ret = fcloseall();
  assert(ret == 0);
  return 0;
}

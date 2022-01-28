/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Chronometer.h"
#include "Compression.h"
#include "Echo.h"
#include "FifoServer.h"
#include "MemoryPool.h"
#include "ProgramOptions.h"
#include "TaskThread.h"
#include "TcpServer.h"
#include "Transaction.h"
#include <csignal>
#include <iostream>

volatile std::atomic<bool> stopFlag = false;

unsigned getNumberTaskThreads() {
  unsigned numberTaskThreadsCfg = ProgramOptions::get("NumberTaskThreads", 0);
  return numberTaskThreadsCfg > 0 ? numberTaskThreadsCfg : std::thread::hardware_concurrency();
}

size_t getMemPoolBufferSize() {
  return ProgramOptions::get("DYNAMIC_BUFFER_SIZE", 100000);
}

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
  std::string compressorStr = ProgramOptions::get("Compression", std::string());
  Compression::setCompressionEnabled(compressorStr);
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
  if (!fifo::FifoServer::startThreads(ProgramOptions::get("FifoDirectoryName", std::string()),
				      ProgramOptions::get("FifoBaseNames", std::string())))
    return 1;
  tcp::TcpServer::startServer(ProgramOptions::get("TcpPort", 0), ProgramOptions::get("Timeout", 1));
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

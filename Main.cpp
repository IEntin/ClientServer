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

void signalHandler(int signal) {}

int main() {
  signal(SIGPIPE, SIG_IGN);
  std::signal(SIGINT, signalHandler);
  sigset_t set;
  sigemptyset(&set);
  if (sigaddset(&set, SIGINT) == -1)
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ' ' << strerror(errno) << std::endl;
  MemoryPool::setup(ProgramOptions::get("DYNAMIC_BUFFER_SIZE", 100000));
  Compression::setCompressionEnabled(ProgramOptions::get("Compression", std::string()));
  // optionally record elapsed times
  Chronometer chronometer(ProgramOptions::get("Timing", false), __FILE__, __LINE__);
  // method to apply to every request in the batch
  ProcessRequest processRequest;
  std::string method = ProgramOptions::get("ProcessRequestMethod", std::string());
  if (method == "Transaction") {
    Ad::load(ProgramOptions::get("AdsFileName", std::string()));
    processRequest = Transaction::processRequest;
  }
  else if (method == "Echo")
    processRequest = echo::processRequest;
  else {
    std::cerr << "No valid processRequest definition provided" << std::endl;
    return 1;
  }
  unsigned numberWorkThreadsCfg = ProgramOptions::get("NumberTaskThreads", 0);
  unsigned numberWorkThreads = numberWorkThreadsCfg > 0 ? numberWorkThreadsCfg :
    std::thread::hardware_concurrency();
  auto taskThreadPool = std::make_shared<TaskThreadPool>(numberWorkThreads, processRequest);
  taskThreadPool->start();
  if (!fifo::FifoServer::startThreads(ProgramOptions::get("FifoDirectoryName", std::string()),
				      ProgramOptions::get("FifoBaseNames", std::string())))
    return 1;
  tcp::TcpServer tcpServer(ProgramOptions::get("ExpectedTcpConnections", 1),
			   ProgramOptions::get("TcpPort", 0),
			   ProgramOptions::get("Timeout", 1));
  int sig = 0;
  if (sigwait(&set, &sig) != SIGINT)
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ' ' << strerror(errno) << std::endl;
  fifo::FifoServer::joinThreads();
  tcpServer.stop();
  taskThreadPool->stop();
  int ret = fcloseall();
  assert(ret == 0);
  return 0;
}

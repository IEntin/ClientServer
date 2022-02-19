/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Chronometer.h"
#include "Compression.h"
#include "FifoServer.h"
#include "MemoryPool.h"
#include "ProgramOptions.h"
#include "TaskController.h"
#include "TcpServer.h"
#include "Transaction.h"
#include <csignal>
#include <filesystem>
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
  // optionally record elapsed times
  Chronometer chronometer(ProgramOptions::get("Timing", false), __FILE__, __LINE__);
  // method to apply to every request in the batch
  ProcessRequest processRequest = Transaction::processRequest;
  if (!Ad::load(ProgramOptions::get("AdsFileName", std::string())))
    return 1;
  unsigned numberWorkThreadsCfg = ProgramOptions::get("NumberTaskThreads", 0);
  unsigned numberWorkThreads = numberWorkThreadsCfg > 0 ? numberWorkThreadsCfg :
    std::thread::hardware_concurrency();
  TaskControllerPtr taskController =
    std::make_shared<TaskController>(numberWorkThreads, processRequest);
  taskController->start();
  auto compression = Compression::isCompressionEnabled(ProgramOptions::get("Compression", std::string(LZ4)));
  if (!tcp::TcpServer::start(ProgramOptions::get("ExpectedTcpConnections", 1),
			     ProgramOptions::get("TcpPort", 49172),
			     ProgramOptions::get("Timeout", 1),
			     compression)) {
    taskController->stop();
    return 2;
  }
  if (!fifo::FifoServer::start(ProgramOptions::get("FifoDirectoryName", std::filesystem::current_path().string()),
			       ProgramOptions::get("FifoBaseNames", std::string("client1")),
			       compression)) {
    tcp::TcpServer::stop();
    taskController->stop();
    return 3;
  }
  int sig = 0;
  if (sigwait(&set, &sig) != SIGINT)
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ' ' << strerror(errno) << std::endl;
  fifo::FifoServer::stop();
  tcp::TcpServer::stop();
  taskController->stop();
  int ret = fcloseall();
  assert(ret == 0);
  return 0;
}

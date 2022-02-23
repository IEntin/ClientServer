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
  TaskControllerPtr taskController = TaskController::instance(numberWorkThreads, processRequest);
  COMPRESSORS compressor = Compression::isCompressionEnabled(ProgramOptions::get("Compression", std::string(LZ4)));
  tcp::TcpServerPtr tcpServer =
    std::make_shared<tcp::TcpServer>(taskController,
				     ProgramOptions::get("ExpectedTcpConnections", 1),
				     ProgramOptions::get("TcpPort", 49172),
				     ProgramOptions::get("Timeout", 1),
				     compressor);
  if (!tcpServer->start())
    return 2;
  fifo::FifoServerPtr fifoServer =
    std::make_shared<fifo::FifoServer>(taskController,
				       ProgramOptions::get("FifoDirectoryName", std::filesystem::current_path().string()),
				       ProgramOptions::get("FifoBaseNames", std::string("client1")),
				       compressor);
  if (!fifoServer->start())
    return 3;
  int sig = 0;
  if (sigwait(&set, &sig) != SIGINT)
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ' ' << strerror(errno) << std::endl;
  fifoServer->stop();
  tcpServer->stop();
  taskController->stop();
  int ret = fcloseall();
  assert(ret == 0);
  return 0;
}

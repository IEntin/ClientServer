/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Ad.h"
#include "Chronometer.h"
#include "FifoServer.h"
#include "ServerOptions.h"
#include "Task.h"
#include "TaskController.h"
#include "TcpServer.h"
#include "Transaction.h"
#include <csignal>

void signalHandler([[maybe_unused]] int signal) {}

int main() {
  signal(SIGPIPE, SIG_IGN);
  std::signal(SIGINT, signalHandler);
  sigset_t set;
  sigemptyset(&set);
  if (sigaddset(&set, SIGINT) == -1)
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ' ' << strerror(errno) << std::endl;
  ServerOptions options("ServerOptions.json");
  // optionally record elapsed times
  Chronometer chronometer(options._timingEnabled, __FILE__, __LINE__);
  if (options._processRequest == Transaction::processRequest) {
    if (!Ad::load(options._adsFileName))
      return 1;
    Task::setPreprocessMethod(Transaction::normalizeSizeKey);
  }
  Task::setProcessMethod(options._processRequest);
  TaskControllerPtr taskController = TaskController::instance(&options);
  tcp::TcpServerPtr tcpServer = std::make_shared<tcp::TcpServer>(options, taskController);
  if (!tcpServer->start())
    return 2;
  fifo::FifoServerPtr fifoServer = std::make_shared<fifo::FifoServer>(options, taskController);
  if (!fifoServer->start(options))
    return 3;
  int sig = 0;
  if (sigwait(&set, &sig))
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ' ' << strerror(errno) << std::endl;
  tcpServer->stop();
  fifoServer->stop();
  int ret = fcloseall();
  assert(ret == 0);
  return 0;
}

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Chronometer.h"
#include "ClientOptions.h"
#include "Logger.h"
#include "Metrics.h"
#include "FifoClient.h"
#include "TcpClient.h"
#include <csignal>

void signalHandler(int) {
  Client::setStopFlag();
}

int main() {
  struct DoAtEnd {
    DoAtEnd() = default;
    ~DoAtEnd() {
      Metrics::print();
    }
  } doAtEnd;
  ClientOptions options("ClientOptions.json");
  if (options._fifoClient)
    std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);
  sigset_t set;
  sigemptyset(&set);
  if (sigaddset(&set, SIGINT) == -1)
    LogError << strerror(errno) << std::endl;
  if (sigaddset(&set, SIGTERM) == -1)
    LogError << strerror(errno) << std::endl;
  Chronometer chronometer(options._timing, __FILE__, __LINE__, __func__);
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);
  std::cout.tie(nullptr);
  signal(SIGPIPE, SIG_IGN);
  try {
    if (options._fifoClient) {
      fifo::FifoClient client(options);
      if (!client.run())
	return 1;
    }
    if (options._tcpClient) {
      tcp::TcpClient client(options);
      if (!client.run())
	return 2;
    }
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
    return 3;
  }
  catch (...) {
    LogError << std::strerror(errno) << std::endl;
    return 4;
  }
  return 0;
}

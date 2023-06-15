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

namespace {
  ClientOptions options("ClientOptions.json");
} // end of anonimous namespace

void signalHandler(int) {
  if (options._fifoClient)
    fifo::FifoClient::onClose();
  else if (options._tcpClient) {
    tcp::TcpClient::onClose();
  }
}

int main() {
  struct DoAtEnd {
    DoAtEnd() = default;
    ~DoAtEnd() {
      Metrics::print();
    }
  } doAtEnd;
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);
  signal(SIGPIPE, SIG_IGN);
  Chronometer chronometer(options._timing, __FILE__, __LINE__, __func__);
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);
  std::cout.tie(nullptr);
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

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Chronometer.h"
#include "ClientOptions.h"
#include "Metrics.h"
#include "FifoClient.h"
#include "TcpClient.h"
#include <csignal>

void signalHandler(int) {
  Client::onSignal();
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
  ClientOptions options("ClientOptions.json");
  Chronometer chronometer(ClientOptions::_timing, __FILE__, __LINE__, __func__);
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);
  std::cout.tie(nullptr);
  try {
    if (ClientOptions::_fifoClient) {
      fifo::FifoClient client(options);
      if (!client.run())
	return 1;
    }
    if (ClientOptions::_tcpClient) {
      tcp::TcpClient client(options);
      if (!client.run())
	return 2;
    }
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
    return 3;
  }
  catch (...) {
    LogError << std::strerror(errno) << '\n';
    return 4;
  }
  return 0;
}

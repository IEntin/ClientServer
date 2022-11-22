/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Chronometer.h"
#include "ClientOptions.h"
#include "Metrics.h"
#include "FifoClient.h"
#include "TcpClient.h"
#include "Utility.h"
#include <csignal>

void signal_handler(int) {
  Client::setStopFlag();
}

int main() {
  struct DoAtEnd {
    DoAtEnd() = default;
    ~DoAtEnd() {
      Metrics metrics;
      metrics.print();
    }
  } doAtEnd;
  ClientOptions options("ClientOptions.json");
  std::signal(SIGINT, signal_handler);
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
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-' << e.what() << std::endl;
    return 3;
  }
  catch (...) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-' << std::strerror(errno) << std::endl;
    return 4;
  }
  return 0;
}

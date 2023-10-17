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
  ClientOptions::parse("ClientOptions.json");
  Chronometer chronometer(ClientOptions::_timing, __FILE__, __LINE__, __func__);
  try {
    if (ClientOptions::_fifoClient) {
      fifo::FifoClient client;
      if (!client.run())
	return 1;
    }
    if (ClientOptions::_tcpClient) {
      tcp::TcpClient client;
      if (!client.run())
	return 2;
    }
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
    return 3;
  }
  return 0;
}

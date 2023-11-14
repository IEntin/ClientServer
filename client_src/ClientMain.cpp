/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ClientOptions.h"
#include "Metrics.h"
#include "FifoClient.h"
#include "TcpClient.h"
#include <csignal>

namespace {
  std::atomic<Client*> clientPtr = 0;
}// end of anonimous namespace

void signalHandler(int) {
  Client::onSignal(clientPtr);
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
  ClientOptions clientOptions;
  clientOptions.parse("ClientOptions.json");
  try {
    if (clientOptions._fifoClient) {
      fifo::FifoClient client;
      clientPtr.store(&client);
      if (!client.run())
	return 1;
    }
    if (clientOptions._tcpClient) {
      tcp::TcpClient client;
      clientPtr.store(&client);
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

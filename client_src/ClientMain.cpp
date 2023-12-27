/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <csignal>

#include <filesystem>

#include "ClientOptions.h"
#include "Metrics.h"
#include "FifoClient.h"
#include "TcpClient.h"

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
  std::string fileName("ClientOptions.json");
  // The folowing covers setup with the client starting from
  // the project root rather than prepared client directory.
  if (!std::filesystem::exists(fileName)) {
    LogError << fileName << " not found,\n trying client_src/ClientOptions.json.\n";
    fileName = "client_src/ClientOptions.json";
  }
  ClientOptions::parse(fileName);
  try {
    if (ClientOptions::_fifoClient) {
      fifo::FifoClient client;
      clientPtr.store(&client);
      if (!client.run())
	return 1;
    }
    if (ClientOptions::_tcpClient) {
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

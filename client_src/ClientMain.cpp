/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <csignal>

#include <filesystem>

#include "Client.h"
#include "ClientOptions.h"
#include "FifoClient.h"
#include "Metrics.h"
#include "TcpClient.h"

namespace {
  std::shared_ptr<ClientWrapper> wrapper;
}// end of anonimous namespace

void signalHandler(int) {
  Client::onSignal(wrapper);
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
  std::string_view fileName("ClientOptions.json");
  // The folowing covers setup with the client starting from
  // the project root rather than prepared client directory.
  if (!std::filesystem::exists(fileName)) {
    Expected << fileName << " not found,\n trying client_src/ClientOptions.json.\n";
    fileName = "client_src/ClientOptions.json";
  }
  ClientOptions::parse(fileName);
  try {
    if (ClientOptions::_fifoClient) {
      fifo::FifoClient client;
      wrapper = std::make_shared<ClientWrapper>(client);
      client.run();
    }
    if (ClientOptions::_tcpClient) {
      tcp::TcpClient client;
      wrapper = std::make_shared<ClientWrapper>(client);
      client.run();
    }
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
    return 3;
  }
  return 0;
}

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <csignal>

#include <boost/stacktrace.hpp>

#include "Client.h"
#include "ClientOptions.h"
#include "DebugLog.h"
#include "FifoClient.h"
#include "Metrics.h"
#include "TcpClient.h"
#include "Utility.h"

void signalHandler(int) {
  Client::onSignal();
}

int main() {
  DebugLog::setDebugLog(APPTYPE::CLIENT);
  std::string terminal(getenv("GNOME_TERMINAL_SCREEN"));
  utility::setClientTerminal(terminal);
  bool initialized[[maybe_unused]] = cryptodefinitions::initialize();
  struct Finally {
    Finally() = default;
    ~Finally() {
      Metrics::print();
    }
  } finally;
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);
  signal(SIGPIPE, SIG_IGN);
  std::string_view fileName("ClientOptions.json");
  ClientOptions::parse(fileName);
  try {
    if (ClientOptions::_fifoClient) {
      fifo::FifoClient client;
      client.run();
    }
    else if (ClientOptions::_tcpClient) {
      tcp::TcpClient client;
      client.run();
    }
  }
  catch (const std::exception& e) {
    LogError << boost::stacktrace::stacktrace() << '\n';
    LogError << e.what() << '\n';
    return 3;
  }
  return 0;
}

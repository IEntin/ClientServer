/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <cassert>
#include <csignal>
#include <cstring>
#include <filesystem>

#include "Chronometer.h"
#include "Metrics.h"
#include "ServerOptions.h"
#include "Server.h"
#include "TransactionStrategy.h"

void signalHandler([[maybe_unused]] int signal) {}

int main() {
  struct DoAtEnd {
    DoAtEnd() = default;
    ~DoAtEnd() {
      Metrics::print();
    }
  } doAtEnd;
  try {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    sigset_t set;
    sigemptyset(&set);
    if (sigaddset(&set, SIGINT) == -1)
      LogError << strerror(errno) << '\n';
    if (sigaddset(&set, SIGTERM) == -1)
      LogError << strerror(errno) << '\n';
    ServerOptions serverOptions;
    serverOptions.parse("ServerOptions.json");
    Server server(std::make_unique<TransactionStrategy>());
    if (!server.start())
      return 3;
    int sig = 0;
    if (sigwait(&set, &sig))
      LogError << strerror(errno) << '\n';
    Metrics::save();
    server.stop();
    int closed = fcloseall();
    assert(closed == 0);
    return 0;
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
    return 5;
  }
}

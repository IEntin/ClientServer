/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Chronometer.h"
#include "Globals.h"
#include "Logger.h"
#include "Metrics.h"
#include "ServerOptions.h"
#include "Server.h"
#include <cassert>
#include <csignal>
#include <cstring>
#include <filesystem>

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
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);
    sigset_t set;
    sigemptyset(&set);
    if (sigaddset(&set, SIGINT) == -1)
      LogError << strerror(errno) << std::endl;
    if (sigaddset(&set, SIGTERM) == -1)
      LogError << strerror(errno) << std::endl;
    ServerOptions options("ServerOptions.json");
    // optionally record elapsed times
    Chronometer chronometer(options._timing, __FILE__, __LINE__);
    Server server(options);
    if (!server.start())
      return 3;
    int sig = 0;
    if (sigwait(&set, &sig))
      LogError << strerror(errno) << std::endl;
    std::filesystem::remove(options._controlFileName);
    Metrics::save();
    server.stop();
    int closed = fcloseall();
    assert(closed == 0);
    return 0;
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
    return 5;
  }
  catch (...) {
    LogError << strerror(errno) << std::endl;
    return 6;
  }
}

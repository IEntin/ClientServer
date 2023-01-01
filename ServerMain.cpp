/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Chronometer.h"
#include "Globals.h"
#include "Metrics.h"
#include "ServerOptions.h"
#include "TaskController.h"
#include "Logger.h"
#include <cassert>
#include <csignal>
#include <cstring>
 
void signalHandler([[maybe_unused]] int signal) {
  Globals::_stopFlag.test_and_set();
}

int main() {
  struct DoAtEnd {
    DoAtEnd() = default;
    ~DoAtEnd() {
      Metrics::print();
    }
  } doAtEnd;
  try {
    signal(SIGPIPE, SIG_IGN);
    std::signal(SIGINT, signalHandler);
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);
    sigset_t set;
    sigemptyset(&set);
    if (sigaddset(&set, SIGINT) == -1)
      LogError << ' ' << strerror(errno) << std::endl;
    ServerOptions options("ServerOptions.json");
    // optionally record elapsed times
    Chronometer chronometer(options._timingEnabled, __FILE__, __LINE__);
    if (!TaskController::create(options))
      return 3;
    int sig = 0;
    if (sigwait(&set, &sig))
      LogError << ' ' << strerror(errno) << std::endl;
    Metrics::save();
    TaskController::destroy();
    int closed = fcloseall();
    assert(closed == 0);
    return 0;
  }
  catch (const std::exception& e) {
    LogError << ':' << e.what() << std::endl;
    return 5;
  }
  catch (...) {
    LogError << ':' << strerror(errno) << std::endl;
    return 6;
  }
}

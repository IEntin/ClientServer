/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Chronometer.h"
#include "Metrics.h"
#include "ServerOptions.h"
#include "TaskController.h"
#include "Utility.h"
#include <cassert>
#include <csignal>

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
    std::signal(SIGINT, signalHandler);
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);
    sigset_t set;
    sigemptyset(&set);
    if (sigaddset(&set, SIGINT) == -1)
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << strerror(errno) << std::endl;
    ServerOptions options("ServerOptions.json");
    // optionally record elapsed times
    Chronometer chronometer(options._timingEnabled, __FILE__, __LINE__);
    if (!TaskController::start(options))
      return 3;
    int sig = 0;
    if (sigwait(&set, &sig))
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << strerror(errno) << std::endl;
    Metrics::save();
    TaskController::stop();
    int closed = fcloseall();
    assert(closed == 0);
    return 0;
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << std::endl;
    return 5;
  }
  catch (...) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << strerror(errno) << std::endl;
    return 6;
  }
}

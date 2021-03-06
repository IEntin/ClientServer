/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Chronometer.h"
#include "ServerOptions.h"
#include "TaskController.h"
#include "Utility.h"
#include <csignal>

void signalHandler([[maybe_unused]] int signal) {}

int main() {
  try {
    signal(SIGPIPE, SIG_IGN);
    std::signal(SIGINT, signalHandler);
    sigset_t set;
    sigemptyset(&set);
    if (sigaddset(&set, SIGINT) == -1)
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << strerror(errno) << '\n';
    ServerOptions options("ServerOptions.json");
    // optionally record elapsed times
    Chronometer chronometer(options._timingEnabled, __FILE__, __LINE__);
    RunnablePtr taskController = TaskController::instance(&options);
    if (!taskController->start())
      return 3;
    int sig = 0;
    if (sigwait(&set, &sig))
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << strerror(errno) << '\n';
    taskController->stop();
    int ret = fcloseall();
    assert(ret == 0);
    return 0;
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << '\n';
    return 5;
  }
  catch (...) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << strerror(errno) << '\n';
    return 6;
  }
}

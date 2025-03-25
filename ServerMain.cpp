/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <cassert>
#include <csignal>

#include "EchoPolicy.h"
#include "Metrics.h"
#include "ServerOptions.h"
#include "Server.h"
#include "NoSortInputPolicy.h"
#include "SortInputPolicy.h"
#include "Utility.h"

void signalHandler([[maybe_unused]] int signal) {}

int main() {
  std::string terminal(getenv("GNOME_TERMINAL_SCREEN"));
  utility::setServerTerminal(terminal);
  atexit(Server::removeNamedMutex);
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
    ServerOptions::parse("ServerOptions.json");
    POLICY policyEnum = ServerOptions::_policy;
    EchoPolicy echoPolicy;
    SortInputPolicy sortInputPolicy;
    NoSortInputPolicy noSortInputPolicy;
    std::reference_wrapper<Policy> policyRef = noSortInputPolicy;
    switch (std::to_underlying(policyEnum)) {
    case std::to_underlying(POLICY::NOSORTINPUT) :
      break;
    case std::to_underlying(POLICY::SORTINPUT) :
      policyRef = sortInputPolicy;
      break;
    case std::to_underlying(POLICY::ECHOPOLICY) :
      policyRef = echoPolicy;
      break;
    default:
      assert(false);
      break;
    }
    ServerPtr server = std::make_shared<Server>(policyRef);
    if (!server->start())
      return 3;
    int sig = 0;
    if (sigwait(&set, &sig))
      LogError << strerror(errno) << '\n';
    Metrics::save();
    server->stop();
    Metrics::print();
    int closed = fcloseall();
    assert(closed == 0);
    return 0;
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
    return 5;
  }
}

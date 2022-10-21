/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Chronometer.h"
#include "ClientOptions.h"
#include "FifoClient.h"
#include "TcpClient.h"
#include "Utility.h"
#include <csignal>

volatile std::sig_atomic_t stopSignal;

void signal_handler(int signal) {
  stopSignal = signal;
}

int main() {
  ClientOptions options("ClientOptions.json");
  const std::string communicationType = options._communicationType;
  const bool useFifo = communicationType == "FIFO";
  const bool useTcp = communicationType == "TCP";
  std::signal(SIGINT, signal_handler);
  Chronometer chronometer(options._timing, __FILE__, __LINE__, __func__);
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);
  std::cout.tie(nullptr);
  signal(SIGPIPE, SIG_IGN);
  try {
    if (useFifo) {
      fifo::FifoClient client(options);
      if (!client.run())
	return 1;
    }
    if (useTcp) {
      tcp::TcpClient client(options);
      if (!client.run())
	return 2;
    }
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-' << e.what() << '\n';
    return 3;
  }
  catch (...) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-' << std::strerror(errno) << '\n';
    return 4;
  }
  return 0;
}

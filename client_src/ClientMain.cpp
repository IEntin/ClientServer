/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Chronometer.h"
#include "ClientOptions.h"
#include "FifoClient.h"
#include "ProgramOptions.h"
#include "TcpClient.h"
#include <csignal>

int main() {
  const std::string communicationType = ProgramOptions::get("CommunicationType", std::string());
  const bool useFifo = communicationType == "FIFO";
  const bool useTcp = communicationType == "TCP";
  bool timing = ProgramOptions::get("Timing", false);
  Chronometer chronometer(timing, __FILE__, __LINE__, __func__);
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);
  std::cout.tie(nullptr);
  signal(SIGPIPE, SIG_IGN);
  if (useFifo) {
    FifoClientOptions options;
    fifo::FifoClient client(options);
    if (!client.run())
      return 1;
  }
  if (useTcp) {
    TcpClientOptions options;
    tcp::TcpClient client(options);
    if (!client.run())
      return 2;
  }
  return 0;
}

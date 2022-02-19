/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Chronometer.h"
#include "ClientOptions.h"
#include "FifoClient.h"
#include "MemoryPool.h"
#include "ProgramOptions.h"
#include "TcpClient.h"
#include <csignal>
#include <iostream>

int main() {
  const std::string communicationType = ProgramOptions::get("CommunicationType", std::string());
  const bool useFifo = communicationType == "FIFO";
  const bool useTcp = communicationType == "TCP";
  try {
    bool timing = ProgramOptions::get("Timing", false);
    Chronometer chronometer(timing, __FILE__, __LINE__, __func__);
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);
    signal(SIGPIPE, SIG_IGN);
    MemoryPool::setup(ProgramOptions::get("DYNAMIC_BUFFER_SIZE", 100000));
    chronometer.start(__FILE__, __func__, __LINE__);
    std::string sourceName = ProgramOptions::get("SourceName", std::string());
    Batch payload;
    Client::createPayload(sourceName.c_str(), payload);
    chronometer.stop(__FILE__, __func__, __LINE__);
    if (useFifo) {
      FifoClientOptions options;
      fifo::FifoClient client(options);
      if (!client.run(payload))
	return 1;
    }
    if (useTcp) {
      TcpClientOptions options;
      tcp::TcpClient client(options);
      if (!client.run(payload))
	return 1;
    }
  }
  catch (std::exception& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' '
	      << std::strerror(errno) << '-' << e.what() << std::endl;
    return 2;
  }
  catch (...) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' '
	      << std::strerror(errno) << std::endl;
    return 3;
  }
  return 0;
}

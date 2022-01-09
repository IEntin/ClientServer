/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Chronometer.h"
#include "FifoClient.h"
#include "ProgramOptions.h"
#include "Utility.h"
#include <csignal>
#include <iostream>

void signalHandler(int signal) {
  fifo::stop();
}

size_t createPayload(Batch& payload) {
  std::string sourceName = ProgramOptions::get("SourceName", std::string());
  std::ifstream input(sourceName, std::ifstream::in | std::ifstream::binary);
  if (!input)
    throw std::runtime_error(sourceName);
  unsigned long long requestIndex = 0;
  std::string line;
  Batch batch;
  while (std::getline(input, line)) {
    if (line.empty())
      continue;
    std::string modifiedLine(utility::createRequestId(requestIndex++));
    modifiedLine.append(line.append(1, '\n'));
    payload.emplace_back(std::move(modifiedLine));
  }
  return payload.size();
}

int main() {
  try {
    const bool timing = ProgramOptions::get("Timing", false);
    const std::string instrumentationFn = ProgramOptions::get("InstrumentationFn", std::string());
    std::ostream* instrStream = nullptr;
    std::ofstream instrFileStream;
    if (!instrumentationFn.empty()) {
      instrFileStream.open(instrumentationFn);
      instrStream = &instrFileStream;
    }
    Chronometer chronometer(timing, __FILE__, __LINE__, __func__, instrStream);
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);
    signal(SIGPIPE, SIG_IGN);
    std::signal(SIGINT, signalHandler);
    chronometer.start(__FILE__, __func__, __LINE__);
    Batch payload;
    createPayload(payload);
    chronometer.stop(__FILE__, __func__, __LINE__);
    const bool runLoop = ProgramOptions::get("RunLoop", false);
    const std::string outpuFileName = ProgramOptions::get("OutputFileName", std::string());
    std::ostream* dataStream = nullptr;
    std::ofstream dataFileStream;
    unsigned maxNumberTasks = 0;
    if (!outpuFileName.empty()) {
      dataFileStream.open(outpuFileName, std::ofstream::binary);
      dataStream = &dataFileStream;
      maxNumberTasks = ProgramOptions::get("MaxNumberTasks", 100);
    }
    else
      maxNumberTasks = std::numeric_limits<unsigned int>::max();
    if (!fifo::run(payload, runLoop, maxNumberTasks, dataStream, instrStream))
      return 1;
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

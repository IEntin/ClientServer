#include "Chronometer.h"
#include "FifoClient.h"
#include "ProgramOptions.h"
#include "Utility.h"
#include <csignal>
#include <iostream>

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
    std::string modifiedLine(utility::createIndexPrefix(requestIndex++));
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
    chronometer.start(__FILE__, __func__, __LINE__);
    Batch payload;
    createPayload(payload);
    chronometer.stop(__FILE__, __func__, __LINE__);
    const bool runLoop = ProgramOptions::get("RunLoop", false);
    const std::string outpuFileName = ProgramOptions::get("OutputFileName", std::string());
    std::ostream* dataStream = nullptr;
    std::ofstream dataFileStream;
    unsigned maxNumberIterations = 0;
    if (!outpuFileName.empty()) {
      dataFileStream.open(outpuFileName, std::ofstream::binary);
      dataStream = &dataFileStream;
      maxNumberIterations = ProgramOptions::get("MaxNumberIterations", 100);
    }
    else
      maxNumberIterations = std::numeric_limits<unsigned int>::max();
    if (!fifo::run(payload, runLoop, maxNumberIterations, dataStream, instrStream))
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

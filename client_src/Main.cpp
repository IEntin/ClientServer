/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Chronometer.h"
#include "ClientOptions.h"
#include "CommUtility.h"
#include "Compression.h"
#include "FifoClient.h"
#include "MemoryPool.h"
#include "ProgramOptions.h"
#include "TcpClient.h"
#include "Utility.h"
#include <atomic>
#include <csignal>
#include <iostream>

volatile std::atomic<bool> stopFlag = false;

void signalHandler(int signal) {
  stopFlag.store(true);
}

// placed here to limit the number of property_tree includes
std::ofstream ClientOptions::_dataFileStream;
std::ofstream ClientOptions::_instrFileStream;

ClientOptions::ClientOptions() :
  _diagnostics(ProgramOptions::get("Diagnostics", false)),
  _runLoop(ProgramOptions::get("RunLoop", false)),
  _prepareOnce(ProgramOptions::get("PrepareBatchOnce", false)),
  _timing(ProgramOptions::get("Timing", false)),
  _dataStream(initStream(ProgramOptions::get("OutputFileName", std::string()), _dataFileStream)),
  _maxNumberTasks(_dataStream ? ProgramOptions::get("MaxNumberTasks", 100) :
		  std::numeric_limits<unsigned int>::max()),
  _instrStream(initStream(ProgramOptions::get("InstrumentationFn", std::string()), _instrFileStream)) {}

TcpClientOptions::TcpClientOptions() : ClientOptions(),
  _serverHost(ProgramOptions::get("ServerHost", std::string())),
  _tcpPort(ProgramOptions::get("TcpPort", std::string())) {}

FifoClientOptions::FifoClientOptions() : ClientOptions(),
  _fifoName(ProgramOptions::get("FifoDirectoryName", std::string()) + '/' +
	    ProgramOptions::get("FifoBaseName", std::string())) {}

std::ostream* ClientOptions::initStream(const std::string& fileName,
					std::ofstream& fileStream) {
  std::string outpuFileName = ProgramOptions::get("OutputFileName", std::string());
  if (!fileName.empty()) {
    fileStream.open(fileName, std::ofstream::binary);
    return &fileStream;
  }
  return nullptr;
}

int main() {
  const std::string communicationType = ProgramOptions::get("CommunicationType", std::string());
  const bool useFifo = communicationType == "FIFO";
  const bool useTcp = communicationType == "TCP";
  try {
    Chronometer chronometer(true, __FILE__, __LINE__, __func__);
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);
    signal(SIGPIPE, SIG_IGN);
    std::signal(SIGINT, signalHandler);
    MemoryPool::setup(ProgramOptions::get("DYNAMIC_BUFFER_SIZE", 100000));
    chronometer.start(__FILE__, __func__, __LINE__);
    std::string compressorStr = ProgramOptions::get("Compression", std::string());
    Compression::setCompressionEnabled(compressorStr);
    std::string sourceName = ProgramOptions::get("SourceName", std::string());
    Batch payload;
    commutility::createPayload(sourceName.c_str(), payload);
    chronometer.stop(__FILE__, __func__, __LINE__);
    if (useFifo) {
      FifoClientOptions options;
      if (!fifo::run(payload, options))
	return 1;
    }
    if (useTcp) {
      TcpClientOptions options;
      if (!tcp::run(payload, options))
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

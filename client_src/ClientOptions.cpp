/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ClientOptions.h"
#include "ProgramOptions.h"
#include <filesystem>

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

std::ostream* ClientOptions::initStream(const std::string& fileName, std::ofstream& fileStream) {
  std::string outpuFileName = ProgramOptions::get("OutputFileName", std::string());
  if (!fileName.empty()) {
    fileStream.open(fileName, std::ofstream::binary);
    return &fileStream;
  }
  return nullptr;
}

TcpClientOptions::TcpClientOptions() : ClientOptions(),
  _serverHost(ProgramOptions::get("ServerHost", std::string("localhost"))),
  _tcpPort(ProgramOptions::get("TcpPort", std::string("49172"))) {}

FifoClientOptions::FifoClientOptions() : ClientOptions(),
  _fifoName(ProgramOptions::get("FifoDirectoryName", std::filesystem::current_path().string()) + '/' +
	    ProgramOptions::get("FifoBaseName", std::string("client1"))) {}

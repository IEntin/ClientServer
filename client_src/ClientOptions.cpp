/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ClientOptions.h"
#include "Compression.h"
#include "ProgramOptions.h"
#include <filesystem>

ClientOptions::ClientOptions(const std::pair<COMPRESSORS, bool>& compression,
			     std::ostream* externalDataStream) :
  _compression(compression),
  _diagnostics(ProgramOptions::get("Diagnostics", false)),
  _runLoop(ProgramOptions::get("RunLoop", false)),
  _prepareOnce(ProgramOptions::get("PrepareBatchOnce", false)),
  _timing(ProgramOptions::get("Timing", false)),
  _dataStream(initDataStream(externalDataStream,
			     ProgramOptions::get("OutputFileName", std::string()),
			     _dataFileStream)),
  _maxNumberTasks(_dataStream ? ProgramOptions::get("MaxNumberTasks", 100) :
		  std::numeric_limits<unsigned int>::max()),
  _instrStream(initInstrStream(ProgramOptions::get("InstrumentationFn", std::string()), _instrFileStream)) {}

std::ostream* ClientOptions::initDataStream(std::ostream* externalDataStream,
					    const std::string& fileName,
					    std::ofstream& fileStream) {
  if (externalDataStream)
    return externalDataStream;
  else if (!fileName.empty()) {
      fileStream.open(fileName, std::ofstream::binary);
      return &fileStream;
  }
  else
    return nullptr;
}

std::ostream* ClientOptions::initInstrStream(const std::string& fileName, std::ofstream& fileStream) {
  if (!fileName.empty()) {
    fileStream.open(fileName, std::ofstream::binary);
    return &fileStream;
  }
  return nullptr;
}

TcpClientOptions::TcpClientOptions(const std::pair<COMPRESSORS, bool>& compression,
				   std::ostream* externalDataStream) :
  ClientOptions(compression, externalDataStream),
  _serverHost(ProgramOptions::get("ServerHost", std::string("localhost"))),
  _tcpPort(ProgramOptions::get("TcpPort", std::string("49172"))) {}

FifoClientOptions::FifoClientOptions(const std::pair<COMPRESSORS, bool>& compression,
				     std::ostream* externalDataStream) :
  ClientOptions(compression, externalDataStream),
  _fifoName(ProgramOptions::get("FifoDirectoryName", std::filesystem::current_path().string()) + '/' +
	    ProgramOptions::get("FifoBaseName", std::string("client1"))) {}
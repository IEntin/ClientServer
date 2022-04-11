/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ClientOptions.h"
#include "ProgramOptions.h"
#include <filesystem>
#include <iostream>

ClientOptions::ClientOptions(std::ostream* externalDataStream) :
  _turnOffLogging(ProgramOptions::get("TurnOffLogging", true)),
  _sourceName(ProgramOptions::get("SourceName", std::string("data/requests.log"))),
  _bufferSize(ProgramOptions::get("DYNAMIC_BUFFER_SIZE", 100000)),
  _compressor(Compression::isCompressionEnabled(
         ProgramOptions::get("Compression", std::string(LZ4)))),
  _diagnostics(ProgramOptions::get("Diagnostics", false)),
  _runLoop(ProgramOptions::get("RunLoop", false)),
  _timing(ProgramOptions::get("Timing", false)),
  _dataStream([=]()->std::ostream* {
		if (externalDataStream)
		  return externalDataStream;
		else {
		  const std::string fileName = ProgramOptions::get("OutputFileName", std::string());
		  if (!fileName.empty()) {
		    static std::ofstream fileStream(fileName, std::ofstream::binary);
		    return &fileStream;
		  }
		  else
		    return nullptr;
		}
	      }()),
  _maxNumberTasks(ProgramOptions::get("MaxNumberTasks", 0)),
  _instrStream([]()->std::ostream* {
		 const std::string fileName = ProgramOptions::get("InstrumentationFn", std::string());
		 if (!fileName.empty()) {
		   static std::ofstream instrFileStream(fileName, std::ofstream::binary);
		   return &instrFileStream;
		 }
		 return nullptr;
	       }()) {
  // disable clog
  if (_turnOffLogging)
    std::clog.rdbuf(nullptr);
}

TcpClientOptions::TcpClientOptions(std::ostream* externalDataStream) :
  ClientOptions(externalDataStream),
  _serverHost(ProgramOptions::get("ServerHost", std::string("localhost"))),
  _tcpPort(ProgramOptions::get("TcpPort", std::string("49172"))) {}

FifoClientOptions::FifoClientOptions(std::ostream* externalDataStream) :
  ClientOptions(externalDataStream),
  _fifoName(ProgramOptions::get("FifoDirectoryName", std::filesystem::current_path().string()) + '/' +
	    ProgramOptions::get("FifoBaseName", std::string("client1"))) {}

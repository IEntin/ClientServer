/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "AppOptions.h"
#include "ClientOptions.h"
#include "Compression.h"
#include <filesystem>
#include <iostream>

ClientOptions::ClientOptions(const std::string& jsonName, std::ostream* externalDataStream) :
  _appOptions(jsonName),
  _communicationType(_appOptions.get("CommunicationType", std::string(""))),
  _sourceName(_appOptions.get("SourceName", std::string("data/requests.log"))),
  _bufferSize(_appOptions.get("DYNAMIC_BUFFER_SIZE", 100000)),
  _dataStream([=]()->std::ostream* {
		if (externalDataStream)
		  return externalDataStream;
		else {
		  const std::string filename = _appOptions.get("OutputFileName", std::string());
		  if (!filename.empty()) {
		    static std::ofstream fileStream(filename, std::ofstream::binary);
		    return &fileStream;
		  }
		  else
		    return nullptr;
		}
	      }()),
  _instrStream([this]()->std::ostream* {
		 const std::string filename = _appOptions.get("InstrumentationFn", std::string());
		 if (!filename.empty()) {
		   static std::ofstream instrFileStream(filename, std::ofstream::binary);
		   return &instrFileStream;
		 }
		 return nullptr;
	       }()),
  _maxNumberTasks(_appOptions.get("MaxNumberTasks", 0)),
  _numberRepeatEINTR(_appOptions.get("NumberRepeatEINTR", 3)),
  _numberRepeatENXIO(_appOptions.get("NumberRepeatENXIO", 10)),
  _ENXIOwait(_appOptions.get("ENXIOwai", 20000)),
  _serverHost(_appOptions.get("ServerHost", std::string("localhost"))),
  _tcpPort(_appOptions.get("TcpPort", std::string("49172"))),
  _fifoName(_appOptions.get("FifoDirectoryName", std::filesystem::current_path().string()) + '/' +
	    _appOptions.get("FifoBaseName", std::string("client1"))),
  _compressor(Compression::isCompressionEnabled(
         _appOptions.get("Compression", std::string(LZ4)))),
  _diagnostics(_appOptions.get("Diagnostics", false)),
  _runLoop(_appOptions.get("RunLoop", false)),
  _timing(_appOptions.get("Timing", false)),
  _setPipeSize(_appOptions.get("SetPipeSize", true)),
  _turnOffLogging(_appOptions.get("TurnOffLogging", true)) {
  // disable clog
  if (_turnOffLogging)
    std::clog.rdbuf(nullptr);
}

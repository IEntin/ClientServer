/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ClientOptions.h"
#include "AppOptions.h"
#include "Compression.h"
#include <iostream>

ClientOptions::ClientOptions(std::string_view jsonName, std::ostream* externalDataStream) :
  Options(jsonName) {
  AppOptions appOptions(jsonName);
  std::string clientType = appOptions.get("ClientType", std::string(""));
  _fifoClient = clientType == "FIFO";
  _tcpClient = clientType == "TCP";
  _compressor = compression::translateName(appOptions.get("Compression", std::string("LZ4")));
  _encrypted = appOptions.get("Encrypted", false);
  _serverAddress = appOptions.get("ServerAddress", std::string("127.0.0.1"));
  _sourceName = appOptions.get("SourceName", std::string("data/requests.log"));
  if (externalDataStream)
    _dataStream = externalDataStream;
  else {
    const std::string filename = appOptions.get("OutputFileName", std::string());
    static std::ofstream fileStream(filename, std::ofstream::binary);
    if (!filename.empty())
      _dataStream = &fileStream;
    else
      _dataStream = nullptr;
  }
  const std::string filename = appOptions.get("InstrumentationFn", std::string());
  if (!filename.empty()) {
    static std::ofstream instrFileStream(filename, std::ofstream::binary);
    _instrStream = &instrFileStream;
  }
  else
    _instrStream = nullptr;
  _maxNumberTasks = appOptions.get("MaxNumberTasks", 0);
  _heartbeatPeriod = appOptions.get("HeartbeatPeriod", 5000);
  _heartbeatTimeout = appOptions.get("HeartbeatTimeout", 2000);
  _heartbeatEnabled = appOptions.get("HeartbeatEnabled", true);
  _diagnostics = appOptions.get("Diagnostics", false);
  _runLoop = appOptions.get("RunLoop", false);
  _bufferSize = appOptions.get("DYNAMIC_BUFFER_SIZE", 100000);
}

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ClientOptions.h"
#include "AppOptions.h"
#include <iostream>

ClientOptions::ClientOptions(std::string_view jsonName, std::ostream* externalDataStream) :
  Options(jsonName) {
  AppOptions appOptions(jsonName);
  _clientType = appOptions.get("ClientType", std::string(""));
  _fifoClient = _clientType == "FIFO";
  _tcpClient = _clientType == "TCP";
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
}

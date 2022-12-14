/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ClientOptions.h"
#include "AppOptions.h"
#include "Compression.h"
#include <filesystem>
#include <iostream>

ClientOptions::ClientOptions(const std::string& jsonName, std::ostream* externalDataStream) {
  AppOptions appOptions(jsonName);
  _communicationType = appOptions.get("CommunicationType", std::string(""));
  _fifoClient = _communicationType == "FIFO";
  _tcpClient = _communicationType == "TCP";
  _sourceName = appOptions.get("SourceName", std::string("data/requests.log"));
  _bufferSize = appOptions.get("DYNAMIC_BUFFER_SIZE", 100000);
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
  _numberRepeatEINTR = appOptions.get("NumberRepeatEINTR", 3);
  _numberRepeatENXIO = appOptions.get("NumberRepeatENXIO", 10);
  _ENXIOwait = appOptions.get("ENXIOwai", 20);
  _serverHost = appOptions.get("ServerHost", std::string("127.0.0.1"));
  _tcpPort = appOptions.get("TcpPort", std::string("49172"));
  _fifoDirectoryName = appOptions.get("FifoDirectoryName", std::filesystem::current_path().string());
  _acceptorName = _fifoDirectoryName + '/' + appOptions.get("AcceptorBaseName", std::string("acceptor"));
  _compressor = Compression::isCompressionEnabled(appOptions.get("Compression", std::string(LZ4)));
  _heartbeatPeriod = appOptions.get("HeartbeatPeriod", 30);
  _heartbeatTimeout = appOptions.get("HeartbeatTimeout", 15);
  _enableHeartbeat = appOptions.get("EnableHeartbeat", true);
  _diagnostics = appOptions.get("Diagnostics", false);
  _runLoop = appOptions.get("RunLoop", false);
  _timing = appOptions.get("Timing", false);
  _setPipeSize = appOptions.get("SetPipeSize", true);
  _turnOffLogging = appOptions.get("TurnOffLogging", true);
  // disable clog
  if (_turnOffLogging)
    std::clog.rdbuf(nullptr);
}

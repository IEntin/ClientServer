/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ClientOptions.h"

#include <filesystem>

#include "AppOptions.h"
#include "Header.h"
#include "Logger.h"

bool ClientOptions::_parsed = false;
bool ClientOptions::_fifoClient;
bool ClientOptions::_tcpClient;
std::string  ClientOptions::_fifoDirectoryName;
std::string  ClientOptions::_acceptorName;
COMPRESSORS ClientOptions::_compressor;
bool ClientOptions::_encrypted;
bool ClientOptions::_showKey;
std::string ClientOptions::_sourceName;
std::ostream* ClientOptions::_dataStream;
std::ostream* ClientOptions::_instrStream;
int ClientOptions::_maxNumberTasks;
int ClientOptions::_heartbeatPeriod;
int ClientOptions::_heartbeatTimeout;
bool ClientOptions::_heartbeatEnabled;
DIAGNOSTICS ClientOptions::_diagnostics;
bool ClientOptions::_runLoop;
std::size_t ClientOptions::_bufferSize;
bool ClientOptions::_timing;
int ClientOptions::_numberRepeatENXIO;
bool ClientOptions::_setPipeSize;
std::size_t ClientOptions::_pipeSize;
std::string ClientOptions::_serverAddress;
unsigned short ClientOptions::_tcpPort;
bool ClientOptions::_printHeader;

void ClientOptions::parse(std::string_view jsonName, std::ostream* externalDataStream) {
  _parsed = true;
  AppOptions appOptions(jsonName);
  std::string clientType = appOptions.get("ClientType", std::string(""));
  _fifoClient = clientType == "FIFO";
  _tcpClient = clientType == "TCP";
  _fifoDirectoryName = appOptions.get("FifoDirectoryName", std::filesystem::current_path().string());
  _acceptorName = _fifoDirectoryName + '/' + appOptions.get("AcceptorBaseName", std::string("acceptor"));
  _compressor = translateCompressorString(appOptions.get("Compression", std::string("LZ4")));
  _encrypted = appOptions.get("Crypto", true);
  _showKey = appOptions.get("ShowKey", false);
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
  _heartbeatPeriod = appOptions.get("HeartbeatPeriod", 15000);
  _heartbeatTimeout = appOptions.get("HeartbeatTimeout", 3000);
  _heartbeatEnabled = appOptions.get("HeartbeatEnabled", true);
  _diagnostics = translateDiagnosticsString(appOptions.get("Diagnostics", std::string("Disabled")));
  _runLoop = appOptions.get("RunLoop", false);
  _bufferSize = appOptions.get("BufferSize", 100000);
  _timing = appOptions.get("Timing", false);
  _numberRepeatENXIO = appOptions.get("NumberRepeatENXIO", 200);
  _setPipeSize = appOptions.get("SetPipeSize", true);
  _pipeSize = appOptions.get("PipeSize", 1000000);
  _serverAddress = appOptions.get("ServerAddress", std::string("127.0.0.1"));
  _tcpPort = appOptions.get("TcpPort", 49151);
  _printHeader = appOptions.get("PrintHeader", false);
  Logger::translateLogThreshold(appOptions.get("LogThreshold", std::string("ERROR")));
}

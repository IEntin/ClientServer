/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ClientOptions.h"

#include <filesystem>
#include <fstream>

#include "BoostJsonParser.h"
#include "Logger.h"
#include "Options.h"

bool ClientOptions::_fifoClient;
bool ClientOptions::_tcpClient;
COMPRESSORS ClientOptions::_compressor;
int ClientOptions::_compressionLevel;
bool ClientOptions::_doEncrypt(false);
boost::static_string<100> ClientOptions::_sourceName;
std::ostream* ClientOptions::_dataStream;
std::ostream* ClientOptions::_instrStream(nullptr);
int ClientOptions::_maxNumberTasks(0);
int ClientOptions::_heartbeatPeriod(15000);
int ClientOptions::_heartbeatTimeout(3000);
bool ClientOptions::_heartbeatEnabled(true);
DIAGNOSTICS ClientOptions::_diagnostics(translateDiagnosticsString("Disabled"));
bool ClientOptions::_runLoop(false);
std::size_t ClientOptions::_bufferSize(100000);
bool ClientOptions::_timing(false);
bool ClientOptions::_printHeader(false);
boost::static_string<100> ClientOptions::_logThresholdName("ERROR");

void ClientOptions::parse(std::string_view jsonName, std::ostream* externalDataStream) {
  if (!jsonName.empty()) {
    Options::parse(jsonName);
    parseJson(jsonName, Options::_jv);
    auto clientType = Options::_jv.at("ClientType").as_string();
    _fifoClient = clientType == "FIFO";
    _tcpClient = clientType == "TCP";
    _compressor = translateCompressorString(Options::_jv.at("Compression").as_string());
    _compressionLevel = Options::_jv.at("CompressionLevel").as_int64();
    _doEncrypt = Options::_jv.at("doEncrypt").as_bool();
    _sourceName = Options::_jv.at("SourceName").as_string();
    if (externalDataStream)
      _dataStream = externalDataStream;
    else {
      const auto filename = Options::_jv.at("OutputFileName").as_string();
      static std::ofstream fileStream(filename.data(), std::ofstream::binary);
      if (!filename.empty())
	_dataStream = &fileStream;
      else
	_dataStream = nullptr;
    }
    const auto filename = Options::_jv.at("InstrumentationFn").as_string();
    _maxNumberTasks = Options::_jv.at("MaxNumberTasks").as_int64();
    _heartbeatPeriod = Options::_jv.at("HeartbeatPeriod").as_int64();
    _heartbeatTimeout = Options::_jv.at("HeartbeatTimeout").as_int64();
    _heartbeatEnabled = Options::_jv.at("HeartbeatEnabled").as_bool();
    _diagnostics = translateDiagnosticsString(Options::_jv.at("Diagnostics").as_string());
    _runLoop = Options::_jv.at("RunLoop").as_bool();
    _bufferSize = Options::_jv.at("BufferSize").as_int64();
    _timing = Options::_jv.at("Timing").as_bool();
    _printHeader = Options::_jv.at("PrintHeader").as_bool();
    _logThresholdName = Options::_jv.at("LogThreshold").as_string();
  }
  else {
    _sourceName = "data/requests.log";
    if (externalDataStream)
      _dataStream = externalDataStream;
    else {
      _dataStream = nullptr;
    }
  }
  Logger::translateLogThreshold(_logThresholdName);
}

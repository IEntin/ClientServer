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
std::ostream* ClientOptions::_dataStream = nullptr;
std::ostream* ClientOptions::_instrStream(nullptr);
int ClientOptions::_maxNumberTasks(0);
int ClientOptions::_heartbeatPeriod(15000);
int ClientOptions::_heartbeatTimeout(3000);
bool ClientOptions::_heartbeatEnabled(true);
DIAGNOSTICS ClientOptions::_diagnostics(DIAGNOSTICS::NONE);
bool ClientOptions::_runLoop(false);
std::size_t ClientOptions::_bufferSize(100000);
bool ClientOptions::_timing(false);
bool ClientOptions::_printHeader(false);

void ClientOptions::parse(std::string_view jsonName, std::ostream* externalDataStream) {
  _dataStream = externalDataStream? externalDataStream : nullptr;
  try {
  if (_jvC.is_null() && !jsonName.empty())
    parseJson(jsonName, _jvC);
  }
  catch (const std::exception& e) {
    std::cerr << "Parse error: " << e.what() << '\n';
    std::exit(3);
  }
  if (!_jvC.is_null()) {
    extractMatching(_jvC);
    auto clientType = _jvC.at("ClientType").as_string();
    _fifoClient = clientType == "FIFO";
    _tcpClient = clientType == "TCP";
    _compressor = translateCompressorString(_jvC.at("Compression").as_string());
    _compressionLevel = _jvC.at("CompressionLevel").as_int64();
    _doEncrypt = _jvC.at("doEncrypt").as_bool();
    _sourceName = _jvC.at("SourceName").as_string();
    const auto filename = _jvC.at("InstrumentationFn").as_string();
    _maxNumberTasks = _jvC.at("MaxNumberTasks").as_int64();
    _heartbeatPeriod = _jvC.at("HeartbeatPeriod").as_int64();
    _heartbeatTimeout = _jvC.at("HeartbeatTimeout").as_int64();
    _heartbeatEnabled = _jvC.at("HeartbeatEnabled").as_bool();
    _diagnostics = translateDiagnosticsString(_jvC.at("Diagnostics").as_string());
    _runLoop = _jvC.at("RunLoop").as_bool();
    _bufferSize = _jvC.at("BufferSize").as_int64();
    _timing = _jvC.at("Timing").as_bool();
    _printHeader = _jvC.at("PrintHeader").as_bool();
    _logThresholdName = _jvC.at("LogThreshold").as_string();
  }
  _sourceName = "data/requests.log";
  Logger::translateLogThreshold(_logThresholdName);
}

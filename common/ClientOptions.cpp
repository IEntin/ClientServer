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
std::string ClientOptions::_sourceName;
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

void ClientOptions::parse(std::string_view jsonName, std::ostream* externalDataStream) {
  if (!jsonName.empty()) {
    Options::parse(jsonName);
    boost::json::value jv;
    parseJson(jsonName, jv);
    auto clientType = jv.at("ClientType").as_string();
    _fifoClient = clientType == "FIFO";
    _tcpClient = clientType == "TCP";
    _compressor = translateCompressorString(jv.at("Compression").as_string());
    _compressionLevel = jv.at("CompressionLevel").as_int64();
    _doEncrypt = jv.at("doEncrypt").as_bool();
    _sourceName = jv.at("SourceName").as_string();
    if (externalDataStream)
      _dataStream = externalDataStream;
    else {
      const auto filename = jv.at("OutputFileName").as_string();
      static std::ofstream fileStream(filename.data(), std::ofstream::binary);
      if (!filename.empty())
	_dataStream = &fileStream;
      else
	_dataStream = nullptr;
    }
    const auto filename = jv.at("InstrumentationFn").as_string();
    _maxNumberTasks = jv.at("MaxNumberTasks").as_int64();
    _heartbeatPeriod = jv.at("HeartbeatPeriod").as_int64();
    _heartbeatTimeout = jv.at("HeartbeatTimeout").as_int64();
    _heartbeatEnabled = jv.at("HeartbeatEnabled").as_bool();
    _diagnostics = translateDiagnosticsString(jv.at("Diagnostics").as_string());
    _runLoop = jv.at("RunLoop").as_bool();
    _bufferSize = jv.at("BufferSize").as_int64();
    _timing = jv.at("Timing").as_bool();
    _printHeader = jv.at("PrintHeader").as_bool();
    Logger::translateLogThreshold(jv.at("LogThreshold").as_string());
  }
  else {
    _sourceName = "data/requests.log";
    if (externalDataStream)
      _dataStream = externalDataStream;
    else {
      _dataStream = nullptr;
    }
    Logger::translateLogThreshold("ERROR");
  }
}

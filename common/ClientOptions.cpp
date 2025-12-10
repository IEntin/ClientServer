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
bool ClientOptions::_doEncrypt;
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
bool ClientOptions::_printHeader;

void ClientOptions::parse(std::string_view jsonName, std::ostream* externalDataStream) {
  Options::parse(jsonName);
  if (!jsonName.empty()) {
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
    _doEncrypt = false;
    _sourceName = "data/requests.log";
    if (externalDataStream)
      _dataStream = externalDataStream;
    else {
      _dataStream = nullptr;
    }
    std::string filename = "";
    _instrStream = nullptr;
    _maxNumberTasks = 0;
    _heartbeatPeriod =  15000;
    _heartbeatTimeout =  3000;
    _heartbeatEnabled =  true;
    _diagnostics = translateDiagnosticsString("Disabled");
    _runLoop =  false;
    _bufferSize = 100000;
    _timing = false;
    _printHeader =  false;
    Logger::translateLogThreshold("ERROR");
  }
}

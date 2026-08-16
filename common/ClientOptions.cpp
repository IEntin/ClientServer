/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ClientOptions.h"

#include <filesystem>
#include <fstream>

#include "BoostJsonParser.h"
#include "Client.h"
#include "Logger.h"
#include "Options.h"

CLIENT_TYPE ClientOptions::_clientType;
CLIENT_TYPE _clientType;
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
  if (Client::Client::_jvC.is_null() && !jsonName.empty())
    parseJson(jsonName, Client::Client::_jvC);
  }
  catch (const std::exception& e) {
    std::cerr << "Parse error: " << e.what() << '\n';
    std::exit(3);
  }
  if (!Client::Client::_jvC.is_null()) {
    extractMatching(Client::Client::_jvC);
    _clientType = translateClientType(Client::_jvC.at("ClientType").as_string());
    _compressor = translateCompressorString(Client::_jvC.at("Compression").as_string());
    _compressionLevel = Client::_jvC.at("CompressionLevel").as_int64();
    _doEncrypt = Client::_jvC.at("doEncrypt").as_bool();
    _sourceName = Client::_jvC.at("SourceName").as_string();
    const auto filename = Client::_jvC.at("InstrumentationFn").as_string();
    _maxNumberTasks = Client::_jvC.at("MaxNumberTasks").as_int64();
    _heartbeatPeriod = Client::_jvC.at("HeartbeatPeriod").as_int64();
    _heartbeatTimeout = Client::_jvC.at("HeartbeatTimeout").as_int64();
    _heartbeatEnabled = Client::_jvC.at("HeartbeatEnabled").as_bool();
    _diagnostics = translateDiagnosticsString(Client::_jvC.at("Diagnostics").as_string());
    _runLoop = Client::_jvC.at("RunLoop").as_bool();
    _bufferSize = Client::_jvC.at("BufferSize").as_int64();
    _timing = Client::_jvC.at("Timing").as_bool();
    _printHeader = Client::_jvC.at("PrintHeader").as_bool();
    _logThresholdName = Client::_jvC.at("LogThreshold").as_string();
  }
  Logger::translateLogThreshold(_logThresholdName);
}

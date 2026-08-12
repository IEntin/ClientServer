/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ServerOptions.h"

#include <filesystem>
#include <thread>

#include "BoostJsonParser.h"
#include "Logger.h"

boost::static_string<100> ServerOptions::_adsFileName;
COMPRESSORS ServerOptions::_compressor;
int ServerOptions::_compressionLevel;
bool ServerOptions::_doEncrypt;
int ServerOptions::_numberWorkThreads;
int ServerOptions::_maxTcpSessions;
int ServerOptions::_maxFifoSessions;
int ServerOptions::_maxTotalSessions;
int ServerOptions::_tcpTimeout;
bool ServerOptions::_useRegex;
POLICYENUM ServerOptions::_policyEnum;
std::size_t ServerOptions::_bufferSize;
bool ServerOptions::_timing;
bool ServerOptions::_printHeader;
boost::static_string<100> ServerOptions::_logThresholdName;

void ServerOptions::parse(std::string_view jsonName) {
  if (!jsonName.empty()) {
    Options::parse(jsonName);
    parseJson(jsonName, Options::_jv);
    _adsFileName = Options::_jv.at("AdsFileName").as_string();
    _compressor = translateCompressorString(Options::_jv.at("Compression").as_string());
    _compressionLevel = Options::_jv.at("CompressionLevel").as_int64();
    _doEncrypt = Options::_jv.at("doEncrypt").as_bool();
    int numberWorkThreadsCfg = Options::_jv.at("NumberWorkThreads").as_int64();
    _numberWorkThreads = numberWorkThreadsCfg ? numberWorkThreadsCfg : std::thread::hardware_concurrency();
    _maxTcpSessions = Options::_jv.at("MaxTcpSessions").as_int64();
    _maxFifoSessions = Options::_jv.at("MaxFifoSessions").as_int64();
    _maxTotalSessions = Options::_jv.at("MaxTotalSessions").as_int64();
    _tcpTimeout = Options::_jv.at("TcpTimeout").as_int64();
    _useRegex = Options::_jv.at("UseRegex").as_bool();
    _policyEnum = fromString(Options::_jv.at("Policy").as_string());
    _bufferSize = Options::_jv.at("BufferSize").as_int64();
    _timing = Options::_jv.at("Timing").as_bool();
    _printHeader = Options::_jv.at("PrintHeader").as_bool();
    _logThresholdName = Options::_jv.at("LogThreshold").as_string();
  }
  else {
    _policyEnum = fromString("NOSORTINPUT");
  }
  Logger::translateLogThreshold(_logThresholdName);
}

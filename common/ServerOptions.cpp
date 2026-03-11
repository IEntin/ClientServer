/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ServerOptions.h"

#include <filesystem>
#include <thread>

#include "BoostJsonParser.h"
#include "Logger.h"

boost::static_string<100> ServerOptions::_adsFileName("data/ads.txt");
COMPRESSORS ServerOptions::_compressor(translateCompressorString("LZ4"));
int ServerOptions::_compressionLevel(3);
bool ServerOptions::_doEncrypt(true);
int ServerOptions::_numberWorkThreads(std::thread::hardware_concurrency());
int ServerOptions::_maxTcpSessions(2);
int ServerOptions::_maxFifoSessions(2);
int ServerOptions::_maxTotalSessions(2);
int ServerOptions::_tcpTimeout(3000);
bool ServerOptions::_useRegex(true);
POLICYENUM ServerOptions::_policyEnum;
std::size_t ServerOptions::_bufferSize(100000);
bool ServerOptions::_timing(false);
bool ServerOptions::_printHeader(false);
boost::static_string<100> ServerOptions::_logThresholdName("ERROR");

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

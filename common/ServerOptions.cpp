/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ServerOptions.h"

#include <filesystem>
#include <thread>

#include "BoostJsonParser.h"
#include "Logger.h"

std::string ServerOptions::_adsFileName("data/ads.txt");
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

void ServerOptions::parse(std::string_view jsonName) {
  if (!jsonName.empty()) {
    Options::parse(jsonName);
    boost::json::value jv;
    parseJson(jsonName, jv);
    _adsFileName = jv.at("AdsFileName").as_string();
    _compressor = translateCompressorString(jv.at("Compression").as_string());
    _compressionLevel = jv.at("CompressionLevel").as_int64();
    _doEncrypt = jv.at("doEncrypt").as_bool();
    int numberWorkThreadsCfg = jv.at("NumberWorkThreads").as_int64();
    _numberWorkThreads = numberWorkThreadsCfg ? numberWorkThreadsCfg : std::thread::hardware_concurrency();
    _maxTcpSessions = jv.at("MaxTcpSessions").as_int64();
    _maxFifoSessions = jv.at("MaxFifoSessions").as_int64();
    _maxTotalSessions = jv.at("MaxTotalSessions").as_int64();
    _tcpTimeout = jv.at("TcpTimeout").as_int64();
    _useRegex = jv.at("UseRegex").as_bool();
    _policyEnum = fromString(jv.at("Policy").as_string());
    _bufferSize = jv.at("BufferSize").as_int64();
    _timing = jv.at("Timing").as_bool();
    _printHeader = jv.at("PrintHeader").as_bool();
    Logger::translateLogThreshold(jv.at("LogThreshold").as_string());
  }
  else {
    _policyEnum = fromString("NOSORTINPUT");
    Logger::translateLogThreshold("LogThreshold");
  }
}

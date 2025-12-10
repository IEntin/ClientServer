/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ServerOptions.h"

#include <filesystem>
#include <thread>

#include "BoostJsonParser.h"
#include "Logger.h"

std::string ServerOptions::_adsFileName;
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

void ServerOptions::parse(std::string_view jsonName) {
  Options::parse(jsonName);
  if (!jsonName.empty()) {
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
    _adsFileName = "data/ads.txt";
    _compressor = translateCompressorString("LZ4");
    _compressionLevel = 3;
    _doEncrypt = true;
    _numberWorkThreads = std::thread::hardware_concurrency();
    _maxTcpSessions = 2;
    _maxFifoSessions = 2;
    _maxTotalSessions = 2;
    _tcpTimeout = 3000;
    _useRegex = true;
    _policyEnum = fromString("NOSORTINPUT");
    _bufferSize = 100000;
    _timing = false;
    _printHeader = false;
    Logger::translateLogThreshold("LogThreshold");//, std::string("ERROR")));
  }
}

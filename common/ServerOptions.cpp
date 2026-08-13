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
boost::json::value ServerOptions::_jvS;

void ServerOptions::parse(std::string_view jsonName) {
  try {
  if (_jvS.is_null() && !jsonName.empty())
    parseJson(jsonName, _jvS);
  }
  catch (const std::exception& e) {
    std::cerr << "Parse error: " << e.what() << '\n';
    std::exit(3);
  }
  if (!_jvS.is_null()) {
    extractMatching(_jvS);
    _adsFileName = _jvS.at("AdsFileName").as_string();
    _compressor = translateCompressorString(_jvS.at("Compression").as_string());
    _compressionLevel = _jvS.at("CompressionLevel").as_int64();
    _doEncrypt = _jvS.at("doEncrypt").as_bool();
    int numberWorkThreadsCfg = _jvS.at("NumberWorkThreads").as_int64();
    _numberWorkThreads = numberWorkThreadsCfg ? numberWorkThreadsCfg : std::thread::hardware_concurrency();
    _maxTcpSessions = _jvS.at("MaxTcpSessions").as_int64();
    _maxFifoSessions = _jvS.at("MaxFifoSessions").as_int64();
    _maxTotalSessions = _jvS.at("MaxTotalSessions").as_int64();
    _tcpTimeout = _jvS.at("TcpTimeout").as_int64();
    _useRegex = _jvS.at("UseRegex").as_bool();
    _policyEnum = fromString(_jvS.at("Policy").as_string());
    _bufferSize = _jvS.at("BufferSize").as_int64();
    _timing = _jvS.at("Timing").as_bool();
    _printHeader = _jvS.at("PrintHeader").as_bool();
    _logThresholdName = _jvS.at("LogThreshold").as_string();
    Logger::translateLogThreshold(_logThresholdName);
  }
}

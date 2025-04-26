/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ServerOptions.h"

#include <filesystem>
#include <thread>

#include "AppOptions.h"
#include "Header.h"
#include "Logger.h"

std::string ServerOptions::_adsFileName;
COMPRESSORS ServerOptions::_compressor;
int ServerOptions::_compressionLevel;
CRYPTO ServerOptions::_encryption;
bool ServerOptions::_showKey;
int ServerOptions::_numberWorkThreads;
int ServerOptions::_maxTcpSessions;
int ServerOptions::_maxFifoSessions;
int ServerOptions::_maxTotalSessions;
int ServerOptions::_tcpTimeout;
bool ServerOptions::_useRegex;
POLICYENUM ServerOptions::_policyEnum;
bool ServerOptions::_timing;

void ServerOptions::parse(std::string_view jsonName) {
  Options::parse(jsonName);
  AppOptions appOptions(jsonName);
  _adsFileName = appOptions.get("AdsFileName", std::string("data/ads.txt"));
  _compressor = translateCompressorString(appOptions.get("Compression", std::string("LZ4")));
  _compressionLevel = appOptions.get("CompressionLevel", 3);
  _encryption = translateCryptoString(appOptions.get("Crypto", std::string("CRYPTOPP")));
  _showKey = appOptions.get("ShowKey", false);
  int numberWorkThreadsCfg = appOptions.get("NumberWorkThreads", 0);
  _numberWorkThreads = numberWorkThreadsCfg ? numberWorkThreadsCfg : std::thread::hardware_concurrency();
  _maxTcpSessions = appOptions.get("MaxTcpSessions", 2);
  _maxFifoSessions = appOptions.get("MaxFifoSessions", 2);
  _maxTotalSessions = appOptions.get("MaxTotalSessions", 2);
  _tcpTimeout = appOptions.get("TcpTimeout", 3000);
  _useRegex = appOptions.get("UseRegex", true);
  _policyEnum = fromString(appOptions.get("Policy", std::string("NOSORTINPUT")));
  _timing = appOptions.get("Timing", false);
  Logger::translateLogThreshold(appOptions.get("LogThreshold", std::string("ERROR")));
}

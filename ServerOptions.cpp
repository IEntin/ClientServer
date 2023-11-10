/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ServerOptions.h"
#include "AppOptions.h"
#include "Compression.h"
#include "Logger.h"
#include <thread>

std::string ServerOptions::_adsFileName;
COMPRESSORS ServerOptions::_compressor;
bool ServerOptions::_encrypted;
bool ServerOptions::_invalidateKey;
bool ServerOptions::_showKey;
int ServerOptions::_numberWorkThreads;
int ServerOptions::_maxTcpSessions;
int ServerOptions::_maxFifoSessions;
int ServerOptions::_maxTotalSessions;
unsigned short ServerOptions::_tcpPort;
int ServerOptions::_tcpTimeout;
bool ServerOptions::_sortInput;
bool ServerOptions::_timing;
int ServerOptions::_numberRepeatENXIO;
int ServerOptions::_ENXIOwait;

void ServerOptions::parse(std::string_view jsonName) {
  AppOptions appOptions(jsonName);
  Options::parse(appOptions);
  _adsFileName = appOptions.get("AdsFileName", std::string("data/ads.txt"));
  _compressor = compression::translateName(appOptions.get("Compression", std::string("LZ4")));
  _encrypted = appOptions.get("Encrypted", false);
  _invalidateKey = appOptions.get("InvalidateKey", false);
  _showKey = appOptions.get("ShowKey", false);
  int numberWorkThreadsCfg = appOptions.get("NumberWorkThreads", 0);
  _numberWorkThreads = numberWorkThreadsCfg ? numberWorkThreadsCfg : std::thread::hardware_concurrency();
  _maxTcpSessions = appOptions.get("MaxTcpSessions", 2);
  _maxFifoSessions = appOptions.get("MaxFifoSessions", 2);
  _maxTotalSessions = appOptions.get("MaxTotalSessions", 2);
  _tcpPort = appOptions.get("TcpPort", 49151);
  _tcpTimeout = appOptions.get("TcpTimeout", 3000);
  _sortInput = appOptions.get("SortInput", true);
  _timing = appOptions.get("Timing", false);
  // next 2 parameters may be decreased for better responsiveness
  // or increased to prevent deadlocking on slow machines.
  _numberRepeatENXIO = appOptions.get("NumberRepeatENXIO", 50);
  _ENXIOwait = appOptions.get("ENXIOwai", 10);
  Logger::translateLogThreshold(appOptions.get("LogThreshold", std::string("ERROR")));
}

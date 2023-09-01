/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ServerOptions.h"
#include "AppOptions.h"
#include "Compression.h"
#include <iostream>
#include <thread>

void ServerOptions::parse(std::string_view jsonName) {
  AppOptions appOptions(jsonName);
  Options::parse(appOptions);
  _adsFileName = appOptions.get("AdsFileName", std::string("data/ads.txt"));
  _compressor = compression::translateName(appOptions.get("Compression", std::string("LZ4")));
  _encrypted = appOptions.get("Encrypted", false);
  int numberWorkThreadsCfg = appOptions.get("NumberWorkThreads", 0);
  _numberWorkThreads = numberWorkThreadsCfg ? numberWorkThreadsCfg : std::thread::hardware_concurrency();
  _maxTcpSessions = appOptions.get("MaxTcpSessions", 2);
  _maxFifoSessions = appOptions.get("MaxFifoSessions", 2);
  _maxTotalSessions = appOptions.get("MaxTotalSessions", 2);
  _tcpTimeout = appOptions.get("TcpTimeout", 1000);
  _invalidateKey = appOptions.get("InvalidateKey", true);
  _sortInput = appOptions.get("SortInput", true);
}

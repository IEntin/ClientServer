/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ServerOptions.h"
#include "AppOptions.h"
#include <iostream>
#include <thread>

ServerOptions::ServerOptions(const std::string& jsonName) :
  Options(jsonName) {
  AppOptions appOptions(jsonName);
  _processType = appOptions.get("ProcessType", std::string("Transaction"));
  _adsFileName = appOptions.get("AdsFileName", std::string("data/ads.txt"));
  int numberWorkThreadsCfg = appOptions.get("NumberWorkThreads", 0);
  _numberWorkThreads = numberWorkThreadsCfg ? numberWorkThreadsCfg : std::jthread::hardware_concurrency();
  _maxTcpSessions = appOptions.get("MaxTcpSessions", 2);
  _maxFifoSessions = appOptions.get("MaxFifoSessions", 2);
  _tcpPort = appOptions.get("TcpPort", 49172);
  _tcpTimeout = appOptions.get("TcpTimeout", 1000);
  _sortInput = appOptions.get("SortInput", true);
}

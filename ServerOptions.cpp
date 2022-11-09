/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ServerOptions.h"
#include "AppOptions.h"
#include "Compression.h"
#include <filesystem>
#include <iostream>
#include <thread>

ServerOptions::ServerOptions(const std::string& jsonName) {
  AppOptions appOptions(jsonName);
  _processType = appOptions.get("ProcessType", std::string("Transaction"));
  _bufferSize = appOptions.get("DYNAMIC_BUFFER_SIZE", 100000);
  _adsFileName = appOptions.get("AdsFileName", std::string("data/ads.txt"));
  _fifoDirectoryName = appOptions.get("FifoDirectoryName", std::filesystem::current_path().string());
  _acceptorName = _fifoDirectoryName + '/' + appOptions.get("AcceptorBaseName", std::string("acceptor"));
  int numberWorkThreadsCfg = appOptions.get("NumberTaskThreads", 0);
  _numberWorkThreads = numberWorkThreadsCfg ? numberWorkThreadsCfg : std::jthread::hardware_concurrency();
  _maxTcpSessions = appOptions.get("MaxTcpSessions", 2);
  _maxFifoSessions = appOptions.get("MaxFifoSessions", 2);
  _tcpPort = appOptions.get("TcpPort", 49172);
  _tcpTimeout = appOptions.get("TcpTimeout", 5000);
  _heartbeatPeriod = appOptions.get("HeartbeatPeriod", 5);
  _numberRepeatEINTR = appOptions.get("NumberRepeatEINTR", 3);
  _numberRepeatENXIO = appOptions.get("NumberRepeatENXIO", 10);
  _ENXIOwait = appOptions.get("ENXIOwai", 20);
  _compressor = Compression::isCompressionEnabled(appOptions.get("Compression", std::string(LZ4)));
  _timingEnabled = appOptions.get("Timing", false);
  _turnOffLogging = appOptions.get("TurnOffLogging", true);
  // disable clog
  if (_turnOffLogging)
    std::clog.rdbuf(nullptr);
  _sortInput = appOptions.get("SortInput", true);
  _setPipeSize = appOptions.get("SetPipeSize", true);
}

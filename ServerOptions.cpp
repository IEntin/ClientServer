/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ServerOptions.h"
#include "Compression.h"
#include "Transaction.h"
#include <filesystem>
#include <iostream>
#include <thread>

ServerOptions::ServerOptions(const std::string& jsonName) :
  _appOptions(jsonName),
  _bufferSize(_appOptions.get("DYNAMIC_BUFFER_SIZE", 100000)),
  _adsFileName(_appOptions.get("AdsFileName", std::string("data/ads.txt"))),
  _fifoDirectoryName(_appOptions.get("FifoDirectoryName", std::filesystem::current_path().string())),
  _fifoBaseNames(_appOptions.get("FifoBaseNames", std::string("client1"))),
  _processRequest(Transaction::processRequest),
  _numberWorkThreads([this] ()->int {
		       int numberWorkThreadsCfg = _appOptions.get("NumberTaskThreads", 0);
		       return numberWorkThreadsCfg > 0 ? numberWorkThreadsCfg :
			 std::thread::hardware_concurrency();}()),
  _maxConnections(_appOptions.get("MaxConnections", 2)),
  _tcpPort(_appOptions.get("TcpPort", 49172)),
  _tcpTimeout(_appOptions.get("TcpTimeout", 1)),
  _numberRepeatEINTR(_appOptions.get("NumberRepeatEINTR", 3)),
  _numberRepeatENXIO(_appOptions.get("NumberRepeatENXIO", 10)),
  _ENXIOwait(_appOptions.get("ENXIOwai", 10000)),
  _compressor(Compression::isCompressionEnabled(_appOptions.get("Compression", std::string(LZ4)))),
  _timingEnabled(_appOptions.get("Timing", false)),
  _turnOffLogging(_appOptions.get("TurnOffLogging", true)),
  _sortInput(_appOptions.get("SortInput", true)),
  _setPipeSize(_appOptions.get("SetPipeSize", true)),
  _destroyBufferOnClientDisconnect(_appOptions.get("DestroyBufferOnClientDisconnect", true)) {
  // disable clog
  if (_turnOffLogging)
    std::clog.rdbuf(nullptr);
}

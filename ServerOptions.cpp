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
  _turnOffLogging(_appOptions.get("TurnOffLogging", true)),
  _sortInput(_appOptions.get("SortInput", true)),
  _bufferSize(_appOptions.get("DYNAMIC_BUFFER_SIZE", 100000)),
  _timingEnabled(_appOptions.get("Timing", false)),
  _processRequest(Transaction::processRequest),
  _adsFileName(_appOptions.get("AdsFileName", std::string("data/ads.txt"))),
  _numberWorkThreads([this] ()->int {
		       int numberWorkThreadsCfg = _appOptions.get("NumberTaskThreads", 0);
		       return numberWorkThreadsCfg > 0 ? numberWorkThreadsCfg :
			 std::thread::hardware_concurrency();}()),
  _compressor(Compression::isCompressionEnabled(_appOptions.get("Compression", std::string(LZ4)))),
  _expectedTcpConnections(_appOptions.get("ExpectedTcpConnections", 1)),
  _tcpPort(_appOptions.get("TcpPort", 49172)),
  _tcpTimeout(_appOptions.get("TcpTimeout", 1)),
  _fifoDirectoryName(_appOptions.get("FifoDirectoryName", std::filesystem::current_path().string())),
  _fifoBaseNames(_appOptions.get("FifoBaseNames", std::string("client1"))),
  _numberRepeatEINTR(_appOptions.get("NumberRepeatEINTR", 3)),
  _numberRepeatENXIO(_appOptions.get("NumberRepeatENXIO", 10)),
  _ENXIOwait(_appOptions.get("ENXIOwai", 10000)),
  _setPipeSize(_appOptions.get("SetPipeSize", true)) {
  // disable clog
  if (_turnOffLogging)
    std::clog.rdbuf(nullptr);
}

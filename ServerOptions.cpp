/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ServerOptions.h"
#include "Compression.h"
#include "ProgramOptions.h"
#include "Transaction.h"
#include <filesystem>
#include <iostream>
#include <thread>

ServerOptions::ServerOptions() :
  _turnOffLogging(ProgramOptions::get("TurnOffLogging", true)),
  _sortInput(ProgramOptions::get("SortInput", true)),
  _bufferSize(ProgramOptions::get("DYNAMIC_BUFFER_SIZE", 100000)),
  _timingEnabled(ProgramOptions::get("Timing", false)),
  _processRequest(Transaction::processRequest),
  _adsFileName(ProgramOptions::get("AdsFileName", std::string("data/ads.txt"))),
  _numberWorkThreads([] ()->int {
		       int numberWorkThreadsCfg = ProgramOptions::get("NumberTaskThreads", 0);
		       return numberWorkThreadsCfg > 0 ? numberWorkThreadsCfg :
			 std::thread::hardware_concurrency();}()),
  _compressor(Compression::isCompressionEnabled(ProgramOptions::get("Compression", std::string(LZ4)))),
  _expectedTcpConnections(ProgramOptions::get("ExpectedTcpConnections", 1)),
  _tcpPort(ProgramOptions::get("TcpPort", 49172)),
  _tcpTimeout(ProgramOptions::get("TcpTimeout", 1)),
  _fifoDirectoryName(ProgramOptions::get("FifoDirectoryName", std::filesystem::current_path().string())),
  _fifoBaseNames(ProgramOptions::get("FifoBaseNames", std::string("client1"))),
  _numberRepeatEINTR(ProgramOptions::get("NumberRepeatEINTR", 3)),
  _numberRepeatENXIO(ProgramOptions::get("NumberRepeatENXIO", 10)),
  _ENXIOwait(ProgramOptions::get("ENXIOwai", 10000)),
  _setPipeSize(ProgramOptions::get("SetPipeSize", true)) {
  // disable clog
  if (_turnOffLogging)
    std::clog.rdbuf(nullptr);
}

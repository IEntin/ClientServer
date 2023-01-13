/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Options.h"
#include "AppOptions.h"
#include "Compression.h"
#include "Logger.h"
#include <filesystem>

Options::Options(const std::string& jsonName) {
  AppOptions appOptions(jsonName);
  _bufferSize = appOptions.get("DYNAMIC_BUFFER_SIZE", 100000);
  _fifoDirectoryName = appOptions.get("FifoDirectoryName", std::filesystem::current_path().string());
  _acceptorName = _fifoDirectoryName + '/' + appOptions.get("AcceptorBaseName", std::string("acceptor"));
  _serverAddress = appOptions.get("ServerAddress", std::string("127.0.0.1"));
  _tcpPort = appOptions.get("TcpPort", std::string("49152"));;
  _numberRepeatEINTR = appOptions.get("NumberRepeatEINTR", 3);
  _numberRepeatENXIO = appOptions.get("NumberRepeatENXIO", 10);
  _ENXIOwait = appOptions.get("ENXIOwai", 20);
  _compressor = Compression::isCompressionEnabled(appOptions.get("Compression", std::string(LZ4)));
  _timing = appOptions.get("Timing", false);
  _setPipeSize = appOptions.get("SetPipeSize", true);
  _logThreshold = translateLogThreshold(appOptions.get("LogThreshold", std::string("ERROR")));
}

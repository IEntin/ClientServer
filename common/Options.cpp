/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Options.h"
#include "AppOptions.h"
#include "CommonConstants.h"
#include "Compression.h"
#include "Logger.h"
#include <filesystem>

Options::Options(std::string_view jsonName) {
  AppOptions appOptions(jsonName);
  _bufferSize = appOptions.get("DYNAMIC_BUFFER_SIZE", 100000);
  _fifoDirectoryName = appOptions.get("FifoDirectoryName", std::filesystem::current_path().string());
  _acceptorName = _fifoDirectoryName + '/' + appOptions.get("AcceptorBaseName", std::string("acceptor"));
  _tcpPort = appOptions.get("TcpPort", 49151);
  _portString = std::to_string(_tcpPort);
  _tcpService = _portString;
  // next 2 parameters may be decreased for better responsiveness
  // or increased to prevent deadlocking on slow machines.
  _numberRepeatENXIO = appOptions.get("NumberRepeatENXIO", 50);
  _ENXIOwait = appOptions.get("ENXIOwai", 10);
  _compressor = compression::translateName(appOptions.get("Compression", std::string("LZ4")));
  _encrypted = appOptions.get("Encrypted", false);
  _showKey = appOptions.get("ShowKey", false);
  _timing = appOptions.get("Timing", false);
  _setPipeSize = appOptions.get("SetPipeSize", true);
  _pipeSize = appOptions.get("PipeSize", 1000000);
  _logThreshold = Logger::translateLogThreshold(appOptions.get("LogThreshold", std::string("ERROR")));
}

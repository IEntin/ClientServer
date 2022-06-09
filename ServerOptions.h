/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "AppOptions.h"
#include <string>

enum class COMPRESSORS : int;

using ProcessRequest = std::string (*)(std::string_view, std::string_view);

struct ServerOptions {
  explicit ServerOptions(const std::string& jsonName = "");
  ~ServerOptions() = default;
  AppOptions _appOptions;
  const bool _turnOffLogging;
  bool _sortInput;
  size_t _bufferSize;
  const bool _timingEnabled;
  const ProcessRequest _processRequest;
  const std::string _adsFileName;
  const int _numberWorkThreads;
  COMPRESSORS _compressor;
  const int _expectedTcpConnections;
  const int _tcpPort;
  const int _tcpTimeout;
  const std::string _fifoDirectoryName;
  const std::string _fifoBaseNames;
  const int _numberRepeatEINTR;
  const int _numberRepeatENXIO;
  const int _ENXIOwait;
  const bool _setPipeSize;
};

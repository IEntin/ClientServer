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
  size_t _bufferSize;
  const std::string _adsFileName;
  const std::string _fifoDirectoryName;
  const std::string _fifoBaseNames;
  const ProcessRequest _processRequest;
  const int _numberWorkThreads;
  const int _maxTcpConnections;
  const int _maxFifoConnections;
  const int _tcpPort;
  const int _tcpTimeout;
  const int _numberRepeatEINTR;
  const int _numberRepeatENXIO;
  const int _ENXIOwait;
  COMPRESSORS _compressor;
  const bool _timingEnabled;
  const bool _turnOffLogging;
  bool _sortInput;
  const bool _setPipeSize;
  const bool _destroyBufferOnClientDisconnect;
};

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "AppOptions.h"
#include <string>

enum class COMPRESSORS : char;

struct ServerOptions {
  explicit ServerOptions(const std::string& jsonName = "");
  ~ServerOptions() {}
  AppOptions _appOptions;
  size_t _bufferSize;
  std::string _adsFileName;
  std::string _fifoDirectoryName;
  std::string _acceptorBaseName;
  std::string _processType;
  int _numberWorkThreads;
  int _maxTcpConnections;
  int _maxFifoConnections;
  unsigned short _tcpAcceptorPort;
  unsigned short _tcpStatusPort;
  int _tcpTimeout;
  int _numberRepeatEINTR;
  int _numberRepeatENXIO;
  int _ENXIOwait;
  COMPRESSORS _compressor;
  bool _timingEnabled;
  bool _turnOffLogging;
  bool _sortInput;
  bool _setPipeSize;
  bool _destroyBufferOnClientDisconnect;
};

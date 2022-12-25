/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Logger.h"
#include <string>

enum class COMPRESSORS : char;

struct ServerOptions {
  explicit ServerOptions(const std::string& jsonName = "");
  ~ServerOptions() = default;
  std::string _processType;
  size_t _bufferSize;
  std::string _adsFileName;
  std::string _fifoDirectoryName;
  std::string _acceptorName;
  int _numberWorkThreads;
  unsigned _maxTcpSessions;
  unsigned _maxFifoSessions;
  unsigned short _tcpPort;
  int _tcpTimeout;
  int _numberRepeatEINTR;
  int _numberRepeatENXIO;
  int _ENXIOwait;
  COMPRESSORS _compressor;
  bool _timingEnabled;
  bool _sortInput;
  bool _setPipeSize;
  LOG_LEVEL _logThreshold;
};

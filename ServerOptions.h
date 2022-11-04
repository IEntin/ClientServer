/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

enum class COMPRESSORS : char;

struct ServerOptions {
  explicit ServerOptions(const std::string& jsonName = "");
  ~ServerOptions() {}
  size_t _bufferSize;
  std::string _adsFileName;
  std::string _fifoDirectoryName;
  std::string _acceptorName;
  std::string _processType;
  int _numberWorkThreads;
  unsigned _maxTcpSessions;
  unsigned _maxFifoSessions;
  unsigned short _tcpPort;
  int _tcpTimeout;
  int _heartbeatPeriod;
  int _numberRepeatEINTR;
  int _numberRepeatENXIO;
  int _ENXIOwait;
  COMPRESSORS _compressor;
  bool _timingEnabled;
  bool _turnOffLogging;
  bool _sortInput;
  bool _setPipeSize;
};

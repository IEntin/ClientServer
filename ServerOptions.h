/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Options.h"

struct ServerOptions : Options {
  explicit ServerOptions(const std::string& jsonName = "");
  ~ServerOptions() = default;
  std::string _processType;
  std::string _adsFileName;
  int _numberWorkThreads;
  int _maxTcpSessions;
  int _maxFifoSessions;
  int _maxTotalSessions;
  int _tcpTimeout;
  int _cryptoKeySize;
  bool _invalidateKeys;
  bool _sortInput;
};

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
  unsigned _maxTcpSessions;
  unsigned _maxFifoSessions;
  int _tcpTimeout;
  bool _sortInput;
};

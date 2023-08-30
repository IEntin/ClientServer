/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Options.h"

struct ServerOptions : Options {
  explicit ServerOptions(std::string_view jsonName = "");
  ~ServerOptions() override {}
  inline static std::string _adsFileName;
  int _numberWorkThreads;
  int _maxTcpSessions;
  int _maxFifoSessions;
  int _maxTotalSessions;
  inline static int _tcpTimeout;
  inline static bool _invalidateKey;
  bool _sortInput;
};

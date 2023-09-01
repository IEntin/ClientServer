/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Options.h"

struct ServerOptions : Options {
  explicit ServerOptions(std::string_view jsonName = "");
  ~ServerOptions() override {}
  inline static std::string _adsFileName;
  inline static COMPRESSORS _compressor;
  inline static bool _encrypted;
  inline static int _numberWorkThreads;
  inline static int _maxTcpSessions;
  inline static int _maxFifoSessions;
  inline static int _maxTotalSessions;
  inline static int _tcpTimeout;
  inline static bool _invalidateKey;
  inline static bool _sortInput;
};

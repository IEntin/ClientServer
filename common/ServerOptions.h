/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Options.h"

struct ServerOptions : Options {
  ServerOptions() = delete;
  ~ServerOptions() = delete;
  static void parse(std::string_view jsonName);
  static std::string _adsFileName;
  static COMPRESSORS _compressor;
  static bool _encrypted;
  static int _numberWorkThreads;
  static int _maxTcpSessions;
  static int _maxFifoSessions;
  static int _maxTotalSessions;
  static int _tcpTimeout;
  static bool _invalidateKey;
  static bool _sortInput;
};

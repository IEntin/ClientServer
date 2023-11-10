/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Options.h"

enum class COMPRESSORS : char;

struct ServerOptions : Options {
  ServerOptions() = delete;
  ~ServerOptions() = delete;
  static void parse(std::string_view jsonName);
  static std::string _adsFileName;
  static COMPRESSORS _compressor;
  static bool _encrypted;
  static bool _invalidateKey;
  static bool _showKey;
  static int _numberWorkThreads;
  static int _maxTcpSessions;
  static int _maxFifoSessions;
  static int _maxTotalSessions;
  static unsigned short _tcpPort;
  static int _tcpTimeout;
  static bool _sortInput;
  static bool _timing;
  static int _numberRepeatENXIO;
  static int _ENXIOwait;
};

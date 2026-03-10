/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Options.h"
#include "Policy.h"

enum class COMPRESSORS : char;

struct ServerOptions : Options {
  static void parse(std::string_view jsonName);
  static std::string _adsFileName;
  static COMPRESSORS _compressor;
  static int _compressionLevel;
  static bool _doEncrypt;
  static int _numberWorkThreads;
  static int _maxTcpSessions;
  static int _maxFifoSessions;
  static int _maxTotalSessions;
  static int _tcpTimeout;
  static bool _useRegex;
  static POLICYENUM _policyEnum;
  static bool _timing;
  static bool _printHeader;
private:
  ServerOptions() = delete;
  ~ServerOptions() = delete;
};

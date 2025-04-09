/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Options.h"
#include "Policy.h"

enum class COMPRESSORS : char;
enum class CRYPTO : char;

struct ServerOptions : protected Options {
  static void parse(std::string_view jsonName);
  static std::string _adsFileName;
  static COMPRESSORS _compressor;
  static int _compressionLevel;
  static CRYPTO _encryption;
  static bool _showKey;
  static int _numberWorkThreads;
  static int _maxTcpSessions;
  static int _maxFifoSessions;
  static int _maxTotalSessions;
  static int _tcpTimeout;
  static POLICYENUM _policyEnum;
  static bool _timing;
private:
  ServerOptions() = delete;
  ~ServerOptions() = delete;
};

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

enum class COMPRESSORS : char;
enum class CRYPTO : char;

struct ServerOptions {
  ServerOptions() = delete;
  ~ServerOptions() = delete;
  static void parse(std::string_view jsonName);
  static std::string _adsFileName;
  static COMPRESSORS _compressor;
  static bool _encrypted;
  static bool _showKey;
  static int _numberWorkThreads;
  static int _maxTcpSessions;
  static int _maxFifoSessions;
  static int _maxTotalSessions;
  static int _tcpTimeout;
  static bool _sortInput;
  static bool _timing;
  static bool _setPipeSize;
  static std::size_t _pipeSize;
  static bool _printHeader;
};

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <any>
#include <filesystem>

enum class COMPRESSORS : char;

struct ServerOptions {
  ServerOptions() {
    _self = *this;
  }
  ~ServerOptions() = default;
  static std::any _self;
  static void parse(std::string_view jsonName);
  static std::string _adsFileName;
  static std::string _fifoDirectoryName;
  static std::string _acceptorName;
  static COMPRESSORS _compressor;
  static bool _encrypted;
  static unsigned _cryptoKeySize;
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
  static bool _setPipeSize;
  static size_t _pipeSize;
};

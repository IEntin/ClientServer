/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <iostream>

enum class COMPRESSORS : char;
enum class CRYPTO : char;
enum class DIAGNOSTICS : char;

struct ClientOptions {
  ClientOptions() = delete;
  ~ClientOptions() = delete;
  static void parse(std::string_view jsonName, std::ostream* externalDataStream = nullptr);
  static bool _fifoClient;
  static bool _tcpClient;
  static COMPRESSORS _compressor;
  static bool _encrypted;
  static bool _showKey;
  static std::string _sourceName;
  static std::ostream* _dataStream;
  static std::ostream* _instrStream;
  // max number iterations when _runLoop is true,
  // unlimited if it is 0.
  static int _maxNumberTasks;
  static int _heartbeatPeriod;
  static int _heartbeatTimeout;
  static bool _heartbeatEnabled;
  static DIAGNOSTICS _diagnostics;
  static bool _runLoop;
  static std::size_t _bufferSize;
  static bool _timing;
  static bool _setPipeSize;
  static std::size_t _pipeSize;
  static bool _printHeader;
};

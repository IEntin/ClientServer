/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Options.h"
#include <fstream>

struct ClientOptions : Options {
  ClientOptions() = delete;
  ~ClientOptions() = delete;
  static void parse(std::string_view jsonName, std::ostream* externalDataStream = nullptr);
  static bool _fifoClient;
  static bool _tcpClient;
  static COMPRESSORS _compressor;
  static bool _encrypted;
  static std::string _serverAddress;
  static std::string _sourceName;
  static std::ostream* _dataStream;
  static std::ostream* _instrStream;
  // max number iterations when _runLoop is true,
  // unlimited if it is 0.
  static int _maxNumberTasks;
  static int _heartbeatPeriod;
  static int _heartbeatTimeout;
  static bool _heartbeatEnabled;
  static bool _diagnostics;
  static bool _runLoop;
  static size_t _bufferSize;
};
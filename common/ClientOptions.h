/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <iostream>

#include "Options.h"

struct ClientOptions : Options {
  static void parse(std::string_view jsonName, std::ostream* externalDataStream = nullptr);
  static CLIENT_TYPE _clientType;
  static COMPRESSORS _compressor;
  static int _compressionLevel;
  static bool _doEncrypt;
  inline static std::string _sourceName = "data/requests.log";
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
  static bool _printHeader;
  inline static std::string _logThresholdName = "ERROR";
  inline static boost::json::value _jvC;
private:
  ClientOptions() = delete;
  ~ClientOptions() = delete;
};

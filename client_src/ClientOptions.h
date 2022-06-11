/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "AppOptions.h"
#include <fstream>

enum class COMPRESSORS : int;

struct ClientOptions {
  explicit ClientOptions(const std::string& jsonName = "", std::ostream* externalDataStream = nullptr);

  AppOptions _appOptions;
  const std::string _communicationType;
  std::string _sourceName;
  size_t _bufferSize;
  std::ostream* _dataStream;
  std::ostream* _instrStream;
  // max number iterations when _runLoop is true,
  // unlimited if it is 0.
  const int _maxNumberTasks;
  const int _numberRepeatEINTR;
  const int _numberRepeatENXIO;
  const int _ENXIOwait;
  const std::string _serverHost;
  const std::string _tcpPort;
  const std::string _fifoName;
  COMPRESSORS _compressor;
  bool _diagnostics;
  const bool _runLoop;
  const bool _timing;
  const bool _setPipeSize;
  const bool _turnOffLogging;
};

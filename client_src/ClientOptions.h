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
  const bool _turnOffLogging;
  const std::string _communicationType;
  std::string _sourceName;
  size_t _bufferSize;
  COMPRESSORS _compressor;
  bool _diagnostics;
  const bool _runLoop;
  const bool _timing;
  std::ostream* _dataStream;
  // max number iterations when _runLoop is true,
  // unlimited if it is 0.
  const int _maxNumberTasks;
  std::ostream* _instrStream;
  const int _numberRepeatEINTR;
  const int _numberRepeatENXIO;
  const int _ENXIOwait;
  const std::string _serverHost;
  const std::string _tcpPort;
  const std::string _fifoName;
  const bool _setPipeSize;
};

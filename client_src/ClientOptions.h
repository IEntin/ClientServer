/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "AppOptions.h"
#include <fstream>

enum class COMPRESSORS : char;

struct ClientOptions {
  explicit ClientOptions(const std::string& jsonName = "", std::ostream* externalDataStream = nullptr);

  AppOptions _appOptions;
  std::string _communicationType;
  std::string _sourceName;
  size_t _bufferSize;
  std::ostream* _dataStream;
  std::ostream* _instrStream;
  // max number iterations when _runLoop is true,
  // unlimited if it is 0.
  int _maxNumberTasks;
  int _numberRepeatEINTR;
  int _numberRepeatENXIO;
  int _ENXIOwait;
  std::string _serverHost;
  std::string _tcpPort;
  bool _tcpHeartbeatEnabled;
  int _heartbeatPeriod;
  std::string _fifoDirectoryName;
  std::string _acceptorName;
  COMPRESSORS _compressor;
  bool _diagnostics;
  bool _runLoop;
  bool _timing;
  bool _setPipeSize;
  bool _turnOffLogging;
  int _numberBuilderThreads;
};

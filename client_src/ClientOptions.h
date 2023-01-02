/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Options.h"
#include <fstream>

struct ClientOptions : Options {
  explicit ClientOptions(const std::string& jsonName = "", std::ostream* externalDataStream = nullptr);
  ~ClientOptions() = default;
  std::string _communicationType;
  bool _fifoClient;
  bool _tcpClient;
  std::string _sourceName;
  std::ostream* _dataStream;
  std::ostream* _instrStream;
  // max number iterations when _runLoop is true,
  // unlimited if it is 0.
  int _maxNumberTasks;
  std::string _serverHost;
  std::string _tcpPort;
  int _heartbeatPeriod;
  int _heartbeatTimeout;
  bool _enableHeartbeat;
  bool _diagnostics;
  bool _runLoop;
};

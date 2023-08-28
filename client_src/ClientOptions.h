/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Options.h"
#include <fstream>

struct ClientOptions : Options {
  explicit ClientOptions(std::string_view jsonName = "", std::ostream* externalDataStream = nullptr);
  ~ClientOptions() = default;
  std::string _clientType;
  bool _fifoClient;
  bool _tcpClient;
  std::string _serverAddress;
  std::string _sourceName;
  std::ostream* _dataStream;
  std::ostream* _instrStream;
  // max number iterations when _runLoop is true,
  // unlimited if it is 0.
  int _maxNumberTasks;
  int _heartbeatPeriod;
  int _heartbeatTimeout;
  bool _heartbeatEnabled;
  bool _diagnostics;
  bool _runLoop;
};

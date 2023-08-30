/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Options.h"
#include <fstream>

struct ClientOptions : Options {
  explicit ClientOptions(std::string_view jsonName = "", std::ostream* externalDataStream = nullptr);
  ~ClientOptions() override {}
  inline static std::string _clientType;
  inline static bool _fifoClient;
  inline static bool _tcpClient;
  inline static std::string _serverAddress;
  std::string _sourceName;
  inline static std::ostream* _dataStream;
  inline static std::ostream* _instrStream;
  // max number iterations when _runLoop is true,
  // unlimited if it is 0.
  inline static int _maxNumberTasks;
  inline static int _heartbeatPeriod;
  inline static int _heartbeatTimeout;
  inline static bool _heartbeatEnabled;
  inline static bool _diagnostics;
  inline static bool _runLoop;
};

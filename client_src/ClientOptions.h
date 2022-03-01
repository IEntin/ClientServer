/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Compression.h"
#include <fstream>

struct ClientOptions {
  explicit ClientOptions(std::ostream* externalDataStream = nullptr);

  const std::string _sourceName;
  size_t _bufferSize;
  COMPRESSORS _compressor;
  const bool _diagnostics;
  const bool _runLoop;
  const bool _timing;
  std::ostream* _dataStream;
  // if output file specified limit the number of invocations
  // not to create big logs.
  const unsigned _maxNumberTasks;
  std::ostream* _instrStream;
};

struct TcpClientOptions : ClientOptions {
  explicit TcpClientOptions(std::ostream* externalDataStream = nullptr);

  const std::string _serverHost;
  const std::string _tcpPort;
};

struct FifoClientOptions : ClientOptions {
  explicit FifoClientOptions(std::ostream* externalDataStream = nullptr);

  const std::string _fifoName;
};

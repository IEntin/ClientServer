/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Compression.h"
#include <fstream>

struct ClientOptions {
  explicit ClientOptions(std::ostream* externalDataStream = nullptr);

  CompressionDescription _compression;
  const bool _diagnostics;
  const bool _runLoop;
  // Simulate fast client to measure server performance.
  // used with "RunLoop" : true
  const bool _prepareOnce;
  const bool _timing;
  std::ostream* _dataStream;
  // if output file specified limit the number of invocations
  // not to create big logs.
  const unsigned _maxNumberTasks;
  std::ostream* _instrStream;

  std::ofstream _dataFileStream;
  std::ofstream _instrFileStream;

  // redirect output to the file or external stream.
  std::ostream* initDataStream(std::ostream* externalDataStream,
			       const std::string& fileName,
			       std::ofstream& fileStream);
  std::ostream* initInstrStream(const std::string& fileName,
				std::ofstream& fileStream);
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

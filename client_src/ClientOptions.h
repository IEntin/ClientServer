#pragma once

#include <fstream>

struct ClientOptions {
  ClientOptions();

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

  static std::ofstream _dataFileStream;
  static std::ofstream _instrFileStream;

  // redirect output to the file if specified.
  std::ostream* initStream(const std::string& fileName,
			   std::ofstream& fileStream);
};

struct TcpClientOptions : ClientOptions {
  TcpClientOptions();

  const std::string _serverHost;
  const std::string _tcpPort;
};

struct FifoClientOptions : ClientOptions {
  FifoClientOptions();

  const std::string _fifoName;
};

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <fstream>

enum class COMPRESSORS : unsigned short;

struct ClientOptions {
  ClientOptions(const std::pair<COMPRESSORS, bool>& compression,
		std::ostream* externalDataStream = nullptr);

  const std::pair<COMPRESSORS, bool> _compression;
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
  TcpClientOptions(const std::pair<COMPRESSORS, bool>& compression,
		   std::ostream* externalDataStream = nullptr);

  const std::string _serverHost;
  const std::string _tcpPort;
};

struct FifoClientOptions : ClientOptions {
  FifoClientOptions(const std::pair<COMPRESSORS, bool>& compression,
		    std::ostream* externalDataStream = nullptr);

  const std::string _fifoName;
};

#pragma once

#include "Compression.h"

using ProcessRequest = std::string (*)(std::string_view);

struct ServerOptions {
  ServerOptions();
  ~ServerOptions() = default;
  const size_t _bufferSize;
  const bool _timingEnabled;
  const ProcessRequest _processRequest;
  const std::string _adsFileName;
  const unsigned _numberWorkThreads;
  const COMPRESSORS _compressor;
  const unsigned _expectedTcpConnections;
  const unsigned _tcpPort;
  const unsigned _tcpTimeout;
  const std::string _fifoDirectoryName;
  const std::string _fifoBaseNames;
};

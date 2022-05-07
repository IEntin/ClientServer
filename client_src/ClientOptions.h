/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <fstream>

enum class COMPRESSORS : short unsigned int;

struct ClientOptions {
  explicit ClientOptions(std::ostream* externalDataStream = nullptr);

  const bool _turnOffLogging;
  const std::string _sourceName;
  size_t _bufferSize;
  COMPRESSORS _compressor;
  bool _diagnostics;
  const bool _runLoop;
  const bool _timing;
  std::ostream* _dataStream;
  // max number iterations when _runLoop is true,
  // unlimited if it is 0.
  const unsigned _maxNumberTasks;
  std::ostream* _instrStream;
  const int _numberRepeatEINTR;
  const int _numberRepeatENXIO;
  const int _ENXIOwait;
  int getNumberRepeatEINTR() const { return _numberRepeatEINTR; }
  int getNumberRepeatENXIO() const { return _numberRepeatENXIO; }
  int getENXIOwait() const { return _ENXIOwait; }
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

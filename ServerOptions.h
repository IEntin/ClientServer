/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

enum class COMPRESSORS : short unsigned int;

using ProcessRequest = std::string (*)(std::string_view, std::string_view);

struct ServerOptions {
  ServerOptions();
  ~ServerOptions() = default;
  const bool _turnOffLogging;
  const bool _sortInput;
  size_t _bufferSize;
  const bool _timingEnabled;
  const ProcessRequest _processRequest;
  const std::string _adsFileName;
  const unsigned _numberWorkThreads;
  COMPRESSORS _compressor;
  const unsigned _expectedTcpConnections;
  const unsigned _tcpPort;
  const unsigned _tcpTimeout;
  const std::string _fifoDirectoryName;
  const std::string _fifoBaseNames;
  const int _numberRepeatEINTR;
  const int _numberRepeatENXIO;
  const int _ENXIOwait;
  int getNumberRepeatEINTR() const { return _numberRepeatEINTR; }
  int getNumberRepeatENXIO() const { return _numberRepeatENXIO; }
  int getENXIOwait() const { return _ENXIOwait; }
};

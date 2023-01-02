/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

enum class COMPRESSORS : char;
enum class LOG_LEVEL : char;

struct Options {
  explicit Options(const std::string& jsonName);
  ~Options() = default;
  size_t _bufferSize;
  std::string _fifoDirectoryName;
  std::string _acceptorName;
  unsigned short _tcpPort;
  std::string _tcpPortS;
  int _numberRepeatEINTR;
  int _numberRepeatENXIO;
  int _ENXIOwait;
  COMPRESSORS _compressor;
  bool _timing;
  bool _setPipeSize;
  LOG_LEVEL _logThreshold;
};

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

enum class COMPRESSORS : char;
enum class LOG_LEVEL : char;

struct Options {
  explicit Options(std::string_view jsonName);
  ~Options() = default;
  size_t _bufferSize;
  std::string _fifoDirectoryName;
  std::string _acceptorName;
  unsigned short _tcpPort;
  std::string_view _tcpService;
  int _numberRepeatENXIO;
  int _ENXIOwait;
  COMPRESSORS _compressor;
  bool _encrypted;
  bool _showKey;
  bool _timing;
  bool _setPipeSize;
  size_t _pipeSize;
  LOG_LEVEL _logThreshold;
  static int _cryptoKeySize;
private:
  std::string _portString;
};

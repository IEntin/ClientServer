/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

enum class COMPRESSORS : char;
enum class LOG_LEVEL : char;

// static variables will have the same value for server
// and client in tests running in the same process.

struct Options {
  explicit Options(std::string_view jsonName);
  virtual ~Options() {}
  size_t _bufferSize;
  inline static std::string _fifoDirectoryName;
  inline static std::string _acceptorName;
  inline static unsigned short _tcpPort;
  inline static std::string_view _tcpService;
  inline static int _numberRepeatENXIO;
  inline static int _ENXIOwait;
  COMPRESSORS _compressor;
  bool _encrypted;
  inline static bool _showKey;
  bool _timing;
  inline static bool _setPipeSize;
  inline static size_t _pipeSize;
  inline static int _cryptoKeySize = 32;
private:
  inline static std::string _portString;
};

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
  Options() = delete;
  ~Options() = delete;
  static void parse(class AppOptions& appOptions);
  static std::string _fifoDirectoryName;
  static std::string _acceptorName;
  static unsigned short _tcpPort;
  static std::string _tcpService;
  static int _numberRepeatENXIO;
  static int _ENXIOwait;
  static bool _showKey;
  static bool _timing;
  static bool _setPipeSize;
  static size_t _pipeSize;
  static int _cryptoKeySize;
};

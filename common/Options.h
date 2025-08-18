/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

#include "Header.h"

struct Options {
public:
  static void parse(std::string_view jsonName);
  static std::string _fifoDirectoryName;
  static std::string _acceptorBaseName;
  static std::string _acceptorName;
  static int _numberRepeatENXIO;
  static bool _setPipeSize;
  static std::size_t _pipeSize;
  static std::string _serverAddress;
  static unsigned short _tcpPort;
  static COMPRESSORS _compressor;
private:
  Options() = delete;
  ~Options() = delete;
};

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"

struct Options {
public:
  static void parse(std::string_view jsonName);
  static constexpr CRYPTO _encryptorTypeDefault = CRYPTO::CRYPTOSODIUM;
  static CRYPTO _encryptorType;
  static std::string _fifoDirectoryName;
  static std::string _acceptorBaseName;
  static std::string _acceptorName;
  static int _numberRepeatENXIO;
  static bool _setPipeSize;
  static std::size_t _pipeSize;
  static std::string _serverAddress;
  static unsigned short _tcpPort;
private:
  Options() = delete;
  ~Options() = delete;
};

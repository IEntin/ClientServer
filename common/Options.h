/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/json/value.hpp>
#include <boost/static_string.hpp>

#include <filesystem>
#include <iostream>

#include "Header.h"

struct Options {
public:
  static void parse(std::string_view jsonName);
  static constexpr bool _debug = false;
  static constexpr CRYPTO _primaryEncryptor = CRYPTO::CRYPTOSODIUM;
  static constexpr CRYPTO _secondaryEncryptor = CRYPTO::CRYPTOPP;
  static CRYPTO _singleEncryptor;
  static bool _doubleEncryption;
  static boost::static_string<100> _fifoDirectoryName;
  static boost::static_string<100> _acceptorBaseName;
  static boost::static_string<100> _acceptorName;
  static int _numberRepeatENXIO;
  static bool _setPipeSize;
  static std::size_t _pipeSize;
  static boost::static_string<100> _serverAddress;
  static unsigned short _tcpPort;
  static boost::json::value _jv;
private:
  Options() = delete;
  ~Options() = delete;
};

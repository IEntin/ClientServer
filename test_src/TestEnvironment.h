/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <sstream>

#include <gtest/gtest.h>

#include "CryptoVariant.h"
#include "ServerOptions.h"
#include "Utility.h"

class TestEnvironment : public ::testing::Environment {
public:

  TestEnvironment() = default;

  ~TestEnvironment() override = default;

  void SetUp() override;

  void TearDown() override;

  static void reset();

  static std::ostringstream _oss;
  static std::string _source;
  static std::string _outputD;
  static std::string _outputND;
  static std::string _outputAltFormatD;
  static thread_local std::string _buffer;

  };

  struct TestCompressEncrypt : testing::Test {
    static void testCompressEncrypt(COMPRESSORS compressor,
				    bool doEncrypt,
				    CRYPTO crypto) {
      // must be a copy
      std::string data = TestEnvironment::_source;
      HEADER header{ HEADERTYPE::SESSION,
		     0,
		     data.size(),
		     compressor,
		     DIAGNOSTICS::NONE,
		     STATUS::NONE,
		     0 };
      if (ServerOptions::_printHeader)
	printHeader(header, LOG_LEVEL::ALWAYS);
      CryptoVariant& clientVariant = cryptovariant::getEncryptorVariant(APPTYPE::CLIENT, crypto);
      std::string_view dataView =
	cryptovariant::compressEncrypt(clientVariant, TestEnvironment::_buffer, header, data, doEncrypt);
      HEADER restoredHeader;
      data = dataView;
      ASSERT_EQ(CryptoBase::isEncrypted(data), doEncrypt);
      cryptovariant::decryptDecompress(APPTYPE::SERVER, crypto, TestEnvironment::_buffer, restoredHeader, data);
      ASSERT_EQ(header, restoredHeader);
      ASSERT_EQ(data, TestEnvironment::_source);
    }
    void TearDown() {
      TestEnvironment::reset();
    }
  };

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <sstream>

#include <gtest/gtest.h>

#include "CryptoTuple.h"
#include "CryptoOperations.h"
#include "ServerOptions.h"
#include "TaskBuilder.h"
#include "Utility.h"

using namespace cryptooperations;

class TestEnvironment : public ::testing::Environment {
public:

  TestEnvironment() = default;

  ~TestEnvironment() override = default;

  void SetUp() override;

  void TearDown() override;

  static void reset();

  static std::ostringstream _oss;
  static std::string _source;
  static inline std::string _outputD;
  static inline std::string _outputND;
  static inline std::string _outputAltFormatD;
  static thread_local std::string _buffer;

  };

  struct TestCompressEncrypt : testing::Test {
    static void testCompressEncrypt(COMPRESSORS compressor,
				    bool doEncrypt,
				    CRYPTO crypto) {
      std::string& data = TestEnvironment::_source;
      HEADER header{ HEADERTYPE::SESSION,
		     0,
		     data.size(),
		     compressor,
		     DIAGNOSTICS::NONE,
		     STATUS::NONE,
		     0,
		     0 };
      if (ServerOptions::_printHeader)
	printHeader(header, LOG_LEVEL::ALWAYS);
      const CryptoTuple& clientTuple = cryptotuple::getClientEncryptorTuple();
      std::string_view dataView =
	compressSingleEncrypt(clientTuple, crypto, TestEnvironment::_buffer, header, data, doEncrypt);
      HEADER restoredHeader;
      data = dataView;
      ASSERT_EQ(CryptoBase::isEncrypted(data), doEncrypt);
      const CryptoTuple& serverTuple = cryptotuple::getServerEncryptorTuple();
      singleDecryptDecompress(serverTuple, crypto, TestEnvironment::_buffer, restoredHeader, data);
      ASSERT_EQ(header, restoredHeader);
      ASSERT_EQ(data, TestEnvironment::_source);
    }
    void TearDown() {
      TestEnvironment::reset();
    }
  };

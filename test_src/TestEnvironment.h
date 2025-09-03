/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <sstream>

#include <gtest/gtest.h>

#include "CryptoDefinitions.h"
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

  static CryptoSodiumPtr createServer(CryptoSodiumPtr cryptoC) {
    std::string_view signatureWithPubKeySign(std::bit_cast<const char*>(cryptoC->_signatureWithPubKeySign.data()),
					     cryptoC->_signatureWithPubKeySign.size());
    return std::make_shared<CryptoSodium>(cryptoC->_msgHash,
					  cryptoC->_encodedPubKeyAes,
					  signatureWithPubKeySign);
  }

  static CryptoPlPlPtr createServer(CryptoPlPlPtr cryptoC) {
    return std::make_shared<CryptoPlPl>(cryptoC->_msgHash,
					cryptoC->_encodedPubKeyAes,
					cryptoC->_signatureWithPubKeySign);
  }

  struct TestCompressEncrypt : testing::Test {
    template <typename CryptoType, typename COMPRESSORS>
    void testCompressEncrypt(COMPRESSORS compressor, bool doEncrypt) {
      // client
      auto cryptoC(std::make_shared<CryptoType>(utility::generateRawUUID()));
      // server
      auto cryptoS = createServer(cryptoC);
      cryptodefinitions::clientKeyExchange(cryptoC, cryptoS->_encodedPubKeyAes);
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
      std::string_view dataView =
	cryptodefinitions::compressEncrypt(TestEnvironment::_buffer, header, doEncrypt, std::weak_ptr(cryptoC), data);
      HEADER restoredHeader;
      data = dataView;
      ASSERT_EQ(cryptodefinitions::isEncrypted(data), doEncrypt);
      cryptodefinitions::decryptDecompress(TestEnvironment::_buffer, restoredHeader, std::weak_ptr(cryptoS), data);
      ASSERT_EQ(header, restoredHeader);
      ASSERT_EQ(data, TestEnvironment::_source);
    }
    void TearDown() {
      TestEnvironment::reset();
    }
  };

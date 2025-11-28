/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <sstream>

#include <gtest/gtest.h>

#include "EncryptorTemplates.h"
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

  static auto createServer(CryptoSodiumPtr cryptoC) {
    return std::make_shared<CryptoSodium>(cryptoC->_encodedPubKeyAes,
					  cryptoC->_signatureWithPubKeySign);
  }

  static auto createServer(CryptoPlPlPtr cryptoC) {
    return std::make_shared<CryptoPlPl>(cryptoC->_encodedPubKeyAes,
					cryptoC->_signatureWithPubKeySign);
  }

  struct TestCompressEncrypt : testing::Test {
    template <typename CryptoType, typename COMPRESSORS>
    void testCompressEncrypt(COMPRESSORS compressor,
			     bool doEncrypt,
			     [[maybe_unused]] const CryptoVariant& container) {
      // client
      auto cryptoC(std::make_shared<CryptoType>());
      // server
      auto cryptoS = createServer(cryptoC);
      cryptoC->clientKeyExchange(cryptoS->_encodedPubKeyAes);
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
	compressEncrypt(container, TestEnvironment::_buffer, header, data, doEncrypt);
      HEADER restoredHeader;
      data = dataView;
      ASSERT_EQ(CryptoBase::isEncrypted(data), doEncrypt);
      decryptDecompress(container, TestEnvironment::_buffer, restoredHeader, data);
      ASSERT_EQ(header, restoredHeader);
      ASSERT_EQ(data, TestEnvironment::_source);
    }
    void TearDown() {
      TestEnvironment::reset();
    }
  };

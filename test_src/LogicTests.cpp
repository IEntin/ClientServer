/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ClientOptions.h"
#include "FifoClient.h"
#include "Server.h"
#include "ServerOptions.h"
#include "TcpClient.h"
#include "TestEnvironment.h"

// for i in {1..10}; do ./testbin --gtest_filter=LogicTest.TCP_LZ4_NONE_3600000_ENCRYPT_NOTENCRYPT_ND;done
// for i in {1..10}; do ./testbin --gtest_filter=LogicTest.FIFO_LZ4_LZ4_3600000_NOTENCRYPT_NOTENCRYPT_D;done
// for i in {1..10}; do ./testbin --gtest_filter=LogicTest.FIFO_NONE_NONE_100000_ENCRYPT_ENCRYPT_D;done
// for i in {1..10}; do ./testbin --gtest_filter=LogicTest.TCP_LZ4_LZ4_3600000_ENCRYPT_ENCRYPT_D;done
// for i in {1..10}; do ./testbin --gtest_filter=LogicTest.TCP_SNAPPY_LZ4_3000000_ENCRYPT_ENCRYPT_D;done
// for i in {1..10}; do ./testbin --gtest_filter=LogicTest.TCP_LZ4_ZSTD_3000000_ENCRYPT_ENCRYPT_D;done
// for i in {1..10}; do ./testbin --gtest_filter=LogicTest.FIFO_LZ4_SNAPPY_10000_ENCRYPT_ENCRYPT_D;done
// for i in {1..10}; do ./testbin --gtest_filter=LogicTestAltFormat*;done
// gdb --args testbin --gtest_filter=LogicTest.TCP_LZ4_NONE_3600000_ENCRYPT_NOTENCRYPT_ND

struct LogicTest : testing::Test {
  void testLogicTcp(COMPRESSORS serverCompressor,
		    COMPRESSORS clientCompressor,
		    CRYPTO serverEncrypt,
		    CRYPTO clientEncrypt,
		    std::size_t bufferSize,
		    DIAGNOSTICS diagnostics) {
    // start server
    ServerOptions::_compressor = serverCompressor;
    ServerOptions::_encryption = serverEncrypt;
    ServerOptions::_policyEnum = POLICYENUM::NOSORTINPUT;
    ServerPtr server = std::make_shared<Server>();
    ASSERT_TRUE(server->start());
    // start client
    ClientOptions::_compressor = clientCompressor;
    ClientOptions::_encryption = clientEncrypt;
    ClientOptions::_bufferSize = bufferSize;
    ClientOptions::_diagnostics = diagnostics;
    tcp::TcpClient client;
    client.run();
    std::string_view calibratedOutput = diagnostics == DIAGNOSTICS::ENABLED ?
      TestEnvironment::_outputD : TestEnvironment::_outputND;
    ASSERT_EQ(TestEnvironment::_oss.str(), calibratedOutput);
    server->stop();
  }

  void testLogicFifo(COMPRESSORS serverCompressor,
		     COMPRESSORS clientCompressor,
		     CRYPTO serverEncrypt,
		     CRYPTO clientEncrypt,
		     std::size_t bufferSize,
		     DIAGNOSTICS diagnostics) {
    // start server
    ServerOptions::_compressor = serverCompressor;
    ServerOptions::_encryption = serverEncrypt;
    ServerOptions::_policyEnum = POLICYENUM::NOSORTINPUT;
    ServerPtr server = std::make_shared<Server>();
    ASSERT_TRUE(server->start());
    // start client
    ClientOptions::_compressor = clientCompressor;
    ClientOptions::_encryption = clientEncrypt;
    ClientOptions::_bufferSize = bufferSize;
    ClientOptions::_diagnostics = diagnostics;
    std::string_view calibratedOutput = diagnostics == DIAGNOSTICS::ENABLED?
      TestEnvironment::_outputD : TestEnvironment::_outputND;
    fifo::FifoClient client;
    client.run();
    ASSERT_EQ(TestEnvironment::_oss.str(), calibratedOutput);
    server->stop();
  }

  void TearDown() {
    TestEnvironment::reset();
  }
};

TEST_F(LogicTest, TCP_LZ4_LZ4_3600000_ENCRYPT_ENCRYPT_D) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, CRYPTO::CRYPTOPP, CRYPTO::CRYPTOPP, 3600000, DIAGNOSTICS::ENABLED);
}

TEST_F(LogicTest, TCP_NONE_NONE_3600000_NOTENCRYPT_ENCRYPT_D) {
  testLogicTcp(COMPRESSORS::NONE, COMPRESSORS::NONE, CRYPTO::NONE, CRYPTO::CRYPTOPP, 3600000, DIAGNOSTICS::ENABLED);
}

TEST_F(LogicTest, TCP_NONE_LZ4_3600000_NOTENCRYPT_NOTENCRYPT_D) {
  testLogicTcp(COMPRESSORS::NONE, COMPRESSORS::LZ4, CRYPTO::NONE, CRYPTO::NONE, 3600000, DIAGNOSTICS::ENABLED);
}

TEST_F(LogicTest, TCP_LZ4_NONE_3600000_ENCRYPT_NOTENCRYPT_ND) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::NONE, CRYPTO::CRYPTOPP, CRYPTO::NONE, 3600000, DIAGNOSTICS::NONE);
}

TEST_F(LogicTest, TCP_LZ4_LZ4_3000000_ENCRYPT_ENCRYPT_D) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, CRYPTO::CRYPTOPP, CRYPTO::CRYPTOPP, 3000000, DIAGNOSTICS::ENABLED);
}

TEST_F(LogicTest, TCP_LZ4_ZSTD_3000000_ENCRYPT_ENCRYPT_D) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::ZSTD, CRYPTO::CRYPTOPP, CRYPTO::CRYPTOPP, 3000000, DIAGNOSTICS::ENABLED);
}

TEST_F(LogicTest, TCP_LZ4_SNAPPY_3000000_ENCRYPT_ENCRYPT_D) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::SNAPPY, CRYPTO::CRYPTOPP, CRYPTO::CRYPTOPP, 3000000, DIAGNOSTICS::ENABLED);
}

TEST_F(LogicTest, TCP_SNAPPY_LZ4_3000000_ENCRYPT_ENCRYPT_D) {
  testLogicTcp(COMPRESSORS::SNAPPY, COMPRESSORS::LZ4, CRYPTO::CRYPTOPP, CRYPTO::CRYPTOPP, 3000000, DIAGNOSTICS::ENABLED);
}

TEST_F(LogicTest, TCP_SNAPPY_SNAPPY_3000000_ENCRYPT_ENCRYPT_D) {
  testLogicTcp(COMPRESSORS::SNAPPY, COMPRESSORS::SNAPPY, CRYPTO::CRYPTOPP, CRYPTO::CRYPTOPP, 3000000, DIAGNOSTICS::ENABLED);
}

TEST_F(LogicTest, TCP_LZ4_LZ4_20000_NOTENCRYPT__ENCRYPT_D) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, CRYPTO::NONE, CRYPTO::CRYPTOPP, 20000, DIAGNOSTICS::ENABLED);
}

TEST_F(LogicTest, TCP_LZ4_LZ4_55000_NOTENCRYPT_NOTENCRYPT_D) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, CRYPTO::NONE, CRYPTO::NONE, 55000, DIAGNOSTICS::ENABLED);
}

TEST_F(LogicTest, TCP_LZ4_LZ4_3600000_NOTENCRYPT_NOTENCRYPT_ND) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, CRYPTO::NONE, CRYPTO::NONE, 3600000, DIAGNOSTICS::NONE);
}

TEST_F(LogicTest, TCP_LZ4_NONE_3600000_ENCRYPT_NOTENCRYPT_D) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::NONE, CRYPTO::CRYPTOPP, CRYPTO::NONE, 3600000, DIAGNOSTICS::ENABLED);
}

TEST_F(LogicTest, FIFO_LZ4_LZ4_3600000_NOTENCRYPT_NOTENCRYPT_D) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, CRYPTO::NONE, CRYPTO::NONE, 3600000, DIAGNOSTICS::ENABLED);
}

TEST_F(LogicTest, FIFO_NONE_NONE_100000_ENCRYPT_ENCRYPT_D) {
  testLogicFifo(COMPRESSORS::NONE, COMPRESSORS::NONE, CRYPTO::CRYPTOPP, CRYPTO::CRYPTOPP, 100000, DIAGNOSTICS::ENABLED);
}

TEST_F(LogicTest, FIFO_LZ4_NONE_3600000_ENCRYPT_NOTENCRYPT_D) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::NONE, CRYPTO::CRYPTOPP, CRYPTO::NONE, 3600000, DIAGNOSTICS::ENABLED);
}

TEST_F(LogicTest, FIFO_NONE_LZ4_100000_NOTENCRYPT_ENCRYPT_ND) {
  testLogicFifo(COMPRESSORS::NONE, COMPRESSORS::LZ4, CRYPTO::NONE, CRYPTO::CRYPTOPP, 100000, DIAGNOSTICS::NONE);
}

TEST_F(LogicTest, FIFO_LZ4_LZ4_543_NOTENCRYPT_NOTENCRYPT_D) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, CRYPTO::NONE, CRYPTO::NONE, 543, DIAGNOSTICS::ENABLED);
}

TEST_F(LogicTest, FIFO_LZ4_LZ4_10000_ENCRYPT_ENCRYPT_ND) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, CRYPTO::CRYPTOPP, CRYPTO::CRYPTOPP,  10000, DIAGNOSTICS::NONE);
}

TEST_F(LogicTest, FIFO_SNAPPY_SNAPPY_10000_ENCRYPT_ENCRYPT_D) {
  testLogicFifo(COMPRESSORS::SNAPPY, COMPRESSORS::SNAPPY, CRYPTO::CRYPTOPP, CRYPTO::CRYPTOPP,  10000, DIAGNOSTICS::ENABLED);
}

TEST_F(LogicTest, FIFO_LZ4_SNAPPY_10000_ENCRYPT_ENCRYPT_D) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::SNAPPY, CRYPTO::CRYPTOPP, CRYPTO::CRYPTOPP,  10000, DIAGNOSTICS::ENABLED);
}

TEST_F(LogicTest, FIFO_SNAPPY_LZ4_10000_ENCRYPT_ENCRYPT_ND) {
  testLogicFifo(COMPRESSORS::SNAPPY, COMPRESSORS::LZ4, CRYPTO::CRYPTOPP, CRYPTO::CRYPTOPP,  10000, DIAGNOSTICS::NONE);
}

TEST_F(LogicTest, FIFO_SNAPPY_ZSTD_10000_ENCRYPT_ENCRYPT_D) {
  testLogicFifo(COMPRESSORS::SNAPPY, COMPRESSORS::ZSTD, CRYPTO::CRYPTOPP, CRYPTO::CRYPTOPP,  10000, DIAGNOSTICS::ENABLED);
}

TEST_F(LogicTest, FIFO_ZSTD_ZSTD_10000_ENCRYPT_ENCRYPT_D) {
  testLogicFifo(COMPRESSORS::ZSTD, COMPRESSORS::ZSTD, CRYPTO::CRYPTOPP, CRYPTO::CRYPTOPP,  10000, DIAGNOSTICS::ENABLED);
}

TEST_F(LogicTest, FIFO_LZ4_LZ4_10000000_ENCRYPT_NOTENCRYPT_D) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, CRYPTO::CRYPTOPP, CRYPTO::NONE, 10000000, DIAGNOSTICS::ENABLED);
}

TEST_F(LogicTest, FIFO_LZ4_LZ4_3600000_NOTENCRYPT_ENCRYPT_ND) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, CRYPTO::NONE, CRYPTO::CRYPTOPP, 3600000, DIAGNOSTICS::NONE);
}

TEST_F(LogicTest, FIFO_LZ4_NONE_3600000_NOTENCRYPT_NOTENCRYPT_ND) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::NONE, CRYPTO::NONE, CRYPTO::NONE, 3600000, DIAGNOSTICS::NONE);
}

struct LogicTestAltFormat : testing::Test {
  void testLogicAltFormat() {
    // start server
    ServerOptions::_policyEnum = POLICYENUM::NOSORTINPUT;
    ServerPtr server = std::make_shared<Server>();
    ASSERT_TRUE(server->start());
    // start client
    ClientOptions::_sourceName = "data/requestsDiffFormat.log";
    ClientOptions::_diagnostics = DIAGNOSTICS::ENABLED;
    {
      tcp::TcpClient client;
      client.run();
      std::string_view calibratedOutput = TestEnvironment::_outputAltFormatD;
      ASSERT_EQ(TestEnvironment::_oss.str(), calibratedOutput);
    }
    server->stop();
  }

  void TearDown() {
    TestEnvironment::reset();
  }
};

TEST_F(LogicTestAltFormat, Diagnostics) {
  testLogicAltFormat();
}

struct LogicTestSortInput : testing::Test {
  void testLogicSortInput(bool sort) {
    // start server
    ServerPtr server = ServerPtr();
    if (sort) {
      ServerOptions::_policyEnum = POLICYENUM::SORTINPUT;
      server = std::make_shared<Server>();
    }
    else {
      ServerOptions::_policyEnum = POLICYENUM::NOSORTINPUT;
      server = std::make_shared<Server>();
    }
    ASSERT_TRUE(server->start());
    // start client
    ClientOptions::_diagnostics = DIAGNOSTICS::ENABLED;
    tcp::TcpClient client;
    client.run();
    std::string_view calibratedOutput = TestEnvironment::_outputD;
    ASSERT_EQ(TestEnvironment::_oss.str(), calibratedOutput);
    server->stop();
  }

  void TearDown() {
    TestEnvironment::reset();
  }
};

TEST_F(LogicTestSortInput, Sort) {
  testLogicSortInput(true);
}

TEST_F(LogicTestSortInput, NoSort) {
  testLogicSortInput(false);
}

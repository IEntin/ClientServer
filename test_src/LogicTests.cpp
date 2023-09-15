/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ClientOptions.h"
#include "FifoClient.h"
#include "Logger.h"
#include "ServerOptions.h"
#include "Server.h"
#include "TcpClient.h"
#include "TestEnvironment.h"
#include "TransactionStrategy.h"

struct LogicTest : testing::Test {
  void testLogicTcp(COMPRESSORS serverCompressor,
		    COMPRESSORS clientCompressor,
		    bool serverEncrypt,
		    bool clientEncrypt,
		    size_t bufferSize,
		    bool diagnostics) {
    try {
      // start server
      ServerOptions::_compressor = serverCompressor;
      ServerOptions::_encrypted = serverEncrypt;
      StrategyPtr strategy = std::make_shared<TransactionStrategy>();
      Server server(strategy);
      ASSERT_TRUE(server.start());
      // start client
      ClientOptions::_compressor = clientCompressor;
      ClientOptions::_encrypted = clientEncrypt;
      ClientOptions::_bufferSize = bufferSize;
      ClientOptions::_diagnostics = diagnostics;
      {
	tcp::TcpClient client;
	client.run();
	std::string_view calibratedOutput = diagnostics ? TestEnvironment::_outputD : TestEnvironment::_outputND;
	ASSERT_EQ(TestEnvironment::_oss.str().size(), calibratedOutput.size());
	ASSERT_EQ(TestEnvironment::_oss.str(), calibratedOutput);
      }
      server.stop();
    }
    catch (const std::exception& e) {
      LogError << e.what() << '\n';
      ASSERT_TRUE(false);
    }
  }

  void testLogicFifo(COMPRESSORS serverCompressor,
		     COMPRESSORS clientCompressor,
		     bool serverEncrypt,
		     bool clientEncrypt,
		     size_t bufferSize,
		     bool diagnostics) {
    try {
      // start server
      ServerOptions::_compressor = serverCompressor;
      ServerOptions::_encrypted = serverEncrypt;
      StrategyPtr strategy = std::make_shared<TransactionStrategy>();
      Server server(strategy);
      ASSERT_TRUE(server.start());
      // start client
      ClientOptions::_compressor = clientCompressor;
      ClientOptions::_encrypted = clientEncrypt;
      ClientOptions::_bufferSize = bufferSize;
      ClientOptions::_diagnostics = diagnostics;
      std::string_view calibratedOutput = diagnostics ?
	TestEnvironment::_outputD : TestEnvironment::_outputND;
      {
	fifo::FifoClient client;
	client.run();
	ASSERT_EQ(TestEnvironment::_oss.str().size(), calibratedOutput.size());
	ASSERT_EQ(TestEnvironment::_oss.str(), calibratedOutput);
      }
      server.stop();
    }
    catch (const std::exception& e) {
      LogError << e.what() << '\n';
      ASSERT_TRUE(false);
    }
  }

  void TearDown() {
    TestEnvironment::reset();
  }
};

TEST_F(LogicTest, TCP_LZ4_LZ4_3600000_ENCRYPT_ENCRYPT_D) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, true, true, 3600000, true);
}

TEST_F(LogicTest, TCP_NONE_NONE_3600000_NOTENCRYPT_ENCRYPT_D) {
  testLogicTcp(COMPRESSORS::NONE, COMPRESSORS::NONE, false, true, 3600000, true);
}

TEST_F(LogicTest, TCP_NONE_LZ4_3600000_NOTENCRYPT_NOTENCRYPT_D) {
  testLogicTcp(COMPRESSORS::NONE, COMPRESSORS::LZ4, false, false, 3600000, true);
}

TEST_F(LogicTest, TCP_LZ4_NONE_3600000_ENCRYPT_NOTENCRYPT_ND) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::NONE, true, false, 3600000, false);
}

TEST_F(LogicTest, TCP_LZ4_LZ4_3000000_ENCRYPT_ENCRYPT_D) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, true, true, 3000000, true);
}

TEST_F(LogicTest, TCP_LZ4_LZ4_20000_NOTENCRYPT__ENCRYPT_D) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, false, true, 20000, true);
}

TEST_F(LogicTest, TCP_LZ4_LZ4_55000_NOTENCRYPT_NOTENCRYPT_D) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, false, false, 55000, true);
}

TEST_F(LogicTest, TCP_LZ4_LZ4_3600000_NOTENCRYPT_NOTENCRYPT_ND) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, false, false, 3600000, false);
}

TEST_F(LogicTest, TCP_LZ4_NONE_3600000_ENCRYPT_NOTENCRYPT_D) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::NONE, true, false, 3600000, true);
}


TEST_F(LogicTest, FIFO_LZ4_LZ4_3600000_NOTENCRYPT_NOTENCRYPT_D) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, false, false, 3600000, true);
}

TEST_F(LogicTest, FIFO_NONE_NONE_100000_ENCRYPT_ENCRYPT_D) {
  testLogicFifo(COMPRESSORS::NONE, COMPRESSORS::NONE, true, true, 100000, true);
}

TEST_F(LogicTest, FIFO_LZ4_NONE_3600000_ENCRYPT_NOTENCRYPT_D) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::NONE, true, false, 3600000, true);
}

TEST_F(LogicTest, FIFO_NONE_LZ4_100000_NOTENCRYPT_ENCRYPT_ND) {
  testLogicFifo(COMPRESSORS::NONE, COMPRESSORS::LZ4, false, true, 100000, false);
}

TEST_F(LogicTest, FIFO_LZ4_LZ4_543_NOTENCRYPT_NOTENCRYPT_D) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, false, false, 543, true);
}

TEST_F(LogicTest, FIFO_LZ4_LZ4_10000_ENCRYPT_ENCRYPT_ND) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, true, true,  10000, false);
}

TEST_F(LogicTest, FIFO_LZ4_LZ4_10000000_ENCRYPT_NOTENCRYPT_D) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, true, false, 10000000, true);
}

TEST_F(LogicTest, FIFO_LZ4_LZ4_3600000_NOTENCRYPT_ENCRYPT_ND) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, false, true, 3600000, false);
}

TEST_F(LogicTest, FIFO_LZ4_NONE_3600000_NOTENCRYPT_NOTENCRYPT_ND) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::NONE, false, false, 3600000, false);
}

struct LogicTestAltFormat : testing::Test {
  void testLogicAltFormat() {
    try {
      // start server
      StrategyPtr strategy = std::make_shared<TransactionStrategy>();
      Server server(strategy);
      ASSERT_TRUE(server.start());
      // start client
      ClientOptions::_sourceName = "data/requestsDiffFormat.log";
      ClientOptions::_diagnostics = true;
      {
	tcp::TcpClient client;
	client.run();
	std::string_view calibratedOutput = TestEnvironment::_outputAltFormatD;
	ASSERT_EQ(TestEnvironment::_oss.str().size(), calibratedOutput.size());
	ASSERT_EQ(TestEnvironment::_oss.str(), calibratedOutput);
      }
      server.stop();
    }
    catch (const std::exception& e) {
      LogError << e.what() << '\n';
      ASSERT_TRUE(false);
    }
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
    try {
      // start server
      ServerOptions::_sortInput = sort;
      StrategyPtr strategy = std::make_shared<TransactionStrategy>();
      Server server(strategy);
      ASSERT_TRUE(server.start());
      // start client
      ClientOptions::_diagnostics = true;
      {
	tcp::TcpClient client;
	client.run();
	std::string_view calibratedOutput = TestEnvironment::_outputD;
	ASSERT_EQ(TestEnvironment::_oss.str().size(), calibratedOutput.size());
	ASSERT_EQ(TestEnvironment::_oss.str(), calibratedOutput);
      }
      server.stop();
    }
    catch (const std::exception& e) {
      LogError << e.what() << '\n';
      ASSERT_TRUE(false);
    }
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

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
#include "Transaction.h"

struct LogicTest : testing::Test {
  void testLogicTcp(COMPRESSORS serverCompressor,
		    COMPRESSORS clientCompressor,
		    bool serverEncrypt,
		    bool clientEncrypt,
		    size_t serverBufferSize,
		    size_t clientBufferSize,
		    bool diagnostics = true) {
    try {
      // start server
      TestEnvironment::_serverOptions._processType = "Transaction";
      TestEnvironment::_serverOptions._compressor = serverCompressor;
      TestEnvironment::_serverOptions._encrypted = serverEncrypt;
      TestEnvironment::_serverOptions._bufferSize = serverBufferSize;
      Server server(TestEnvironment::_serverOptions);
      ASSERT_TRUE(server.start());
      // start client
      TestEnvironment::_clientOptions._compressor = clientCompressor;
      TestEnvironment::_clientOptions._encrypted = clientEncrypt;
      TestEnvironment::_clientOptions._bufferSize = clientBufferSize;
      TestEnvironment::_clientOptions._diagnostics = diagnostics;
      {
	tcp::TcpClient client(TestEnvironment::_clientOptions);
	client.run();
      }
      server.stop();
      std::string_view calibratedOutput = diagnostics ? TestEnvironment::_outputD : TestEnvironment::_outputND;
      ASSERT_EQ(TestEnvironment::_oss.str().size(), calibratedOutput.size());
      ASSERT_EQ(TestEnvironment::_oss.str(), calibratedOutput);
    }
    catch (const std::exception& e) {
      LogError << e.what() << std::endl;
      ASSERT_TRUE(false);
    }
  }

  void testLogicFifo(COMPRESSORS serverCompressor,
		     COMPRESSORS clientCompressor,
		     bool serverEncrypt,
		     bool clientEncrypt,
		     size_t serverBufferSize,
		     size_t clientBufferSize,
		     bool diagnostics = true) {
    try {
      // start server
      TestEnvironment::_serverOptions._processType = "Transaction";
      TestEnvironment::_serverOptions._compressor = serverCompressor;
      TestEnvironment::_serverOptions._encrypted = serverEncrypt;
      TestEnvironment::_serverOptions._bufferSize = serverBufferSize;
      Server server(TestEnvironment::_serverOptions);
      ASSERT_TRUE(server.start());
      // start client
      TestEnvironment::_clientOptions._compressor = clientCompressor;
      TestEnvironment::_clientOptions._encrypted = clientEncrypt;
      TestEnvironment::_clientOptions._bufferSize = clientBufferSize;
      TestEnvironment::_clientOptions._diagnostics = diagnostics;
      std::string_view calibratedOutput = diagnostics ?
	TestEnvironment::_outputD : TestEnvironment::_outputND;
      {
	fifo::FifoClient client(TestEnvironment::_clientOptions);
	client.run();
	ASSERT_EQ(TestEnvironment::_oss.str().size(), calibratedOutput.size());
	ASSERT_EQ(TestEnvironment::_oss.str(), calibratedOutput);
      }
      server.stop();
    }
    catch (const std::exception& e) {
      LogError << e.what() << std::endl;
      ASSERT_TRUE(false);
    }
  }

  void TearDown() {
    TestEnvironment::reset();
  }
};

TEST_F(LogicTest, TCP_LZ4_LZ4_100000_3600000_ENCRYPT_ENCRYPT_D) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, true, true, 100000, 3600000);
}

TEST_F(LogicTest, TCP_NONE_NONE_100000_3600000_NOTENCRYPT_ENCRYPT_D) {
  testLogicTcp(COMPRESSORS::NONE, COMPRESSORS::NONE, false, true, 100000, 3600000);
}

TEST_F(LogicTest, TCP_NONE_LZ4_100000_3600000_NOTENCRYPT_NOTENCRYPT_D) {
  testLogicTcp(COMPRESSORS::NONE, COMPRESSORS::LZ4, false, false, 100000, 3600000);
}

TEST_F(LogicTest, TCP_LZ4_NONE_100000_3600000_ENCRYPT_NOTENCRYPT_ND) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::NONE, true, false, 100000, 3600000, false);
}

TEST_F(LogicTest, TCP_LZ4_LZ4_3600000_3600000_ENCRYPT_ENCRYPT_D) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, true, true, 3600000, 3600000);
}

TEST_F(LogicTest, TCP_LZ4_LZ4_3600000_10000_NOTENCRYPT__ENCRYPT_D) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, false, true, 3600000, 10000);
}

TEST_F(LogicTest, TCP_LZ4_LZ4_25000_55000_NOTENCRYPT_NOTENCRYPT_D) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, false, false, 25000, 55000);
}

TEST_F(LogicTest, TCP_LZ4_LZ4_100000_3600000_NOTENCRYPT_NOTENCRYPT_ND) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, false, false, 100000, 3600000, false);
}

TEST_F(LogicTest, TCP_LZ4_NONE_100000_3600000_ENCRYPT_NOTENCRYPT_D) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::NONE, true, false, 100000, 3600000);
}


TEST_F(LogicTest, FIFO_LZ4_LZ4_100000_3600000_NOTENCRYPT_NOTENCRYPT_D) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, false, false, 100000, 3600000);
}

TEST_F(LogicTest, FIFO_NONE_NONE_100000_3600000_ENCRYPT_ENCRYPT_D) {
  testLogicFifo(COMPRESSORS::NONE, COMPRESSORS::NONE, true, true, 100000, 3600000);
}

TEST_F(LogicTest, FIFO_LZ4_NONE_100000_3600000_ENCRYPT_NOTENCRYPT_D) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::NONE, true, false, 100000, 3600000);
}

TEST_F(LogicTest, FIFO_NONE_LZ4_100000_3600000_NOTENCRYPT_ENCRYPT_ND) {
  testLogicFifo(COMPRESSORS::NONE, COMPRESSORS::LZ4, false, true, 100000, 3600000, false);
}

TEST_F(LogicTest, FIFO_LZ4_LZ4_12345_543_NOTENCRYPT_NOTENCRYPT_D) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, false, false, 12345, 543);
}

TEST_F(LogicTest, FIFO_LZ4_LZ4_500_10000_ENCRYPT_ENCRYPT_ND) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, true, true,  500, 10000, false);
}

TEST_F(LogicTest, FIFO_LZ4_LZ4_10000000_10000000_ENCRYPT_NOTENCRYPT_D) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, true, false, 10000000, 10000000);
}

TEST_F(LogicTest, FIFO_LZ4_LZ4_100000_3600000_NOTENCRYPT_ENCRYPT_ND) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, false, true, 100000, 3600000, false);
}

TEST_F(LogicTest, FIFO_LZ4_NONE_100000_3600000_NOTENCRYPT_NOTENCRYPT_ND) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::NONE, false, false, 100000, 3600000, false);
}

struct LogicTestAltFormat : testing::Test {
  void testLogicAltFormat() {
    try {
      // start server
      TestEnvironment::_serverOptions._processType = "Transaction";
      Server server(TestEnvironment::_serverOptions);
      ASSERT_TRUE(server.start());
      // start client
      TestEnvironment::_clientOptions._sourceName = "data/requestsDiffFormat.log";
      TestEnvironment::_clientOptions._diagnostics = true;
      {
	tcp::TcpClient client(TestEnvironment::_clientOptions);
	client.run();
      }
      server.stop();
      std::string_view calibratedOutput = TestEnvironment::_outputAltFormatD;
      ASSERT_EQ(TestEnvironment::_oss.str().size(), calibratedOutput.size());
      ASSERT_EQ(TestEnvironment::_oss.str(), calibratedOutput);
    }
    catch (const std::exception& e) {
      LogError << e.what() << std::endl;
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
      TestEnvironment::_serverOptions._processType = "Transaction";
      TestEnvironment::_serverOptions._sortInput = sort;
      Server server(TestEnvironment::_serverOptions);
      ASSERT_TRUE(server.start());
      // start client
      TestEnvironment::_clientOptions._diagnostics = true;
      {
	tcp::TcpClient client(TestEnvironment::_clientOptions);
	client.run();
      }
      server.stop();
      std::string_view calibratedOutput = TestEnvironment::_outputD;
      ASSERT_EQ(TestEnvironment::_oss.str().size(), calibratedOutput.size());
      ASSERT_EQ(TestEnvironment::_oss.str(), calibratedOutput);
    }
    catch (const std::exception& e) {
      LogError << e.what() << std::endl;
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

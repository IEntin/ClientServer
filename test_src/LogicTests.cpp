/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ClientOptions.h"
#include "FifoAcceptor.h"
#include "FifoClient.h"
#include "MemoryPool.h"
#include "ServerOptions.h"
#include "Task.h"
#include "TaskController.h"
#include "TcpClient.h"
#include "TcpHeartbeatClient.h"
#include "TcpServer.h"
#include "TestEnvironment.h"
#include "Transaction.h"
#include "Utility.h"
#include <gtest/gtest.h>

struct LogicTest : testing::Test {
  void testLogicTcp(COMPRESSORS serverCompressor,
		    COMPRESSORS clientCompressor,
		    size_t serverMemPoolSize,
		    size_t clientMemPoolSize,
		    bool diagnostics = true) {
    try {
      // start server
      TestEnvironment::_serverOptions._compressor = serverCompressor;
      TestEnvironment::_serverOptions._bufferSize = serverMemPoolSize;
      MemoryPool::setExpectedSize(serverMemPoolSize);
      RunnablePtr tcpServer =
	std::make_shared<tcp::TcpServer>(TestEnvironment::_serverOptions);
      bool serverStart = tcpServer->start();
      // start client
      TestEnvironment::_clientOptions._compressor = clientCompressor;
      TestEnvironment::_clientOptions._bufferSize = clientMemPoolSize;
      TestEnvironment::_clientOptions._diagnostics = diagnostics;
      tcp::TcpClient client(TestEnvironment::_clientOptions);
      client.run();
      tcpServer->stop();
      ASSERT_TRUE(serverStart);
      std::string_view calibratedOutput = diagnostics ? TestEnvironment::_outputD : TestEnvironment::_outputND;
      ASSERT_EQ(TestEnvironment::_oss.str().size(), calibratedOutput.size());
      ASSERT_EQ(TestEnvironment::_oss.str(), calibratedOutput);
    }
    catch (const std::exception& e) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << '\n';
    }
  }

  void testLogicFifo(COMPRESSORS serverCompressor,
		     COMPRESSORS clientCompressor,
		     size_t serverMemPoolSize,
		     size_t clientMemPoolSize,
		     bool diagnostics = true) {
    try {
      // start server
      TestEnvironment::_serverOptions._compressor = serverCompressor;
      TestEnvironment::_serverOptions._bufferSize = serverMemPoolSize;
      MemoryPool::setExpectedSize(serverMemPoolSize);
      RunnablePtr fifoAcceptor =
	std::make_shared<fifo::FifoAcceptor>(TestEnvironment::_serverOptions);
      bool serverStart = fifoAcceptor->start();
      // start client
      TestEnvironment::_clientOptions._compressor = clientCompressor;
      TestEnvironment::_clientOptions._bufferSize = clientMemPoolSize;
      TestEnvironment::_clientOptions._diagnostics = diagnostics;
      fifo::FifoClient client(TestEnvironment::_clientOptions);
      client.run();
      fifoAcceptor->stop();
      ASSERT_TRUE(serverStart);
      std::string_view calibratedOutput = diagnostics ? TestEnvironment::_outputD : TestEnvironment::_outputND;
      ASSERT_EQ(TestEnvironment::_oss.str().size(), calibratedOutput.size());
      ASSERT_EQ(TestEnvironment::_oss.str(), calibratedOutput);
    }
    catch (const std::exception& e) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << '\n';
    }
  }

  void TearDown() {
    TestEnvironment::reset();
  }

  static void SetUpTestSuite() {
    // To change options modify defaults in
    // ServerOptions.cpp and rebuild application
    Task::setPreprocessMethod(Transaction::normalizeSizeKey);
    Task::setProcessMethod(Transaction::processRequest);
  }

  static void TearDownTestSuite() {
    TestEnvironment::reset();
  }
};

TEST_F(LogicTest, TCP_LZ4_LZ4_100000_3600000_D) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, 100000, 3600000);
}

TEST_F(LogicTest, TCP_NONE_NONE_100000_3600000_D) {
  testLogicTcp(COMPRESSORS::NONE, COMPRESSORS::NONE, 100000, 3600000);
}

TEST_F(LogicTest, TCP_NONE_LZ4_100000_3600000_D) {
  testLogicTcp(COMPRESSORS::NONE, COMPRESSORS::LZ4, 100000, 3600000);
}

TEST_F(LogicTest, TCP_LZ4_NONE_100000_3600000_D) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::NONE, 100000, 3600000);
}

TEST_F(LogicTest, TCP_LZ4_LZ4_3600000_3600000_D) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, 3600000, 3600000);
}

TEST_F(LogicTest, TCP_LZ4_LZ4_3600000_10000_D) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, 3600000, 10000);
}

TEST_F(LogicTest, TCP_LZ4_LZ4_25000_55000_D) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, 25000, 55000);
}

TEST_F(LogicTest, TCP_LZ4_LZ4_100000_3600000_ND) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, 100000, 3600000, false);
}

TEST_F(LogicTest, TCP_LZ4_NONE_100000_3600000_ND) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::NONE, 100000, 3600000, false);
}


TEST_F(LogicTest, FIFO_LZ4_LZ4_100000_3600000_D) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, 100000, 3600000);
}

TEST_F(LogicTest, FIFO_NONE_NONE_100000_3600000_D) {
  testLogicFifo(COMPRESSORS::NONE, COMPRESSORS::NONE, 100000, 3600000);
}

TEST_F(LogicTest, FIFO_LZ4_NONE_100000_3600000_D) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::NONE, 100000, 3600000);
}

TEST_F(LogicTest, FIFO_NONE_LZ4_100000_3600000_D) {
  testLogicFifo(COMPRESSORS::NONE, COMPRESSORS::LZ4, 100000, 3600000);
}

TEST_F(LogicTest, FIFO_LZ4_LZ4_10000_500_D) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, 10000, 500);
}

TEST_F(LogicTest, FIFO_LZ4_LZ4_500_10000_D) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, 500, 10000);
}

TEST_F(LogicTest, FIFO_LZ4_LZ4_10000000_10000000_D) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, 10000000, 10000000);
}

TEST_F(LogicTest, FIFO_LZ4_LZ4_100000_3600000_ND) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, 100000, 3600000, false);
}

TEST_F(LogicTest, FIFO_LZ4_NONE_100000_3600000_ND) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::NONE, 100000, 3600000, false);
}

struct LogicTestAltFormat : testing::Test {
  void testLogicAltFormat() {
    try {
      // start server
      RunnablePtr tcpServer =
	std::make_shared<tcp::TcpServer>(TestEnvironment::_serverOptions);
      bool serverStart = tcpServer->start();
      // start client
      TestEnvironment::_clientOptions._sourceName = "data/requestsDiffFormat.log";
      TestEnvironment::_clientOptions._diagnostics = true;
      tcp::TcpClient client(TestEnvironment::_clientOptions);
      client.run();
      tcpServer->stop();
      ASSERT_TRUE(serverStart);
      std::string_view calibratedOutput = TestEnvironment::_outputAltFormatD;
      ASSERT_EQ(TestEnvironment::_oss.str().size(), calibratedOutput.size());
      ASSERT_EQ(TestEnvironment::_oss.str(), calibratedOutput);
    }
    catch (const std::exception& e) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << '\n';
    }
  }

  void TearDown() {
    TestEnvironment::reset();
  }

  static void SetUpTestSuite() {
    // To change options modify defaults in
    // ServerOptions.cpp and rebuild application
    Task::setPreprocessMethod(Transaction::normalizeSizeKey);
    Task::setProcessMethod(Transaction::processRequest);
  }
};

TEST_F(LogicTestAltFormat, Diagnostics) {
  testLogicAltFormat();
}

struct LogicTestSortInput : testing::Test {
  void testLogicSortInput(bool sort) {
    try {
      // start server
      TestEnvironment::_serverOptions._sortInput = sort;
      RunnablePtr tcpServer =
	std::make_shared<tcp::TcpServer>(TestEnvironment::_serverOptions);
      bool serverStart = tcpServer->start();
      // start client
      TestEnvironment::_clientOptions._diagnostics = true;
      tcp::TcpClient client(TestEnvironment::_clientOptions);
      client.run();
      tcpServer->stop();
      ASSERT_TRUE(serverStart);
      std::string_view calibratedOutput = TestEnvironment::_outputD;
      ASSERT_EQ(TestEnvironment::_oss.str().size(), calibratedOutput.size());
      ASSERT_EQ(TestEnvironment::_oss.str(), calibratedOutput);
    }
    catch (const std::exception& e) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << '\n';
    }
  }

  void TearDown() {
    TestEnvironment::reset();
  }

  static void SetUpTestSuite() {
    // To change options modify defaults in
    // ServerOptions.cpp and rebuild application
    Task::setPreprocessMethod(Transaction::normalizeSizeKey);
    Task::setProcessMethod(Transaction::processRequest);
  }
};

TEST_F(LogicTestSortInput, Sort) {
  testLogicSortInput(true);
}

TEST_F(LogicTestSortInput, NoSort) {
  testLogicSortInput(false);
}

struct LogicTestHeartbeat : testing::Test {
  void testLogicHeartbeat(bool btimeout = false) {
    try {
      // start server
      if (btimeout)
	TestEnvironment::_serverOptions._tcpHeartbeatTimeout = 0;
      RunnablePtr tcpServer =
	std::make_shared<tcp::TcpServer>(TestEnvironment::_serverOptions);
      bool serverStart = tcpServer->start();
      // start client
      TestEnvironment::_clientOptions._diagnostics = true;
      TestEnvironment::_clientOptions._tcpHeartbeatEnabled = true;
      tcp::TcpClient client(TestEnvironment::_clientOptions);
      client.run();
      tcpServer->stop();
      ASSERT_TRUE(serverStart);
      if (btimeout)
	 ASSERT_TRUE(tcp::TcpHeartbeatClient::_serverDown);
      else {
	std::string_view calibratedOutput = TestEnvironment::_outputD;
	ASSERT_EQ(TestEnvironment::_oss.str().size(), calibratedOutput.size());
	ASSERT_EQ(TestEnvironment::_oss.str(), calibratedOutput);
	ASSERT_FALSE(tcp::TcpHeartbeatClient::_serverDown);
      }
    }
    catch (const std::exception& e) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << '\n';
    }
  }

  void TearDown() {
    TestEnvironment::reset();
  }

  static void SetUpTestSuite() {
    // To change options modify defaults in
    // ServerOptions.cpp and rebuild application
    Task::setPreprocessMethod(Transaction::normalizeSizeKey);
    Task::setProcessMethod(Transaction::processRequest);
  }
};

TEST_F(LogicTestHeartbeat, Heartbeat) {
  testLogicHeartbeat();
}

TEST_F(LogicTestHeartbeat, Timeout) {
  testLogicHeartbeat(true);
}

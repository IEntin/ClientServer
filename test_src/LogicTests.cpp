/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Ad.h"
#include "ClientOptions.h"
#include "FifoClient.h"
#include "FifoServer.h"
#include "ServerOptions.h"
#include "Task.h"
#include "TaskController.h"
#include "TcpClient.h"
#include "TcpConnection.h"
#include "TcpServer.h"
#include "TestEnvironment.h"
#include "Transaction.h"
#include <gtest/gtest.h>

struct LogicTest : testing::Test {
  void testLogicTcp(COMPRESSORS serverCompressor,
		    COMPRESSORS clientCompressor,
		    size_t serverMemPoolSize,
		    size_t clientMemPoolSize,
		    bool diagnostics = true) {
    // start server
    TestEnvironment::_serverOptions._compressor = serverCompressor;
    TestEnvironment::_serverOptions._bufferSize = serverMemPoolSize;
    TestEnvironment::_taskController->setMemoryPoolSize(serverMemPoolSize);
    tcp::TcpServerPtr tcpServer =
      std::make_shared<tcp::TcpServer>(TestEnvironment::_taskController, TestEnvironment::_serverOptions);
    bool serverStart = tcpServer->start();
    // start client
    TestEnvironment::_tcpClientOptions._compressor = clientCompressor;
    TestEnvironment::_tcpClientOptions._bufferSize = clientMemPoolSize;
    TestEnvironment::_tcpClientOptions._diagnostics = diagnostics;
    tcp::TcpClient client(TestEnvironment::_tcpClientOptions);
    client.run();
    ASSERT_TRUE(serverStart);
    std::string calibratedOutput = diagnostics ? TestEnvironment::_outputD : TestEnvironment::_outputND;
    ASSERT_EQ(TestEnvironment::_oss.str().size(), calibratedOutput.size());
    ASSERT_EQ(TestEnvironment::_oss.str(), calibratedOutput);
    tcpServer->stop();
  }

  void testLogicFifo(COMPRESSORS serverCompressor,
		     COMPRESSORS clientCompressor,
		     size_t serverMemPoolSize,
		     size_t clientMemPoolSize,
		     bool diagnostics = true) {
    // start server
    TestEnvironment::_serverOptions._compressor = serverCompressor;
    TestEnvironment::_serverOptions._bufferSize = serverMemPoolSize;
    TestEnvironment::_taskController->setMemoryPoolSize(serverMemPoolSize);
    fifo::FifoServerPtr fifoServer =
      std::make_shared<fifo::FifoServer>(TestEnvironment::_taskController, TestEnvironment::_serverOptions);
    bool serverStart = fifoServer->start(TestEnvironment::_serverOptions);
    // start client
    TestEnvironment::_fifoClientOptions._compressor = clientCompressor;
    TestEnvironment::_fifoClientOptions._bufferSize = clientMemPoolSize;
    TestEnvironment::_fifoClientOptions._diagnostics = diagnostics;
    fifo::FifoClient client(TestEnvironment::_fifoClientOptions);
    client.run();
    ASSERT_TRUE(serverStart);
    std::string calibratedOutput = diagnostics ? TestEnvironment::_outputD : TestEnvironment::_outputND;
    ASSERT_EQ(TestEnvironment::_oss.str().size(), calibratedOutput.size());
    ASSERT_EQ(TestEnvironment::_oss.str(), calibratedOutput);
    fifoServer->stop();
  }

  void SetUp() {}

  void TearDown() {
    TestEnvironment::reset();
  }

  static void SetUpTestSuite() {
    // To change options modify defaults in
    // ServerOptions.cpp and rebuild application
    Ad::load(TestEnvironment::_serverOptions._adsFileName);
    Task::setPreprocessMethod(Transaction::normalizeSizeKey);
    Task::setProcessMethod(Transaction::processRequest);
  }
  static void TearDownTestSuite() {
    // set task controller to default state
    ServerOptions options;
    TestEnvironment::_taskController->setMemoryPoolSize(options._bufferSize);    
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

TEST(LogicTestAltFormat, Diagnostics) {
  // start server
  tcp::TcpServerPtr tcpServer =
    std::make_shared<tcp::TcpServer>(TestEnvironment::_taskController, TestEnvironment::_serverOptions);
  bool serverStart = tcpServer->start();
  // start client
  std::ostringstream oss;
  TcpClientOptions tcpClientOptions(&oss);
  tcpClientOptions._sourceName = "data/requestsDiffFormat.log";
  tcpClientOptions._diagnostics = true;
  tcp::TcpClient client(tcpClientOptions);
  client.run();
  ASSERT_TRUE(serverStart);
  std::string calibratedOutput = TestEnvironment::_outputAltFormatD;
  ASSERT_EQ(oss.str().size(), calibratedOutput.size());
  ASSERT_EQ(oss.str(), calibratedOutput);
  tcpServer->stop();
}

struct LogicTestSortInput : testing::Test {
  void testLogicSortInput(bool sort) {
    // start server
    ServerOptions serverOptions;
    serverOptions._sortInput = sort;
    tcp::TcpServerPtr tcpServer =
      std::make_shared<tcp::TcpServer>(TestEnvironment::_taskController, serverOptions);
    bool serverStart = tcpServer->start();
    // start client
    std::ostringstream oss;
    TcpClientOptions tcpClientOptions(&oss);
    tcpClientOptions._diagnostics = true;
    tcp::TcpClient client(tcpClientOptions);
    client.run();
    ASSERT_TRUE(serverStart);
    std::string calibratedOutput = TestEnvironment::_outputD;
    ASSERT_EQ(oss.str().size(), calibratedOutput.size());
    ASSERT_EQ(oss.str(), calibratedOutput);
    tcpServer->stop();
  }
};

TEST_F(LogicTestSortInput, Sort) {
  testLogicSortInput(true);
}

TEST_F(LogicTestSortInput, NoSort) {
  testLogicSortInput(false);
}

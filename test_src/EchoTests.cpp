/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Echo.h"
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
#include <gtest/gtest.h>
#include <filesystem>

struct EchoTest : testing::Test {
  void testEchoTcp(COMPRESSORS serverCompressor, COMPRESSORS clientCompressor) {
    // start server
    TestEnvironment::_serverOptions._compressor = serverCompressor;
    tcp::TcpServerPtr tcpServer =
      std::make_shared<tcp::TcpServer>(TestEnvironment::_taskController, TestEnvironment::_serverOptions);
    bool serverStart = tcpServer->start();
    // start client
    TestEnvironment::_tcpClientOptions._compressor = clientCompressor;
    tcp::TcpClient client(TestEnvironment::_tcpClientOptions);
    bool clientRun = client.run();
    ASSERT_TRUE(serverStart);
    ASSERT_TRUE(clientRun);
    ASSERT_EQ(TestEnvironment::_oss.str().size(), TestEnvironment::_source.size());
    ASSERT_EQ(TestEnvironment::_oss.str(), TestEnvironment::_source);
    tcpServer->stop();
  }

  void testEchoFifo(COMPRESSORS serverCompressor, COMPRESSORS clientCompressor) {
    // start server
    TestEnvironment::_serverOptions._compressor = serverCompressor;
    fifo::FifoServerPtr fifoServer =
      std::make_shared<fifo::FifoServer>(TestEnvironment::_taskController, TestEnvironment::_serverOptions);
    bool serverStart = fifoServer->start(TestEnvironment::_serverOptions);
    // start client
    TestEnvironment::_fifoClientOptions._compressor = clientCompressor;
    fifo::FifoClient client(TestEnvironment::_fifoClientOptions);
    client.run();
    ASSERT_TRUE(serverStart);
    ASSERT_EQ(TestEnvironment::_oss.str().size(), TestEnvironment::_source.size());
    ASSERT_EQ(TestEnvironment::_oss.str(), TestEnvironment::_source);
    fifoServer->stop();
  }

  void SetUp() {}

  void TearDown() {
    TestEnvironment::reset();
  }

  static void SetUpTestSuite() {
    Task::setProcessMethod(echo::Echo::processRequest);
  }
  static void TearDownTestSuite() {}
};

TEST_F(EchoTest, TCP_LZ4_LZ4) {
  testEchoTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4);
}

TEST_F(EchoTest, TCP_NONE_NONE) {
  testEchoTcp(COMPRESSORS::NONE, COMPRESSORS::NONE);
}

TEST_F(EchoTest, TCP_LZ4_NONE) {
  testEchoTcp(COMPRESSORS::LZ4, COMPRESSORS::NONE);
}

TEST_F(EchoTest, TCP_NONE_LZ4) {
  testEchoTcp(COMPRESSORS::NONE, COMPRESSORS::LZ4);
}

TEST_F(EchoTest, FIFO_LZ4_LZ4) {
  testEchoFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4);
}

TEST_F(EchoTest, FIFO_NONE_NONE) {
  testEchoFifo(COMPRESSORS::NONE, COMPRESSORS::NONE);
}

TEST_F(EchoTest, FIFO_LZ4_NONE) {
  testEchoFifo(COMPRESSORS::LZ4, COMPRESSORS::NONE);
}

TEST_F(EchoTest, FIFO_NONE_LZ4) {
  testEchoFifo(COMPRESSORS::NONE, COMPRESSORS::LZ4);
}

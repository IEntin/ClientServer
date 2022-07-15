/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Echo.h"
#include "ClientOptions.h"
#include "FifoAcceptor.h"
#include "FifoClient.h"
#include "ServerOptions.h"
#include "Task.h"
#include "TcpClient.h"
#include "TcpServer.h"
#include "TestEnvironment.h"
#include "Utility.h"
#include <gtest/gtest.h>
#include <filesystem>

struct EchoTest : testing::Test {
  void testEchoTcp(COMPRESSORS serverCompressor, COMPRESSORS clientCompressor) {
    try {
      // start server
      TestEnvironment::_serverOptions._compressor = serverCompressor;
      RunnablePtr tcpServer =
	std::make_shared<tcp::TcpServer>(TestEnvironment::_serverOptions);
      bool serverStart = tcpServer->start();
      // start client
      TestEnvironment::_clientOptions._compressor = clientCompressor;
      tcp::TcpClient client(TestEnvironment::_clientOptions);
      bool clientRun = client.run();
      tcpServer->stop();
      ASSERT_TRUE(serverStart);
      ASSERT_TRUE(clientRun);
      ASSERT_EQ(TestEnvironment::_oss.str().size(), TestEnvironment::_source.size());
      ASSERT_EQ(TestEnvironment::_oss.str(), TestEnvironment::_source);
    }
    catch (const std::exception& e) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	   << ':' << e.what();
    }
  }

  void testEchoFifo(COMPRESSORS serverCompressor, COMPRESSORS clientCompressor) {
    // start server
    TestEnvironment::_serverOptions._compressor = serverCompressor;
    fifo::FifoAcceptorPtr fifoAcceptor =
      std::make_shared<fifo::FifoAcceptor>(TestEnvironment::_serverOptions);
    bool serverStart = fifoAcceptor->start();
    // start client
    TestEnvironment::_clientOptions._compressor = clientCompressor;
    fifo::FifoClient client(TestEnvironment::_clientOptions);
    bool clientRun = client.run();
    fifoAcceptor->stop();
    ASSERT_TRUE(serverStart);
    ASSERT_TRUE(clientRun);
    ASSERT_EQ(TestEnvironment::_oss.str().size(), TestEnvironment::_source.size());
    ASSERT_EQ(TestEnvironment::_oss.str(), TestEnvironment::_source);
  }

  void TearDown() {
    TestEnvironment::reset();
  }

  static void SetUpTestSuite() {
    Task::setProcessMethod(echo::Echo::processRequest);
  }
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

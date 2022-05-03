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
    TestEnvironment::_serverOptions._compressor = TestEnvironment::_orgServerCompressor;
    TestEnvironment::_serverOptions._bufferSize = TestEnvironment::_orgServerBufferSize;
    TestEnvironment::_taskController->setMemoryPoolSize(TestEnvironment::_orgServerBufferSize);
    TestEnvironment::_oss.str("");
    TestEnvironment::_tcpClientOptions._compressor = TestEnvironment::_orgTcpClientCompressor;
    TestEnvironment::_tcpClientOptions._bufferSize = TestEnvironment::_orgTcpClientBufferSize;
    TestEnvironment::_tcpClientOptions._diagnostics = TestEnvironment::_orgTcpClientDiagnostics;
    TestEnvironment::_fifoClientOptions._compressor = TestEnvironment::_orgFifoClientCompressor;
    TestEnvironment::_fifoClientOptions._bufferSize = TestEnvironment::_orgFifoClientBufferSize;
    TestEnvironment::_fifoClientOptions._diagnostics = TestEnvironment::_orgFifoClientDiagnostics;
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

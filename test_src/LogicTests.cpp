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
  static TaskControllerPtr _taskController;

  void testLogicTcp(COMPRESSORS serverCompressor,
		    COMPRESSORS clientCompressor,
		    size_t serverMemPoolSize,
		    size_t clientMemPoolSize,
		    bool diagnostics = true) {
    // start server
    ServerOptions serverOptions;
    Ad::load(serverOptions._adsFileName);
    _taskController->setMemoryPoolSize(serverMemPoolSize);
    tcp::TcpServerPtr tcpServer =
      std::make_shared<tcp::TcpServer>(_taskController,
				       serverOptions._expectedTcpConnections,
				       serverOptions._tcpPort,
				       serverOptions._tcpTimeout,
				       serverCompressor);
    bool serverStart = tcpServer->start();
    // start client
    std::ostringstream oss;
    TcpClientOptions clientOptions(&oss);
    clientOptions._bufferSize = clientMemPoolSize;
    clientOptions._compressor = clientCompressor;
    clientOptions._diagnostics = diagnostics;
    tcp::TcpClient client(clientOptions);
    bool clientRun = client.run();
    ASSERT_TRUE(serverStart);
    ASSERT_TRUE(clientRun);
    std::string calibratedOutput = diagnostics ? TestEnvironment::_outputD : TestEnvironment::_outputND;
    ASSERT_EQ(oss.str().size(), calibratedOutput.size());
    ASSERT_EQ(oss.str(), calibratedOutput);
    tcpServer->stop();
  }

  void testLogicFifo(COMPRESSORS serverCompressor,
		     COMPRESSORS clientCompressor,
		     size_t serverMemPoolSize,
		     size_t clientMemPoolSize,
		     bool diagnostics = true) {
    // start server
    ServerOptions serverOptions;
    Ad::load(serverOptions._adsFileName);
    _taskController->setMemoryPoolSize(serverMemPoolSize);
    fifo::FifoServerPtr fifoServer =
      std::make_shared<fifo::FifoServer>(_taskController,
					 serverOptions._fifoDirectoryName,
					 serverOptions._fifoBaseNames,
					 serverCompressor);
    bool serverStart = fifoServer->start();
    // start client
    std::ostringstream oss;
    FifoClientOptions clientOptions(&oss);
    clientOptions._bufferSize = clientMemPoolSize;
    clientOptions._compressor = clientCompressor;
    clientOptions._diagnostics = diagnostics;
    fifo::FifoClient client(clientOptions);
    bool clientRun = client.run();
    ASSERT_TRUE(serverStart);
    ASSERT_TRUE(clientRun);
    std::string calibratedOutput = diagnostics ? TestEnvironment::_outputD : TestEnvironment::_outputND;
    ASSERT_EQ(oss.str().size(), calibratedOutput.size());
    ASSERT_EQ(oss.str(), calibratedOutput);
    fifoServer->stop();
  }

  static void SetUpTestSuite() {
    // To change options modify defaults in
    // ServerOptions.cpp and recompile the application
    ServerOptions serverOptions;
    Task::setPreprocessMethod(Transaction::normalizeSizeKey);
    Task::setProcessMethod(Transaction::processRequest);
    _taskController = TaskController::instance(std::thread::hardware_concurrency());
  }
  static void TearDownTestSuite() {}
};
TaskControllerPtr LogicTest::_taskController;

TEST_F(LogicTest, LogicTestTcp1) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, 100000, 3600000);
}

TEST_F(LogicTest, LogicTestTcp2) {
  testLogicTcp(COMPRESSORS::NONE, COMPRESSORS::NONE, 100000, 3600000);
}

TEST_F(LogicTest, LogicTestTcp3) {
  testLogicTcp(COMPRESSORS::NONE, COMPRESSORS::LZ4, 100000, 3600000);
}

TEST_F(LogicTest, LogicTestTcp4) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::NONE, 100000, 3600000);
}

TEST_F(LogicTest, LogicTestTcp5) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, 3600000, 3600000);
}

TEST_F(LogicTest, LogicTestTcp6) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, 3600000, 10000);
}

TEST_F(LogicTest, LogicTestTcp7) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, 25000, 55000);
}

TEST_F(LogicTest, LogicTestTcp8) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::LZ4, 100000, 3600000, false);
}

TEST_F(LogicTest, LogicTestTcp9) {
  testLogicTcp(COMPRESSORS::LZ4, COMPRESSORS::NONE, 100000, 3600000, false);
}


TEST_F(LogicTest, LogicTestFifo1) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, 100000, 3600000);
}

TEST_F(LogicTest, LogicTestFifo2) {
  testLogicFifo(COMPRESSORS::NONE, COMPRESSORS::NONE, 100000, 3600000);
}

TEST_F(LogicTest, LogicTestFifo3) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::NONE, 100000, 3600000);
}

TEST_F(LogicTest, LogicTestFifo4) {
  testLogicFifo(COMPRESSORS::NONE, COMPRESSORS::LZ4, 100000, 3600000);
}

TEST_F(LogicTest, LogicTestFifo5) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, 10000, 500);
}

TEST_F(LogicTest, LogicTestFifo6) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, 500, 10000);
}

TEST_F(LogicTest, LogicTestFifo7) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, 10000000, 10000000);
}

TEST_F(LogicTest, LogicTestFifo8) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, 100000, 3600000, true);
}

TEST_F(LogicTest, LogicTestFifo9) {
  testLogicFifo(COMPRESSORS::LZ4, COMPRESSORS::NONE, 100000, 3600000, true);
}

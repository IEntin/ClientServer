#include "ClientOptions.h"
#include "Compression.h"
#include "FifoClient.h"
#include "FifoServer.h"
#include "MemoryPool.h"
#include "ServerOptions.h"
#include "Task.h"
#include "TaskController.h"
#include "TcpClient.h"
#include "TcpConnection.h"
#include "TcpServer.h"
#include "Transaction.h"
#include <gtest/gtest.h>
#include <filesystem>

struct LogicTest : testing::Test {
  static std::string _input;
  static std::string _calibratedOutput;
  static TaskControllerPtr _taskController;

  void testLogicTcp(COMPRESSORS serverCompressor,
		    COMPRESSORS clientCompressor,
		    size_t serverMemPoolSize,
		    size_t clientMemPoolSize) {
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
    clientOptions._diagnostics = true;
    tcp::TcpClient client(clientOptions);
    bool clientRun = client.run();
    ASSERT_TRUE(serverStart);
    ASSERT_TRUE(clientRun);
    ASSERT_EQ(oss.str().size(), _calibratedOutput.size());
    ASSERT_EQ(oss.str(), _calibratedOutput);
    tcpServer->stop();
  }

  void testLogicFifo(COMPRESSORS serverCompressor,
		     COMPRESSORS clientCompressor,
		     size_t serverMemPoolSize,
		     size_t clientMemPoolSize) {
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
    clientOptions._diagnostics = true;
    fifo::FifoClient client(clientOptions);
    bool clientRun = client.run();
    ASSERT_TRUE(serverStart);
    ASSERT_TRUE(clientRun);
    ASSERT_EQ(oss.str().size(), _calibratedOutput.size());
    ASSERT_EQ(oss.str(), _calibratedOutput);
    fifoServer->stop();
  }

  static void SetUpTestSuite() {
    ClientOptions clientOptions;
    _input = Client::readFile(clientOptions._sourceName);
    _calibratedOutput = Client::readFile("data/outputD.txt");
    Task::setProcessMethod(Transaction::processRequest);
    _taskController = TaskController::instance(std::thread::hardware_concurrency());
  }
  static void TearDownTestSuite() {}
};
std::string LogicTest::_input;
std::string LogicTest::_calibratedOutput;
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

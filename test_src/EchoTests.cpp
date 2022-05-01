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
  static TaskControllerPtr _taskController;

  void testEchoTcp(COMPRESSORS serverCompressor, COMPRESSORS clientCompressor) {
    // start server
    std::ostringstream oss;
    TcpClientOptions options(&oss);
    options._compressor = clientCompressor;
    tcp::TcpServerPtr tcpServer =
      std::make_shared<tcp::TcpServer>(_taskController, 1, std::atoi(options._tcpPort.data()), 1, serverCompressor);
    bool serverStart = tcpServer->start();
    // start client
    tcp::TcpClient client(options);
    bool clientRun = client.run();
    ASSERT_TRUE(serverStart);
    ASSERT_TRUE(clientRun);
    ASSERT_EQ(oss.str().size(), TestEnvironment::_source.size());
    ASSERT_EQ(oss.str(), TestEnvironment::_source);
    tcpServer->stop();
  }

  void testEchoFifo(COMPRESSORS serverCompressor, COMPRESSORS clientCompressor) {
    // start server
    std::string fifoDirName = std::filesystem::current_path().string();
    fifo::FifoServerPtr fifoServer =
      std::make_shared<fifo::FifoServer>(_taskController, fifoDirName, std::string("client1"), serverCompressor);
    bool serverStart = fifoServer->start();
    // start client
    std::ostringstream oss;
    FifoClientOptions options(&oss);
    options._compressor = clientCompressor;
    fifo::FifoClient client(options);
    client.run();
    ASSERT_TRUE(serverStart);
    ASSERT_EQ(oss.str().size(), TestEnvironment::_source.size());
    ASSERT_EQ(oss.str(), TestEnvironment::_source);
    fifoServer->stop();
  }

  static void SetUpTestSuite() {
    ClientOptions clientOptions;
    Task::setProcessMethod(echo::Echo::processRequest);
    _taskController = TaskController::instance(std::thread::hardware_concurrency());
    ServerOptions serverOptions;
    _taskController->setMemoryPoolSize(serverOptions._bufferSize);
  }
  static void TearDownTestSuite() {}
};
TaskControllerPtr EchoTest::_taskController;

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

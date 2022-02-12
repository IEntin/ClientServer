#include "Echo.h"
#include "ClientOptions.h"
#include "CommUtility.h"
#include "Compression.h"
#include "FifoClient.h"
#include "FifoServer.h"
#include "TaskThread.h"
#include "TcpClient.h"
#include "TcpConnection.h"
#include "TcpServer.h"
#include <gtest/gtest.h>
#include <filesystem>

struct EchoTest : testing::Test {
  using TaskThreadPoolPtr = std::shared_ptr<TaskThreadPool>;
  static std::string _sourceContent;

  static void SetUpTestSuite() {}
  static void TearDownTestSuite() {}
};
std::string EchoTest::_sourceContent = commutility::readFileContent("requests.log");

TEST_F(EchoTest, EchoTestTcpCompression) {
  Compression::setCompressionEnabled(std::string(LZ4));
  // start server
  TaskThreadPoolPtr taskThreadPool =
    std::make_shared<TaskThreadPool>(std::thread::hardware_concurrency(),
				     echo::processRequest);
  taskThreadPool->start();
  TcpClientOptions options;
  tcp::TcpServer tcpServer(1, std::atoi(options._tcpPort.c_str()), 1);
  // start client
  Batch payload;
  commutility::createPayload("requests.log", payload);
  std::ostringstream oss;
  ASSERT_TRUE(tcp::run(payload, options, &oss));
  ASSERT_EQ(oss.str(), _sourceContent);
  tcpServer.stop();
  taskThreadPool->stop();
}

TEST_F(EchoTest, EchoTestTcpNoCompression) {
  Compression::setCompressionEnabled(std::string(NOP));
  // start server
  TaskThreadPoolPtr taskThreadPool =
    std::make_shared<TaskThreadPool>(std::thread::hardware_concurrency(),
				     echo::processRequest);
  taskThreadPool->start();
  TcpClientOptions options;
  tcp::TcpServer tcpServer(1, std::atoi(options._tcpPort.c_str()), 1);
  // start client
  Batch payload;
  commutility::createPayload("requests.log", payload);
  std::ostringstream oss;
  ASSERT_TRUE(tcp::run(payload, options, &oss));
  ASSERT_EQ(oss.str(), _sourceContent);
  tcpServer.stop();
  taskThreadPool->stop();
}

TEST_F(EchoTest, EchoTestFifoCompression) {
  Compression::setCompressionEnabled(std::string(LZ4));
  // start server
  TaskThreadPoolPtr taskThreadPool =
    std::make_shared<TaskThreadPool>(std::thread::hardware_concurrency(),
				     echo::processRequest);
  taskThreadPool->start();
  std::string fifoDirName = std::filesystem::current_path().string();
  fifo::FifoServer::startThreads(fifoDirName, std::string("client1"));
  // start client
  Batch payload;
  commutility::createPayload("requests.log", payload);
  FifoClientOptions options;
  std::ostringstream oss;
  ASSERT_TRUE(fifo::run(payload, options, &oss));
  ASSERT_EQ(oss.str(), _sourceContent);
  fifo::FifoServer::joinThreads();
  taskThreadPool->stop();
}

TEST_F(EchoTest, EchoTestFifoNoCompression) {
  Compression::setCompressionEnabled(std::string(NOP));
  // start server
  TaskThreadPoolPtr taskThreadPool =
    std::make_shared<TaskThreadPool>(std::thread::hardware_concurrency(),
				     echo::processRequest);
  taskThreadPool->start();
  std::string fifoDirName = std::filesystem::current_path().string();
  fifo::FifoServer::startThreads(fifoDirName, std::string("client1"));
  // start client
  Batch payload;
  commutility::createPayload("requests.log", payload);
  FifoClientOptions options;
  std::ostringstream oss;
  ASSERT_TRUE(fifo::run(payload, options, &oss));
  ASSERT_EQ(oss.str(), _sourceContent);
  fifo::FifoServer::joinThreads();
  taskThreadPool->stop();
}

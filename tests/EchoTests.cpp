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
  // start server
  TaskThreadPoolPtr taskThreadPool =
    std::make_shared<TaskThreadPool>(std::thread::hardware_concurrency(),
				     echo::processRequest);
  taskThreadPool->start();
  auto clientCompression = std::make_pair<COMPRESSORS, bool>(COMPRESSORS::LZ4, true);
  std::ostringstream oss;
  TcpClientOptions options(clientCompression, &oss);
  auto compression = std::make_pair<COMPRESSORS, bool>(COMPRESSORS::LZ4, true);
  tcp::TcpServer tcpServer(1, std::atoi(options._tcpPort.c_str()), 1, compression);
  // start client
  Batch payload;
  commutility::createPayload("requests.log", payload);
  ASSERT_TRUE(tcp::TcpClient::run(payload, options));
  ASSERT_EQ(oss.str(), _sourceContent);
  tcpServer.stop();
  taskThreadPool->stop();
}

TEST_F(EchoTest, EchoTestTcpNoCompression) {
  // start server
  TaskThreadPoolPtr taskThreadPool =
    std::make_shared<TaskThreadPool>(std::thread::hardware_concurrency(),
				     echo::processRequest);
  taskThreadPool->start();
  auto clientCompression = std::make_pair<COMPRESSORS, bool>(COMPRESSORS::NONE, false);
  std::ostringstream oss;
  auto compression = std::make_pair<COMPRESSORS, bool>(COMPRESSORS::NONE, false);
  TcpClientOptions options(clientCompression, &oss);
  tcp::TcpServer tcpServer(1, std::atoi(options._tcpPort.c_str()), 1, compression);
  // start client
  Batch payload;
  commutility::createPayload("requests.log", payload);
  ASSERT_TRUE(tcp::TcpClient::run(payload, options));
  ASSERT_EQ(oss.str(), _sourceContent);
  tcpServer.stop();
  taskThreadPool->stop();
}

TEST_F(EchoTest, EchoTestFifoCompression) {
  // start server
  TaskThreadPoolPtr taskThreadPool =
    std::make_shared<TaskThreadPool>(std::thread::hardware_concurrency(),
				     echo::processRequest);
  taskThreadPool->start();
  std::string fifoDirName = std::filesystem::current_path().string();
  auto compression = std::make_pair<COMPRESSORS, bool>(COMPRESSORS::LZ4, true);
  fifo::FifoServer::startThreads(fifoDirName, std::string("client1"), compression);
  // start client
  Batch payload;
  commutility::createPayload("requests.log", payload);
  auto clientCompression = std::make_pair<COMPRESSORS, bool>(COMPRESSORS::LZ4, true);
  std::ostringstream oss;
  FifoClientOptions options(clientCompression, &oss);
  ASSERT_TRUE(fifo::FifoClient::run(payload, options));
  ASSERT_EQ(oss.str(), _sourceContent);
  fifo::FifoServer::joinThreads();
  taskThreadPool->stop();
}

TEST_F(EchoTest, EchoTestFifoNoCompression) {
  // start server
  TaskThreadPoolPtr taskThreadPool =
    std::make_shared<TaskThreadPool>(std::thread::hardware_concurrency(),
				     echo::processRequest);
  taskThreadPool->start();
  std::string fifoDirName = std::filesystem::current_path().string();
  auto compression = std::make_pair<COMPRESSORS, bool>(COMPRESSORS::NONE, false);
  fifo::FifoServer::startThreads(fifoDirName, std::string("client1"), compression);
  // start client
  Batch payload;
  commutility::createPayload("requests.log", payload);
  auto clientCompression = std::make_pair<COMPRESSORS, bool>(COMPRESSORS::NONE, false);
  std::ostringstream oss;
  FifoClientOptions options(clientCompression, &oss);
  ASSERT_TRUE(fifo::FifoClient::run(payload, options));
  ASSERT_EQ(oss.str(), _sourceContent);
  fifo::FifoServer::joinThreads();
  taskThreadPool->stop();
}

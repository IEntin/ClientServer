#include "Echo.h"
#include "ClientOptions.h"
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
std::string EchoTest::_sourceContent = Client::readFileContent("requests.log");

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
  tcp::TcpServer::start(1, std::atoi(options._tcpPort.c_str()), 1, compression);
  // start client
  Batch payload;
  Client::createPayload("requests.log", payload);
  tcp::TcpClient client(options);
  ASSERT_TRUE(client.run(payload));
  ASSERT_EQ(oss.str(), _sourceContent);
  tcp::TcpServer::stop();
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
  tcp::TcpServer::start(1, std::atoi(options._tcpPort.c_str()), 1, compression);
  // start client
  Batch payload;
  Client::createPayload("requests.log", payload);
  tcp::TcpClient client(options);
  ASSERT_TRUE(client.run(payload));
  ASSERT_EQ(oss.str(), _sourceContent);
  tcp::TcpServer::stop();
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
  fifo::FifoServer::start(fifoDirName, std::string("client1"), compression);
  // start client
  Batch payload;
  Client::createPayload("requests.log", payload);
  auto clientCompression = std::make_pair<COMPRESSORS, bool>(COMPRESSORS::LZ4, true);
  std::ostringstream oss;
  FifoClientOptions options(clientCompression, &oss);
  fifo::FifoClient client(options);
  ASSERT_TRUE(client.run(payload));
  ASSERT_EQ(oss.str(), _sourceContent);
  fifo::FifoServer::stop();
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
  fifo::FifoServer::start(fifoDirName, std::string("client1"), compression);
  // start client
  Batch payload;
  Client::createPayload("requests.log", payload);
  auto clientCompression = std::make_pair<COMPRESSORS, bool>(COMPRESSORS::NONE, false);
  std::ostringstream oss;
  FifoClientOptions options(clientCompression, &oss);
  fifo::FifoClient client(options);
  ASSERT_TRUE(client.run(payload));
  ASSERT_EQ(oss.str(), _sourceContent);
  fifo::FifoServer::stop();
  taskThreadPool->stop();
}

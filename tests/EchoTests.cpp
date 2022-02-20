#include "Echo.h"
#include "ClientOptions.h"
#include "Compression.h"
#include "FifoClient.h"
#include "FifoServer.h"
#include "TaskController.h"
#include "TcpClient.h"
#include "TcpConnection.h"
#include "TcpServer.h"
#include <gtest/gtest.h>
#include <filesystem>

struct EchoTest : testing::Test {
  static std::string _sourceContent;

  static void SetUpTestSuite() {}
  static void TearDownTestSuite() {}
};
std::string EchoTest::_sourceContent = Client::readFileContent("requests.log");

TEST_F(EchoTest, EchoTestTcpCompression) {
  // start server
  TaskController::start(std::thread::hardware_concurrency(), echo::processRequest);
  std::ostringstream oss;
  TcpClientOptions options(&oss);
  auto compression = std::make_pair<COMPRESSORS, bool>(COMPRESSORS::LZ4, true);
  options._compression = compression;
  tcp::TcpServer::start(1, std::atoi(options._tcpPort.c_str()), 1, compression);
  // start client
  Batch payload;
  Client::createPayload("requests.log", payload);
  tcp::TcpClient client(options);
  ASSERT_TRUE(client.run(payload));
  ASSERT_EQ(oss.str(), _sourceContent);
  tcp::TcpServer::stop();
  TaskController::stop();
}

TEST_F(EchoTest, EchoTestTcpNoCompression) {
  // start server
  TaskController::start(std::thread::hardware_concurrency(), echo::processRequest);
  std::ostringstream oss;
  TcpClientOptions options(&oss);
  auto compression =  std::make_pair<COMPRESSORS, bool>(COMPRESSORS::NONE, false);
  options._compression = compression;
  tcp::TcpServer::start(1, std::atoi(options._tcpPort.c_str()), 1, compression);
  // start client
  Batch payload;
  Client::createPayload("requests.log", payload);
  tcp::TcpClient client(options);
  ASSERT_TRUE(client.run(payload));
  ASSERT_EQ(oss.str(), _sourceContent);
  tcp::TcpServer::stop();
  TaskController::stop();
}

TEST_F(EchoTest, EchoTestFifoCompression) {
  // start server
  TaskController::start(std::thread::hardware_concurrency(), echo::processRequest);
  std::string fifoDirName = std::filesystem::current_path().string();
  auto compression = std::make_pair<COMPRESSORS, bool>(COMPRESSORS::LZ4, true);
  fifo::FifoServer::start(fifoDirName, std::string("client1"), compression);
  // start client
  Batch payload;
  Client::createPayload("requests.log", payload);
  std::ostringstream oss;
  FifoClientOptions options(&oss);
  options._compression = compression;
  fifo::FifoClient client(options);
  ASSERT_TRUE(client.run(payload));
  ASSERT_EQ(oss.str(), _sourceContent);
  fifo::FifoServer::stop();
  TaskController::stop();
}

TEST_F(EchoTest, EchoTestFifoNoCompression) {
  // start server
  TaskController::start(std::thread::hardware_concurrency(), echo::processRequest);
  std::string fifoDirName = std::filesystem::current_path().string();
  auto compression = std::make_pair<COMPRESSORS, bool>(COMPRESSORS::NONE, false);
  fifo::FifoServer::start(fifoDirName, std::string("client1"), compression);
  // start client
  Batch payload;
  Client::createPayload("requests.log", payload);
  std::ostringstream oss;
  FifoClientOptions options(&oss);
  options._compression = compression;
  fifo::FifoClient client(options);
  ASSERT_TRUE(client.run(payload));
  ASSERT_EQ(oss.str(), _sourceContent);
  fifo::FifoServer::stop();
  TaskController::stop();
}

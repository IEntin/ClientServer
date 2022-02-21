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
  static CompressionDescription _compressionY;
  static CompressionDescription _compressionN;
  static std::string _sourceContent;
  static Batch _payload;

  static void SetUpTestSuite() {
    Client::createPayload("requests.log", _payload);
  }
  static void TearDownTestSuite() {}
};
std::string EchoTest::_sourceContent = Client::readFileContent("requests.log");
CompressionDescription EchoTest::_compressionY =
  std::make_pair<COMPRESSORS, bool>(COMPRESSORS::LZ4, true);
CompressionDescription EchoTest::_compressionN =
  std::make_pair<COMPRESSORS, bool>(COMPRESSORS::NONE, false);
Batch EchoTest::_payload;

TEST_F(EchoTest, EchoTestTcpCompression) {
  // start server
  TaskController::start(std::thread::hardware_concurrency(), echo::processRequest);
  std::ostringstream oss;
  TcpClientOptions options(&oss);
  options._compression = _compressionY;
  tcp::TcpServer::start(1, std::atoi(options._tcpPort.c_str()), 1, _compressionY);
  // start client
  tcp::TcpClient client(options);
  ASSERT_TRUE(client.run(_payload));
  ASSERT_EQ(oss.str(), _sourceContent);
  tcp::TcpServer::stop();
  TaskController::stop();
}

TEST_F(EchoTest, EchoTestTcpNoCompression) {
  // start server
  TaskController::start(std::thread::hardware_concurrency(), echo::processRequest);
  std::ostringstream oss;
  TcpClientOptions options(&oss);
  options._compression = _compressionN;
  tcp::TcpServer::start(1, std::atoi(options._tcpPort.c_str()), 1, _compressionN);
  // start client
  tcp::TcpClient client(options);
  ASSERT_TRUE(client.run(_payload));
  ASSERT_EQ(oss.str(), _sourceContent);
  tcp::TcpServer::stop();
  TaskController::stop();
}

TEST_F(EchoTest, EchoTestTcpServerCompressionClientNoCompression) {
  // start server
  TaskController::start(std::thread::hardware_concurrency(), echo::processRequest);
  std::ostringstream oss;
  TcpClientOptions options(&oss);
  options._compression = _compressionN;
  tcp::TcpServer::start(1, std::atoi(options._tcpPort.c_str()), 1, _compressionY);
  // start client
  tcp::TcpClient client(options);
  ASSERT_TRUE(client.run(_payload));
  ASSERT_EQ(oss.str(), _sourceContent);
  tcp::TcpServer::stop();
  TaskController::stop();
}

TEST_F(EchoTest, EchoTestTcpServerNoCompressionClientCompression) {
  // start server
  TaskController::start(std::thread::hardware_concurrency(), echo::processRequest);
  std::ostringstream oss;
  TcpClientOptions options(&oss);
  options._compression = _compressionY;
  tcp::TcpServer::start(1, std::atoi(options._tcpPort.c_str()), 1, _compressionN);
  // start client
  tcp::TcpClient client(options);
  ASSERT_TRUE(client.run(_payload));
  ASSERT_EQ(oss.str(), _sourceContent);
  tcp::TcpServer::stop();
  TaskController::stop();
}

TEST_F(EchoTest, EchoTestFifoCompression) {
  // start server
  TaskController::start(std::thread::hardware_concurrency(), echo::processRequest);
  std::string fifoDirName = std::filesystem::current_path().string();
  auto compression = _compressionY;
  fifo::FifoServer::start(fifoDirName, std::string("client1"), _compressionY);
  // start client
  std::ostringstream oss;
  FifoClientOptions options(&oss);
  options._compression = compression;
  fifo::FifoClient client(options);
  ASSERT_TRUE(client.run(_payload));
  ASSERT_EQ(oss.str(), _sourceContent);
  fifo::FifoServer::stop();
  TaskController::stop();
}

TEST_F(EchoTest, EchoTestFifoNoCompression) {
  // start server
  TaskController::start(std::thread::hardware_concurrency(), echo::processRequest);
  std::string fifoDirName = std::filesystem::current_path().string();
  fifo::FifoServer::start(fifoDirName, std::string("client1"), _compressionN);
  // start client
  std::ostringstream oss;
  FifoClientOptions options(&oss);
  options._compression = _compressionN;
  fifo::FifoClient client(options);
  ASSERT_TRUE(client.run(_payload));
  ASSERT_EQ(oss.str(), _sourceContent);
  fifo::FifoServer::stop();
  TaskController::stop();
}

TEST_F(EchoTest, EchoTestFifoServerCompressionClientNoCompression) {
  // start server
  TaskController::start(std::thread::hardware_concurrency(), echo::processRequest);
  std::string fifoDirName = std::filesystem::current_path().string();
  fifo::FifoServer::start(fifoDirName, std::string("client1"), _compressionY);
  // start client
  std::ostringstream oss;
  FifoClientOptions options(&oss);
  options._compression = _compressionN;
  fifo::FifoClient client(options);
  ASSERT_TRUE(client.run(_payload));
  ASSERT_EQ(oss.str(), _sourceContent);
  fifo::FifoServer::stop();
  TaskController::stop();
}

TEST_F(EchoTest, EchoTestFifoServerNoCompressionClientCompression) {
  // start server
  TaskController::start(std::thread::hardware_concurrency(), echo::processRequest);
  std::string fifoDirName = std::filesystem::current_path().string();
  fifo::FifoServer::start(fifoDirName, std::string("client1"), _compressionN);
  // start client
  std::ostringstream oss;
  FifoClientOptions options(&oss);
  options._compression = _compressionY;
  fifo::FifoClient client(options);
  ASSERT_TRUE(client.run(_payload));
  ASSERT_EQ(oss.str(), _sourceContent);
  fifo::FifoServer::stop();
  TaskController::stop();
}

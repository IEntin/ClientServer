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
  using ProcessRequest = std::string (*)(std::string_view);
  static ProcessRequest _processRequest;
  static unsigned _numberWorkThreads;
  static std::shared_ptr<TaskThreadPool> _taskThreadPool;
  static std::string _sourceContent;

  static void SetUpTestSuite() {
    _taskThreadPool->start();
  }
  static void TearDownTestSuite() {
    _taskThreadPool->stop();
  }
};
ProcessRequest EchoTest::_processRequest = echo::processRequest;
unsigned EchoTest::_numberWorkThreads = std::thread::hardware_concurrency();
std::shared_ptr<TaskThreadPool> EchoTest::_taskThreadPool =
  std::make_shared<TaskThreadPool>(_numberWorkThreads, _processRequest);
std::string EchoTest::_sourceContent = commutility::readFileContent("requests.log");

TEST_F(EchoTest, EchoTestTcpCompression) {
  Compression::setCompressionEnabled(std::string(LZ4));
  TcpClientOptions options;
  // start server
  tcp::TcpServer tcpServer(1, std::atoi(options._tcpPort.c_str()), 1);
  // start client
  Batch payload;
  commutility::createPayload("requests.log", payload);
  std::ostringstream oss;
  ASSERT_TRUE(tcp::run(payload, options, &oss));
  ASSERT_EQ(oss.str(), _sourceContent);
  tcpServer.stop();
}

TEST_F(EchoTest, EchoTestTcpNoCompression) {
  Compression::setCompressionEnabled(std::string(NOP));
  TcpClientOptions options;
  // start server
  tcp::TcpServer tcpServer(1, std::atoi(options._tcpPort.c_str()), 1);
  // start client
  Batch payload;
  commutility::createPayload("requests.log", payload);
  std::ostringstream oss;
  ASSERT_TRUE(tcp::run(payload, options, &oss));
  ASSERT_EQ(oss.str(), _sourceContent);
  tcpServer.stop();
}

TEST_F(EchoTest, EchoTestTcpServerCompressionClientNoCompression) {
  Compression::setCompressionEnabled(std::string(NOP));
  Batch payload;
  commutility::createPayload("requests.log", payload);
  Compression::setCompressionEnabled(std::string(LZ4));
  TcpClientOptions options;
  // start server
  tcp::TcpServer tcpServer(1, std::atoi(options._tcpPort.c_str()), 1);
  // start client
  std::ostringstream oss;
  ASSERT_TRUE(tcp::run(payload, options, &oss));
  ASSERT_EQ(oss.str(), _sourceContent);
  tcpServer.stop();
}

TEST_F(EchoTest, EchoTestTcpServerNoCompressionClientCompression) {
  Compression::setCompressionEnabled(std::string(LZ4));
  Batch payload;
  commutility::createPayload("requests.log", payload);
  Compression::setCompressionEnabled(std::string(NOP));
  TcpClientOptions options;
  // start server
  tcp::TcpServer tcpServer(1, std::atoi(options._tcpPort.c_str()), 1);
  // start client
  std::ostringstream oss;
  ASSERT_TRUE(tcp::run(payload, options, &oss));
  ASSERT_EQ(oss.str(), _sourceContent);
  tcpServer.stop();
}

TEST_F(EchoTest, EchoTestFifoCompression) {
  Compression::setCompressionEnabled(std::string(LZ4));
  // start server
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
}

TEST_F(EchoTest, EchoTestFifoNoCompression) {
  Compression::setCompressionEnabled(std::string(NOP));
  // start server
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
}

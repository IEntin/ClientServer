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
  static CompressionType _compressionY;
  static CompressionType _compressionN;
  static std::string _input;
  static Batch _payload;

  void testEchoTcp(CompressionType serverCompression, CompressionType clientCompression) {
    // start server
    TaskController::start(std::thread::hardware_concurrency(), echo::processRequest);
    std::ostringstream oss;
    TcpClientOptions options(&oss);
    options._compression = clientCompression;
    tcp::TcpServer::start(1, std::atoi(options._tcpPort.c_str()), 1, serverCompression);
    // start client
    tcp::TcpClient client(options);
    ASSERT_TRUE(client.run(_payload));
    ASSERT_EQ(oss.str(), _input);
    tcp::TcpServer::stop();
    TaskController::stop();
  }
  void testEchoFifo(CompressionType serverCompression, CompressionType clientCompression) {
    // start server
    TaskController::start(std::thread::hardware_concurrency(), echo::processRequest);
    std::string fifoDirName = std::filesystem::current_path().string();
    fifo::FifoServer::start(fifoDirName, std::string("client1"), serverCompression);
    // start client
    std::ostringstream oss;
    FifoClientOptions options(&oss);
    options._compression = clientCompression;
    fifo::FifoClient client(options);
    ASSERT_TRUE(client.run(_payload));
    ASSERT_EQ(oss.str(), _input);
    fifo::FifoServer::stop();
    TaskController::stop();
  }

  static void SetUpTestSuite() {
    Client::createPayload("requests.log", _payload);
  }
  static void TearDownTestSuite() {}
};
std::string EchoTest::_input = Client::readFileContent("requests.log");
CompressionType EchoTest::_compressionY =
  std::make_pair<COMPRESSORS, bool>(COMPRESSORS::LZ4, true);
CompressionType EchoTest::_compressionN =
  std::make_pair<COMPRESSORS, bool>(COMPRESSORS::NONE, false);
Batch EchoTest::_payload;

TEST_F(EchoTest, EchoTestTcpCompression) {
  testEchoTcp(_compressionY, _compressionY);
}

TEST_F(EchoTest, EchoTestTcpNoCompression) {
  testEchoTcp(_compressionN, _compressionN);
}

TEST_F(EchoTest, EchoTestTcpServerCompressionClientNoCompression) {
  testEchoTcp(_compressionY, _compressionN);
}

TEST_F(EchoTest, EchoTestTcpServerNoCompressionClientCompression) {
  testEchoTcp(_compressionN, _compressionY);
}

TEST_F(EchoTest, EchoTestFifoCompression) {
  testEchoFifo(_compressionY, _compressionY);
}

TEST_F(EchoTest, EchoTestFifoNoCompression) {
  testEchoFifo(_compressionN, _compressionN);
}

TEST_F(EchoTest, EchoTestFifoServerCompressionClientNoCompression) {
  testEchoFifo(_compressionY, _compressionN);
}

TEST_F(EchoTest, EchoTestFifoServerNoCompressionClientCompression) {
  testEchoFifo(_compressionN, _compressionY);
}

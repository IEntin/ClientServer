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
  static COMPRESSORS _compressorY;
  static COMPRESSORS _compressorN;
  static std::string _input;
  static TaskControllerPtr _taskController;

  void testEchoTcp(COMPRESSORS serverCompressor, COMPRESSORS clientCompressor) {
    // start server
    std::ostringstream oss;
    TcpClientOptions options(&oss);
    options._compressor = clientCompressor;
    tcp::TcpServerPtr tcpServer =
      std::make_shared<tcp::TcpServer>(_taskController, 1, std::atoi(options._tcpPort.c_str()), 1, serverCompressor);
    bool serverStart = tcpServer->start();
    // start client
    tcp::TcpClient client(options);
    bool clientRun = client.run();
    tcpServer->stop();
    ASSERT_TRUE(serverStart);
    ASSERT_TRUE(clientRun);
    ASSERT_EQ(oss.str().size(), _input.size());
    ASSERT_EQ(oss.str(), _input);
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
    bool clientRun = client.run();
    ASSERT_TRUE(serverStart);
    ASSERT_TRUE(clientRun);
    ASSERT_EQ(oss.str().size(), _input.size());
    ASSERT_EQ(oss.str(), _input);
    fifoServer->stop();
  }

  static void SetUpTestSuite() {
    _taskController =
      TaskController::instance(std::thread::hardware_concurrency(), echo::processRequest);
  }
  static void TearDownTestSuite() {}
};
std::string EchoTest::_input = Client::readFile("requests.log");
COMPRESSORS EchoTest::_compressorY = COMPRESSORS::LZ4;
COMPRESSORS EchoTest::_compressorN = COMPRESSORS::NONE;
TaskControllerPtr EchoTest::_taskController;

TEST_F(EchoTest, EchoTestTcpCompression) {
  testEchoTcp(_compressorY, _compressorY);
}

TEST_F(EchoTest, EchoTestTcpNoCompression) {
  testEchoTcp(_compressorN, _compressorN);
}

TEST_F(EchoTest, EchoTestTcpServerCompressionClientNoCompression) {
  testEchoTcp(_compressorY, _compressorN);
}

TEST_F(EchoTest, EchoTestTcpServerNoCompressionClientCompression) {
  testEchoTcp(_compressorN, _compressorY);
}

TEST_F(EchoTest, EchoTestFifoCompression) {
  testEchoFifo(_compressorY, _compressorY);
}

TEST_F(EchoTest, EchoTestFifoNoCompression) {
  testEchoFifo(_compressorN, _compressorN);
}

TEST_F(EchoTest, EchoTestFifoServerCompressionClientNoCompression) {
  testEchoFifo(_compressorY, _compressorN);
}

TEST_F(EchoTest, EchoTestFifoServerNoCompressionClientCompression) {
  testEchoFifo(_compressorN, _compressorY);
}

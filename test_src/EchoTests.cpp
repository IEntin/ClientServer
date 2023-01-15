/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Echo.h"
#include "ClientOptions.h"
#include "FifoClient.h"
#include "Logger.h"
#include "ServerOptions.h"
#include "ServerManager.h"
#include "TcpClient.h"
#include "TestEnvironment.h"

struct EchoTest : testing::Test {
  void testEchoTcp(COMPRESSORS serverCompressor, COMPRESSORS clientCompressor) {
    try {
      // start server
      TestEnvironment::_serverOptions._processType = "Echo";
      TestEnvironment::_serverOptions._compressor = serverCompressor;
      ServerManager serverManager(TestEnvironment::_serverOptions);
      ASSERT_TRUE(serverManager.start());
      // start client
      TestEnvironment::_clientOptions._compressor = clientCompressor;
      tcp::TcpClient client(TestEnvironment::_clientOptions);
      ASSERT_TRUE(client.run());
      serverManager.stop();
      ASSERT_EQ(TestEnvironment::_oss.str().size(), TestEnvironment::_source.size());
      ASSERT_EQ(TestEnvironment::_oss.str(), TestEnvironment::_source);
    }
    catch (const std::exception& e) {
      LogError << e.what() << std::endl;
    }
  }

  void testEchoFifo(COMPRESSORS serverCompressor, COMPRESSORS clientCompressor) {
    try {
      // start server
      TestEnvironment::_serverOptions._processType = "Echo";
      TestEnvironment::_serverOptions._compressor = serverCompressor;
      ServerManager serverManager(TestEnvironment::_serverOptions);
      ASSERT_TRUE(serverManager.start());
      // start client
      TestEnvironment::_clientOptions._compressor = clientCompressor;
      fifo::FifoClient client(TestEnvironment::_clientOptions);
      ASSERT_TRUE(client.run());
      serverManager.stop();
      ASSERT_EQ(TestEnvironment::_oss.str().size(), TestEnvironment::_source.size());
      ASSERT_EQ(TestEnvironment::_oss.str(), TestEnvironment::_source);
    }
    catch (const std::exception& e) {
      LogError << e.what() << std::endl;
    }
  }

  void TearDown() {
    TestEnvironment::reset();
  }
};

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

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ClientOptions.h"
#include "FifoClient.h"
#include "Server.h"
#include "ServerOptions.h"
#include "TcpClient.h"
#include "TestEnvironment.h"

// for i in {1..10}; do ./testbin --gtest_filter=LogicTest2.TCP_LZ4_ZSTD_3600000_2ENCRYPT_2ENCRYPT_D;done
// for i in {1..10}; do ./testbin --gtest_filter=LogicTest2.FIFO_SNAPPY_ZSTD_10000_2ENCRYPT_2ENCRYPT_D;done

struct LogicTest2 : testing::Test {
  void SetUp() override {
    //Options::_doubleEncryption = true;
  }
  void testLogic2(CLIENT_TYPE type,
		  COMPRESSORS serverCompressor,
		  COMPRESSORS clientCompressor,
		  std::size_t bufferSize,
		  DIAGNOSTICS diagnostics) {
    ServerOptions::_compressor = serverCompressor;
    ClientOptions::_compressor = clientCompressor;
    // start server
    ServerOptions::_policyEnum = POLICYENUM::NOSORTINPUT;
    ServerPtr server = std::make_shared<Server>();
    ASSERT_TRUE(server->start());
    // start client
    ClientOptions::_compressor = clientCompressor;
    ClientOptions::_bufferSize = bufferSize;
    ClientOptions::_diagnostics = diagnostics;
    switch (type) {
    case CLIENT_TYPE::TCPCLIENT:
      {
	tcp::TcpClient tcpClient;
	tcpClient.run();
      }
      break;
    case CLIENT_TYPE::FIFOCLIENT:
      {
	fifo::FifoClient fifoClient;
	fifoClient.run();
      }
      break;
    }
    std::string_view calibratedOutput = diagnostics == DIAGNOSTICS::ENABLED ?
      TestEnvironment::_outputD : TestEnvironment::_outputND;
    ASSERT_EQ(TestEnvironment::_oss.str(), calibratedOutput);
    server->stop();
  }
  void TearDown() override {
    TestEnvironment::reset();
  }
};

TEST_F(LogicTest2, TCP_LZ4_ZSTD_3600000_2ENCRYPT_2ENCRYPT_D) {
  ServerOptions::_doEncrypt = true;
  ClientOptions::_doEncrypt = true;
  testLogic2(CLIENT_TYPE::TCPCLIENT, COMPRESSORS::LZ4, COMPRESSORS::ZSTD, 3600000, DIAGNOSTICS::ENABLED);
}

TEST_F(LogicTest2, FIFO_SNAPPY_ZSTD_10000_2ENCRYPT_2ENCRYPT_D) {
  ServerOptions::_doEncrypt = true;
  ClientOptions::_doEncrypt = true;
  testLogic2(CLIENT_TYPE::FIFOCLIENT, COMPRESSORS::SNAPPY, COMPRESSORS::ZSTD, 10000, DIAGNOSTICS::ENABLED);
}


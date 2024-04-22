/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <filesystem>

#include "ClientOptions.h"
#include "EchoStrategy.h"
#include "Fifo.h"
#include "FifoClient.h"
#include "Logger.h"
#include "Server.h"
#include "ServerOptions.h"
#include "TcpClient.h"
#include "TestEnvironment.h"

// for i in {1..10}; do ./testbin --gtest_filter=FifoNonblockingTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=EchoTest.TCP_LZ4_LZ4; done
// for i in {1..10}; do ./testbin --gtest_filter=EchoTest.FIFO_LZ4_LZ4; done

struct EchoTest : testing::Test {
  const std::string _originalSource = TestEnvironment::_source;
  void testEchoTcp(COMPRESSORS serverCompressor, COMPRESSORS clientCompressor) {
    // start server
    ServerOptions::_compressor = serverCompressor;
    EchoStrategy strategy;
    ServerPtr server = std::make_shared<Server>(strategy);
    ASSERT_TRUE(server->start());
    // start client
    ClientOptions::_compressor = clientCompressor;
    {
      tcp::TcpClient client;
      client.run();
      ASSERT_EQ(TestEnvironment::_oss.str().size(), _originalSource.size());
      ASSERT_EQ(TestEnvironment::_oss.str(), _originalSource);
    }
    server->stop();
  }

  void testEchoFifo(COMPRESSORS serverCompressor,
		    COMPRESSORS clientCompressor,
		    bool serverEncrypt,
		    bool clientEncrypt) {
    // start server
    ServerOptions::_compressor = serverCompressor;
    ServerOptions::_encrypted = serverEncrypt;
    EchoStrategy strategy;
    ServerPtr server = std::make_shared<Server>(strategy);
    ASSERT_TRUE(server->start());
    // start client
    ClientOptions::_compressor = clientCompressor;
    ClientOptions::_encrypted = clientEncrypt;
    {
      fifo::FifoClient client;
      client.run();
      ASSERT_EQ(TestEnvironment::_oss.str().size(), _originalSource.size());
      ASSERT_EQ(TestEnvironment::_oss.str(), _originalSource);
    }
    server->stop();
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

TEST_F(EchoTest, FIFO_LZ4_LZ4_ENCRYPT_ENCRYPT) {
  testEchoFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, true, true);
}

TEST_F(EchoTest, FIFO_NONE_NONE_ENCRYPT_ENCRYPT) {
  testEchoFifo(COMPRESSORS::NONE, COMPRESSORS::NONE, true, true);
}

TEST_F(EchoTest, FIFO_LZ4_NONE_ENCRYPT_ENCRYPT) {
  testEchoFifo(COMPRESSORS::LZ4, COMPRESSORS::NONE, true, true);
}

TEST_F(EchoTest, FIFO_NONE_LZ4_ENCRYPT_ENCRYPT) {
  testEchoFifo(COMPRESSORS::NONE, COMPRESSORS::LZ4, true, true);
}

TEST_F(EchoTest, FIFO_NONE_LZ4_NOTENCRYPT_ENCRYPT) {
  testEchoFifo(COMPRESSORS::NONE, COMPRESSORS::LZ4, false, true);
}

TEST_F(EchoTest, FIFO_NONE_LZ4_NOTENCRYPT_NOTENCRYPT) {
  testEchoFifo(COMPRESSORS::NONE, COMPRESSORS::LZ4, false, false);
}

TEST_F(EchoTest, FIFO_NONE_LZ4_ENCRYPT_NOTENCRYPT) {
  testEchoFifo(COMPRESSORS::NONE, COMPRESSORS::LZ4, true, false);
}

struct FifoNonblockingTest : testing::Test {
  static constexpr std::string_view _testFifo = "TestFifo";
  static constexpr std::string_view _smallPayload = "abcdefghijklmnopqr0123456789876543210";
  FifoNonblockingTest() {
    if (mkfifo(_testFifo.data(), 0666) == -1 && errno != EEXIST)
      LogError << strerror(errno) << '\n';
  }
  ~FifoNonblockingTest() {
    std::filesystem::remove(_testFifo);
  }
  bool send(std::string_view payload) {
    std::size_t size = payload.size();
    HEADER header{ HEADERTYPE::CREATE_SESSION, size, size, COMPRESSORS::NONE, false, false, STATUS::NONE, 0 };
    return fifo::Fifo::sendMsg(_testFifo, header, payload);
  }
  bool receive(std::string& received) {
    HEADER header;
    return fifo::Fifo::readMsgNonBlock(_testFifo, header, received);
  }

  void testNonblockingFifo(std::string_view payload) {
    std::string received;
    ASSERT_TRUE(std::filesystem::exists(_testFifo));
    auto fs = std::async(std::launch::async, &FifoNonblockingTest::send, this, payload);
    // Optional interval between send and receive
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto fr = std::async(std::launch::async, &FifoNonblockingTest::receive, this, std::ref(received));
    fr.wait();
    fs.wait();
    ASSERT_EQ(received, payload);
  }

  void testNonblockingFifoReverse(std::string_view payload) {
    std::string received;
    ASSERT_TRUE(std::filesystem::exists(_testFifo));
    auto fr = std::async(std::launch::async, &FifoNonblockingTest::receive, this, std::ref(received));
    // Optional interval between receive and send
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto fs = std::async(std::launch::async, &FifoNonblockingTest::send, this, payload);
    fr.wait();
    fs.wait();
    ASSERT_EQ(received, payload);
  }

  void SetUp() {}

  void TearDown() {
    TestEnvironment::reset();
  }
};

TEST_F(FifoNonblockingTest, FifoNonblockingTestD) {
  for (int i = 0; i < 10; ++i) {
    testNonblockingFifo(_smallPayload);
    testNonblockingFifo(TestEnvironment::_source);
  }
}

TEST_F(FifoNonblockingTest, FifoNonblockingTestReverse) {
  for (int i = 0; i < 10; ++i) {
    testNonblockingFifoReverse(_smallPayload);
    testNonblockingFifoReverse(TestEnvironment::_source);
  }
}

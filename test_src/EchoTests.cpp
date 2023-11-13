/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Fifo.h"
#include "ClientOptions.h"
#include "EchoStrategy.h"
#include "FifoClient.h"
#include "Logger.h"
#include "ServerOptions.h"
#include "Server.h"
#include "TcpClient.h"
#include "TestEnvironment.h"
#include <filesystem>

// for i in {1..10}; do ./testbin --gtest_filter=FifoNonblockingTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=EchoTest.TCP_LZ4_LZ4; done

struct EchoTest : testing::Test {
  const std::string _originalSource = TestEnvironment::_source;
  void testEchoTcp(COMPRESSORS serverCompressor, COMPRESSORS clientCompressor) {
    // start server
    ServerOptions::_compressor = serverCompressor;
    Server server(std::make_unique<EchoStrategy>());
    ASSERT_TRUE(server.start());
    // start client
    ClientOptions::_compressor = clientCompressor;
    {
      tcp::TcpClient client;
      client.run();
      ASSERT_EQ(TestEnvironment::_oss.str().size(), _originalSource.size());
      ASSERT_EQ(TestEnvironment::_oss.str(), _originalSource);
    }
    server.stop();
  }

  void testEchoFifo(COMPRESSORS serverCompressor, COMPRESSORS clientCompressor) {
    // start server
    ServerOptions::_compressor = serverCompressor;
    Server server(std::make_unique<EchoStrategy>());
    ASSERT_TRUE(server.start());
    // start client
    ClientOptions::_compressor = clientCompressor;
    {
      fifo::FifoClient client;
      client.run();
      ASSERT_EQ(TestEnvironment::_oss.str().size(), _originalSource.size());
      ASSERT_EQ(TestEnvironment::_oss.str(), _originalSource);
    }
    server.stop();
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

struct FifoNonblockingTest : testing::Test {
  static constexpr std::string_view _testFifo = "TestFifo";
  static constexpr std::string_view _smallPayload = "0123456789876543210";
  FifoNonblockingTest() {
    if (mkfifo(_testFifo.data(), 0666) == -1 && errno != EEXIST)
      LogError << strerror(errno) << '\n';
 }
  ~FifoNonblockingTest() {
    std::filesystem::remove(_testFifo);
  }
  bool send(std::string_view payload) {
    size_t size = payload.size();
    HEADER header{ HEADERTYPE::CREATE_SESSION, size, size, COMPRESSORS::NONE, false, false, STATUS::NONE };
    const std::any& options = ClientOptions();
    return fifo::Fifo::sendMsg(_testFifo, options, header, payload);
  }
  bool receive(std::vector<char>& received) {
    HEADER header;
    return fifo::Fifo::readMsgNonBlock(_testFifo, header, received);
  }

  void testNonblockingFifo(std::string_view payload) {
    static thread_local std::vector<char> received;
    received.clear();
    ASSERT_TRUE(std::filesystem::exists(_testFifo));
    auto fs = std::async(std::launch::async, &FifoNonblockingTest::send, this, payload);
    // Optional interval between send and receive
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto fr = std::async(std::launch::async, &FifoNonblockingTest::receive, this, std::ref(received));
    fr.wait();
    fs.wait();
    ASSERT_EQ(received.size(), payload.size());
    ASSERT_TRUE(std::memcmp(received.data(), payload.data(), payload.size()) == 0);
  }

  void testNonblockingFifoReverse(std::string_view payload) {
    static thread_local std::vector<char> received;
    received.clear();
    ASSERT_TRUE(std::filesystem::exists(_testFifo));
    auto fr = std::async(std::launch::async, &FifoNonblockingTest::receive, this, std::ref(received));
    // Optional interval between receive and send
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto fs = std::async(std::launch::async, &FifoNonblockingTest::send, this, payload);
    fr.wait();
    fs.wait();
    ASSERT_EQ(received.size(), payload.size());
    ASSERT_TRUE(std::memcmp(received.data(), payload.data(), payload.size()) == 0);
  }

  void SetUp() {}

  void TearDown() {}
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

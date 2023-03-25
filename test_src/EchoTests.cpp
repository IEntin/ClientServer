/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Echo.h"
#include "Fifo.h"
#include "ClientOptions.h"
#include "FifoClient.h"
#include "Logger.h"
#include "ServerOptions.h"
#include "Server.h"
#include "TcpClient.h"
#include "TestEnvironment.h"
#include "Utility.h"
#include <filesystem>

// for i in {1..10}; do ./testbin --gtest_filter=FifoNonblockingTest*; done

struct EchoTest : testing::Test {
  void testEchoTcp(COMPRESSORS serverCompressor, COMPRESSORS clientCompressor) {
    try {
      // start server
      TestEnvironment::_serverOptions._processType = "Echo";
      TestEnvironment::_serverOptions._compressor = serverCompressor;
      Server server(TestEnvironment::_serverOptions);
      ASSERT_TRUE(server.start());
      // start client
      TestEnvironment::_clientOptions._compressor = clientCompressor;
      tcp::TcpClient client(TestEnvironment::_clientOptions);
      client.run();
      server.stop();
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
      Server server(TestEnvironment::_serverOptions);
      ASSERT_TRUE(server.start());
      // start client
      TestEnvironment::_clientOptions._compressor = clientCompressor;
      fifo::FifoClient client(TestEnvironment::_clientOptions);
      client.run();
      server.stop();
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

struct FifoNonblockingTest : testing::Test {
  inline static const std::string _testFifo = "TestFifo";
  inline static const std::string _smallPayload = "0123456789876543210";
  int _fdWrite = -1;
  int _fdRead = -1;
  FifoNonblockingTest() = default;
  ~FifoNonblockingTest() {
    close(_fdWrite);
    _fdWrite = -1;
    close(_fdRead);
    _fdRead = -1;
  }
  bool send(std::string_view payload) {
    int fdOld = _fdWrite;
    _fdWrite = fifo::Fifo::openWriteNonBlock(_testFifo, TestEnvironment::_serverOptions, 1000);
    close(fdOld);
    if (TestEnvironment::_serverOptions._setPipeSize)
      fifo::Fifo::setPipeSize(_fdWrite, payload.size());
    size_t size = payload.size();
    HEADER header{ HEADERTYPE::CREATE_SESSION, size, size, COMPRESSORS::NONE, false, STATUS::NONE };
    return fifo::Fifo::sendMsg(_fdWrite, header, payload);
  }

  bool receive(std::vector<char>& received) {
    HEADER header;
    return fifo::Fifo::readMsg(_testFifo, _fdRead, header, received);
  }

  void testNonblockingFifo(std::string_view payload) {
    static thread_local std::vector<char> received;
    try {
      ASSERT_TRUE(std::filesystem::exists(_testFifo));
      auto f1 = std::async(std::launch::async, &FifoNonblockingTest::send, this, payload);
      // uncomment to test send and receive happening at different times.
      //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      auto f2 = std::async(std::launch::async, &FifoNonblockingTest::receive, this, std::ref(received));
      f2.wait();
      ASSERT_EQ(received.size(), payload.size());
      ASSERT_TRUE(std::memcmp(received.data(), payload.data(), payload.size()) == 0);
    }
    catch (const std::exception& e) {
      LogError << e.what() << std::endl;
    }
  }

  void SetUp() {
    ASSERT_TRUE(mkfifo(_testFifo.data(), 0666) != -1 || errno == EEXIST);
  }

  void TearDown() {
    std::filesystem::remove(_testFifo);
  }
};

TEST_F(FifoNonblockingTest, FifoNonblockingTest1) {
  for (int i = 0; i < 10; ++i) {
    testNonblockingFifo(_smallPayload);
    testNonblockingFifo(TestEnvironment::_source);
  }
}

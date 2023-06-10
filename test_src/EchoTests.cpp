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
      {
	tcp::TcpClient client(TestEnvironment::_clientOptions);
	client.run();
	ASSERT_EQ(TestEnvironment::_oss.str().size(), TestEnvironment::_source.size());
	ASSERT_EQ(TestEnvironment::_oss.str(), TestEnvironment::_source);
      }
      server.stop();
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
      {
	fifo::FifoClient client(TestEnvironment::_clientOptions);
	client.run();
      }
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
  FifoNonblockingTest() {
    if (mkfifo(_testFifo.data(), 0666) == -1 && errno != EEXIST)
      LogError << strerror(errno) << std::endl;
 }
  ~FifoNonblockingTest() {
    std::filesystem::remove(_testFifo);
  }
  bool send(std::string_view payload) {
    size_t size = payload.size();
    HEADER header{ HEADERTYPE::CREATE_SESSION, size, size, COMPRESSORS::NONE, false, false, STATUS::NONE };
    return fifo::Fifo::sendMsg(_testFifo, header, TestEnvironment::_serverOptions, payload);
  }
  bool receive(std::vector<char>& received) {
    HEADER header;
    return fifo::Fifo::readMsgNonBlock(_testFifo, header, received);
  }

  void testNonblockingFifo(std::string_view payload) {
    static thread_local std::vector<char> received;
    try {
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
    catch (const std::exception& e) {
      LogError << e.what() << std::endl;
    }
  }

  void testNonblockingFifoReverse(std::string_view payload) {
    static thread_local std::vector<char> received;
    try {
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
    catch (const std::exception& e) {
      LogError << e.what() << std::endl;
    }
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

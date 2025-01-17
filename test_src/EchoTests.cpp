/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <filesystem>

#include "ClientOptions.h"
#include "EchoPolicy.h"
#include "Fifo.h"
#include "FifoClient.h"
#include "Logger.h"
#include "Server.h"
#include "ServerOptions.h"
#include "TcpClient.h"
#include "TestEnvironment.h"

// for i in {1..10}; do ./testbin --gtest_filter=FifoNBDuplex*; done
// for i in {1..10}; do ./testbin --gtest_filter=FifoBlockingTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=EchoTest.TCP_LZ4_LZ4; done
// for i in {1..10}; do ./testbin --gtest_filter=EchoTest.FIFO_LZ4_LZ4_ENCRYPT_ENCRYPT; done

struct EchoTest : testing::Test {
  const std::string _originalSource = TestEnvironment::_source;
  void testEchoTcp(COMPRESSORS serverCompressor, COMPRESSORS clientCompressor) {
    // start server
    ServerOptions::_compressor = serverCompressor;
    EchoPolicy policy;
    ServerPtr server = std::make_shared<Server>(policy);
    ASSERT_TRUE(server->start());
    // start client
    ClientOptions::_compressor = clientCompressor;
    tcp::TcpClient client;
    client.run();
    ASSERT_EQ(TestEnvironment::_oss.str(), _originalSource);
    server->stop();
  }

  void testEchoFifo(COMPRESSORS serverCompressor,
		    COMPRESSORS clientCompressor,
		    bool serverEncrypt,
		    bool clientEncrypt) {
    // start server
    ServerOptions::_compressor = serverCompressor;
    ServerOptions::_encrypted = serverEncrypt;
    EchoPolicy policy;
    ServerPtr server = std::make_shared<Server>(policy);
    ASSERT_TRUE(server->start());
    // start client
    ClientOptions::_compressor = clientCompressor;
    ClientOptions::_encrypted = clientEncrypt;
    fifo::FifoClient client;
    client.run();
    ASSERT_EQ(TestEnvironment::_oss.str(), _originalSource);
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

struct FifoBlockingTest : testing::Test {
  static constexpr std::string_view _testFifo = "TestFifo";
  static constexpr std::string_view _smallPayload = "abcdefghijklmnopqr0123456789876543210";
  FifoBlockingTest() {
    if (mkfifo(_testFifo.data(), 0666) == -1 && errno != EEXIST)
      LogError << strerror(errno) << '\n';
  }
  ~FifoBlockingTest() {
    try {
      std::filesystem::remove(_testFifo);
    }
    catch (const std::exception& e) {
      Warn << e.what() << '\n';
    }
  }
  bool send(std::string_view payload) {
    return fifo::Fifo::sendMsg(false, _testFifo, payload);
  }
  void receive(std::string& received) {
    fifo::Fifo::readStringBlock(_testFifo, received);
  }

  void testBlockingFifo(std::string_view payload) {
    std::string received;
    ASSERT_TRUE(std::filesystem::exists(_testFifo));
    auto fs = std::async(std::launch::async, &FifoBlockingTest::send, this, payload);
    // Optional interval between send and receive
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto fr = std::async(std::launch::async, &FifoBlockingTest::receive, this, std::ref(received));
    fr.wait();
    fs.wait();
    ASSERT_EQ(received, payload);
  }

  void testBlockingFifoReverse(std::string_view payload) {
    std::string received;
    ASSERT_TRUE(std::filesystem::exists(_testFifo));
    auto fr = std::async(std::launch::async, &FifoBlockingTest::receive, this, std::ref(received));
    // Optional interval between receive and send
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto fs = std::async(std::launch::async, &FifoBlockingTest::send, this, payload);
    fr.wait();
    fs.wait();
    ASSERT_EQ(received, payload);
  }

  void SetUp() {}

  void TearDown() {}
};

TEST_F(FifoBlockingTest, FifoBlockingTestD) {
  for (int i = 0; i < 10; ++i) {
    testBlockingFifo(_smallPayload);
    testBlockingFifo(TestEnvironment::_source);
  }
}

TEST_F(FifoBlockingTest, FifoBlockingTestReverse) {
  for (int i = 0; i < 10; ++i) {
    testBlockingFifoReverse(_smallPayload);
    testBlockingFifoReverse(TestEnvironment::_source);
  }
}

// test close to possible usage
struct FifoNBDuplex : testing::Test {
  static constexpr std::string_view _testFifo = "TestFifo";
  static constexpr std::string_view _smallPayload = "abcdefghijklmnopqr0123456789876543210";

  FifoNBDuplex() {
    if (mkfifo(_testFifo.data(), 0666) == -1 && errno != EEXIST)
      LogError << strerror(errno) << '\n';
  }

  ~FifoNBDuplex() {
    try {
      std::filesystem::remove(_testFifo);
    }
    catch (const std::exception& e) {
      Warn << e.what() << '\n';
    }
  }

  // client send
  bool sendC(const HEADER& header, std::string_view data) {
    return fifo::Fifo::sendMsgEOM(false, _testFifo, header, data);
  }
  // client receive
  bool receiveC(HEADER& header, std::string& data) {
    return fifo::Fifo::readMsgUntil(_testFifo, false, header, data);
  }
  // server send
  bool sendS(const HEADER& header, std::string_view data) {
    return fifo::Fifo::sendMsgEOM(false, _testFifo, header, data);
  }
  // server receive
  bool receiveS(HEADER& header, std::string& data) {
    return fifo::Fifo::readMsgUntil(_testFifo, false, header, data);
  }

  void testFifoNBDuplex(std::string_view payload) {
    ASSERT_TRUE(std::filesystem::exists(_testFifo));
    std::size_t size = payload.size();
    HEADER header =
      { HEADERTYPE::SESSION, 0, size, COMPRESSORS::NONE, DIAGNOSTICS::NONE, STATUS::NONE, 0 };
    auto fs = std::async(std::launch::async, &FifoNBDuplex::sendC, this, std::cref(header), payload);
    HEADER headerIntermed;
    std::string dataIntermed;
    auto fr = std::async(std::launch::async, &FifoNBDuplex::receiveS, this, std::ref(headerIntermed), std::ref(dataIntermed));
    fr.wait();
    fs.wait();
    ASSERT_EQ(headerIntermed, header);
    ASSERT_EQ(dataIntermed, payload);
    HEADER headerFin;
    std::string dataFin;
    fs = std::async(std::launch::async, &FifoNBDuplex::sendS, this, std::cref(headerIntermed), dataIntermed);
    fr = std::async(std::launch::async, &FifoNBDuplex::receiveC, this, std::ref(headerFin), std::ref(dataFin));
    fr.wait();
    fs.wait();
    ASSERT_EQ(header, headerFin);
    ASSERT_EQ(payload, dataFin);
  }

  void SetUp() {}

  void TearDown() {}
};

TEST_F(FifoNBDuplex, FifoNBDuplex) {
  for (int i = 0; i < 10; ++i) {
    testFifoNBDuplex(_smallPayload);
    testFifoNBDuplex(TestEnvironment::_source);
  }
}

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

// for i in {1..10}; do ./testbin --gtest_filter=FifoNonblockingTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=FifoNonBlockingDuplexTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=FifoBlockingTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=EchoTest.TCP_LZ4_LZ4; done
// for i in {1..10}; do ./testbin --gtest_filter=EchoTest.FIFO_LZ4_LZ4; done

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
		    CRYPTO serverEncrypt,
		    CRYPTO clientEncrypt) {
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
  testEchoFifo(COMPRESSORS::LZ4, COMPRESSORS::LZ4, CRYPTO::ENCRYPTED, CRYPTO::ENCRYPTED);
}

TEST_F(EchoTest, FIFO_NONE_NONE_ENCRYPT_ENCRYPT) {
  testEchoFifo(COMPRESSORS::NONE, COMPRESSORS::NONE, CRYPTO::ENCRYPTED, CRYPTO::ENCRYPTED);
}

TEST_F(EchoTest, FIFO_LZ4_NONE_ENCRYPT_ENCRYPT) {
  testEchoFifo(COMPRESSORS::LZ4, COMPRESSORS::NONE, CRYPTO::ENCRYPTED, CRYPTO::ENCRYPTED);
}

TEST_F(EchoTest, FIFO_NONE_LZ4_ENCRYPT_ENCRYPT) {
  testEchoFifo(COMPRESSORS::NONE, COMPRESSORS::LZ4, CRYPTO::ENCRYPTED, CRYPTO::ENCRYPTED);
}

TEST_F(EchoTest, FIFO_NONE_LZ4_NOTENCRYPT_ENCRYPT) {
  testEchoFifo(COMPRESSORS::NONE, COMPRESSORS::LZ4, CRYPTO::NONE, CRYPTO::ENCRYPTED);
}

TEST_F(EchoTest, FIFO_NONE_LZ4_NOTENCRYPT_NOTENCRYPT) {
  testEchoFifo(COMPRESSORS::NONE, COMPRESSORS::LZ4, CRYPTO::NONE, CRYPTO::NONE);
}

TEST_F(EchoTest, FIFO_NONE_LZ4_ENCRYPT_NOTENCRYPT) {
  testEchoFifo(COMPRESSORS::NONE, COMPRESSORS::LZ4, CRYPTO::ENCRYPTED, CRYPTO::NONE);
}

struct FifoNonblockingTest : testing::Test {
  static constexpr std::string_view _testFifo = "TestFifo";
  static constexpr std::string_view _smallPayload = "abcdefghijklmnopqr0123456789876543210";
  int _fdRead = -1;
  int _fdWrite = -1;

  FifoNonblockingTest() {
    if (mkfifo(_testFifo.data(), 0666) == -1 && errno != EEXIST)
      LogError << strerror(errno) << '\n';
    _fdRead = fifo::Fifo::openReadNonBlock(_testFifo);
    if (_fdRead == -1)
      throw std::runtime_error(utility::createErrorString());
    _fdWrite = fifo::Fifo::openWriteNonBlockOpenedRead(_testFifo);
    if (_fdWrite == -1)
      throw std::runtime_error(utility::createErrorString());
  }

  ~FifoNonblockingTest() {
  if (_fdRead != -1 && close(_fdRead) == -1)
    LogError << std::strerror(errno) << '\n';
  if (_fdWrite != -1 && close(_fdWrite) == -1)
    LogError << std::strerror(errno) << '\n';
  std::filesystem::remove(_testFifo);
  }

  bool send(std::string_view payload) {
    return fifo::Fifo::sendMsg(_fdWrite, payload);
  }

  bool receive(std::string& received) {
    return fifo::Fifo::readMsgNonBlock(_fdRead, received);
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

struct FifoNonBlockingDuplexTest : testing::Test {
  static constexpr std::string_view _testFifo = "TestFifo";
  static constexpr std::string_view _smallPayload = "abcdefghijklmnopqr0123456789876543210";
  int _fdReadC = -1;// client
  int _fdWriteC = -1;// client
  int _fdReadS = -1;// server
  int _fdWriteS = -1;// server

  FifoNonBlockingDuplexTest() {
    if (mkfifo(_testFifo.data(), 0666) == -1 && errno != EEXIST)
      LogError << strerror(errno) << '\n';
  }

  ~FifoNonBlockingDuplexTest() {
    closeFileDescriptors();
    std::filesystem::remove(_testFifo);
  }

  void closeFileDescriptors() {
    if (_fdReadC != -1 && close(_fdReadC) == -1)
      LogError << std::strerror(errno) << '\n';
    _fdReadC = -1;
    if (_fdWriteC != -1 && close(_fdWriteC) == -1)
      LogError << std::strerror(errno) << '\n';
    _fdWriteC  = -1;
    if (_fdReadS != -1 && close(_fdReadS) == -1)
      LogError << std::strerror(errno) << '\n';
    _fdReadS = -1;
    if (_fdWriteS != -1 && close(_fdWriteS) == -1)
      LogError << std::strerror(errno) << '\n';
    _fdWriteS = -1;
  }
  // client send
  bool sendC(std::string_view payload) {
    return fifo::Fifo::sendMsg(_fdWriteC, payload);
  }
  // client receive
  bool receiveC(std::string& received) {
    return fifo::Fifo::readMsgNonBlock(_fdReadC, received);
  }
  // server send
  bool sendS(std::string_view payload) {
    return fifo::Fifo::sendMsg(_fdWriteS, payload);
  }
  // server receive
  bool receiveS(std::string& received) {
    return fifo::Fifo::readMsgNonBlock(_fdReadS, received);
  }

  void testNonblockingFifoDuplex(std::string_view payload) {
    std::string received;
    ASSERT_TRUE(std::filesystem::exists(_testFifo));
    if (_fdReadC == -1)
      _fdReadC = fifo::Fifo::openReadNonBlock(_testFifo);
    if (_fdReadC == -1)
      throw std::runtime_error(utility::createErrorString());
    if (_fdWriteC == -1)
      _fdWriteC = fifo::Fifo::openWriteNonBlockOpenedRead(_testFifo);
    if (_fdWriteC == -1)
      throw std::runtime_error(utility::createErrorString());
    if (_fdReadS== -1)
      _fdReadS = fifo::Fifo::openReadNonBlock(_testFifo);
    if (_fdReadS == -1)
      throw std::runtime_error(utility::createErrorString());
    if (_fdWriteS == -1)
      _fdWriteS = fifo::Fifo::openWriteNonBlockOpenedRead(_testFifo);
    if (_fdWriteS == -1)
      throw std::runtime_error(utility::createErrorString());
    auto fs = std::async(std::launch::async, &FifoNonBlockingDuplexTest::sendC, this, payload);
    auto fr = std::async(std::launch::async, &FifoNonBlockingDuplexTest::receiveS, this, std::ref(received));
    fr.wait();
    fs.wait();
    ASSERT_EQ(received, payload);
    std::string receivedSaved = received;
    received.clear();
    fs = std::async(std::launch::async, &FifoNonBlockingDuplexTest::sendS, this, receivedSaved);
    fr = std::async(std::launch::async, &FifoNonBlockingDuplexTest::receiveC, this, std::ref(received));
    fr.wait();
    fs.wait();
    ASSERT_EQ(received, receivedSaved);
  }

  void SetUp() {}

  void TearDown() {}
};

TEST_F(FifoNonBlockingDuplexTest, FifoNonBlockingDuplexTest) {
  for (int i = 0; i < 10; ++i) {
    testNonblockingFifoDuplex(_smallPayload);
    testNonblockingFifoDuplex(TestEnvironment::_source);
  }
}

struct FifoBlockingTest : testing::Test {
  static constexpr std::string_view _testFifo = "TestFifo";
  static constexpr std::string_view _smallPayload = "abcdefghijklmnopqr0123456789876543210";
  FifoBlockingTest() {
    if (mkfifo(_testFifo.data(), 0666) == -1 && errno != EEXIST)
      LogError << strerror(errno) << '\n';
  }
  ~FifoBlockingTest() {
    std::filesystem::remove(_testFifo);
  }
  bool send(std::string_view payload) {
    return fifo::Fifo::sendMsg(_testFifo, payload);
  }
  void receive(std::string& received) {
    fifo::Fifo::readMsgBlock(_testFifo, received);
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

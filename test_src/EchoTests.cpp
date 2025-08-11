/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <filesystem>

#include "ClientOptions.h"
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
// for i in {1..10}; do ./testbin --gtest_filter=EchoTest.FIFO_SNAPPY_LZ4_ENCRYPT_ENCRYPT; done
// for i in {1..10}; do ./testbin --gtest_filter=EchoTest.FIFO_NONE_NONE_ENCRYPT_ENCRYPT; done
// gdb --args testbin --gtest_filter=EchoTest.FIFO_NONE_NONE_ENCRYPT_ENCRYPT

static constexpr auto _smallPayload = "abcdefghijklmnopqr0123456789876543210";
static constexpr auto _testFifo = "TestFifo";

struct EchoTest : testing::Test {
  const std::string _originalSource = TestEnvironment::_source;
  void testEcho(CLIENT_TYPE type,
		COMPRESSORS serverCompressor,
		COMPRESSORS clientCompressor) {
    // start server
    ServerOptions::_compressor = serverCompressor;
    ServerOptions::_policyEnum = POLICYENUM::ECHOPOLICY;
    ServerPtr server = std::make_shared<Server>();
    ASSERT_TRUE(server->start());
    // start client
    ClientOptions::_compressor = clientCompressor;
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
    ASSERT_EQ(TestEnvironment::_oss.str(), _originalSource);
    server->stop();
  }

  void TearDown() {
    TestEnvironment::reset();
  }
};

TEST_F(EchoTest, TCP_LZ4_LZ4) {
  testEcho(CLIENT_TYPE::TCPCLIENT, COMPRESSORS::LZ4, COMPRESSORS::LZ4);
}

TEST_F(EchoTest, TCP_NONE_NONE) {
  testEcho(CLIENT_TYPE::TCPCLIENT, COMPRESSORS::NONE, COMPRESSORS::NONE);
}

TEST_F(EchoTest, TCP_LZ4_NONE) {
  testEcho(CLIENT_TYPE::TCPCLIENT, COMPRESSORS::LZ4, COMPRESSORS::NONE);
}

TEST_F(EchoTest, TCP_NONE_LZ4) {
  testEcho(CLIENT_TYPE::TCPCLIENT, COMPRESSORS::NONE, COMPRESSORS::LZ4);
}

TEST_F(EchoTest, FIFO_LZ4_LZ4_ENCRYPT_ENCRYPT) {
  testEcho(CLIENT_TYPE::FIFOCLIENT, COMPRESSORS::LZ4, COMPRESSORS::LZ4);
}

TEST_F(EchoTest, FIFO_SNAPPY_SNAPPY_ENCRYPT_ENCRYPT) {
  testEcho(CLIENT_TYPE::FIFOCLIENT, COMPRESSORS::SNAPPY, COMPRESSORS::SNAPPY);
}

TEST_F(EchoTest, FIFO_SNAPPY_LZ4_ENCRYPT_ENCRYPT) {
  testEcho(CLIENT_TYPE::FIFOCLIENT, COMPRESSORS::SNAPPY, COMPRESSORS::LZ4);
}

TEST_F(EchoTest, FIFO_LZ4_SNAPPY_ENCRYPT_ENCRYPT) {
  testEcho(CLIENT_TYPE::FIFOCLIENT, COMPRESSORS::LZ4, COMPRESSORS::SNAPPY);
}

TEST_F(EchoTest, FIFO_NONE_NONE_ENCRYPT_ENCRYPT) {
  testEcho(CLIENT_TYPE::FIFOCLIENT, COMPRESSORS::NONE, COMPRESSORS::NONE);
}

TEST_F(EchoTest, FIFO_LZ4_NONE_ENCRYPT_ENCRYPT) {
  testEcho(CLIENT_TYPE::FIFOCLIENT, COMPRESSORS::LZ4, COMPRESSORS::NONE);
}

TEST_F(EchoTest, FIFO_NONE_LZ4_ENCRYPT_ENCRYPT) {
  testEcho(CLIENT_TYPE::FIFOCLIENT, COMPRESSORS::NONE, COMPRESSORS::LZ4);
}

TEST_F(EchoTest, FIFO_NONE_LZ4_NOTENCRYPT_ENCRYPT) {
  Options::_doEncrypt = false;
  testEcho(CLIENT_TYPE::FIFOCLIENT, COMPRESSORS::NONE, COMPRESSORS::LZ4);
}

TEST_F(EchoTest, FIFO_NONE_LZ4_NOTENCRYPT_NOTENCRYPT) {
  Options::_doEncrypt = false;
  testEcho(CLIENT_TYPE::FIFOCLIENT, COMPRESSORS::NONE, COMPRESSORS::LZ4);
}

TEST_F(EchoTest, FIFO_NONE_LZ4_ENCRYPT_NOTENCRYPT) {
  testEcho(CLIENT_TYPE::FIFOCLIENT, COMPRESSORS::NONE, COMPRESSORS::LZ4);
}

struct FifoBlockingTest : testing::Test {
  FifoBlockingTest() {
    if (mkfifo(_testFifo, 0666) == -1 && errno != EEXIST)
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
    HEADER header{
      HEADERTYPE::SESSION,
      0,
      payload.size(),
      CRYPTO::CRYPTOPP,
      COMPRESSORS::NONE,
      DIAGNOSTICS::NONE,
      STATUS::NONE,
      0 };
    return fifo::Fifo::sendMessage(true, _testFifo, header, payload);
  }

  void receive(std::string& received) {
    HEADER header;
    std::array<std::reference_wrapper<std::string>, 1> array{std::ref(received)};
    fifo::Fifo::readMessage(_testFifo, true, header, array);
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

  void TearDown() {
    TestEnvironment::reset();
  }
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

struct FifoNBDuplex : testing::Test {
  FifoNBDuplex() {
    if (mkfifo(_testFifo, 0666) == -1 && errno != EEXIST)
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
    return fifo::Fifo::sendMessage(false, _testFifo, header, data);
  }
  // client receive
  bool receiveC(HEADER& header, std::string& data) {
     std::array<std::reference_wrapper<std::string>, 1> array{std::ref(data)};
     return fifo::Fifo::readMessage(_testFifo, true, header, array);
  }
  // server send
  bool sendS(const HEADER& header, std::string_view data) {
    return fifo::Fifo::sendMessage(false, _testFifo, header, data);
  }
  // server receive
  bool receiveS(HEADER& header, std::string& data) {
    std::array<std::reference_wrapper<std::string>, 1> array{std::ref(data)};
    return fifo::Fifo::readMessage(_testFifo, true, header, array);
  }

  void testFifoNBDuplex(std::string_view payload) {
    ASSERT_TRUE(std::filesystem::exists(_testFifo));
    std::size_t size = payload.size();
    HEADER header =
      { HEADERTYPE::SESSION, 0, size, CRYPTO::CRYPTOPP, COMPRESSORS::NONE, DIAGNOSTICS::NONE, STATUS::NONE, 0 };
    auto fs = std::async(std::launch::async, &FifoNBDuplex::sendC, this, std::cref(header), payload);
    HEADER headerIntermed;
    std::string dataIntermed;
    auto fr = std::async(std::launch::async, &FifoNBDuplex::receiveS,
			 this, std::ref(headerIntermed), std::ref(dataIntermed));
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

  void TearDown() {
    TestEnvironment::reset();
  }
};

TEST_F(FifoNBDuplex, FifoNBDuplex) {
  for (int i = 0; i < 10; ++i) {
    testFifoNBDuplex(_smallPayload);
    testFifoNBDuplex(TestEnvironment::_source);
  }
}

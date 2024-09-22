/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <cmath>

#include <boost/asio.hpp>

#include "Crypto.h"
#include "Header.h"
#include "IOUtility.h"
#include "TestEnvironment.h"
#include "Utility.h"

// for i in {1..10}; do ./testbin --gtest_filter=CryptoTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=ReadUntilTest*; done

TEST(CryptoTest, 1) {
  CryptoPP::SecByteBlock key(CryptoPP::AES::MAX_KEYLENGTH);
  Crypto::_rng.GenerateBlock(key, key.size());
  std::string_view data = TestEnvironment::_source;
  std::string_view cipher = Crypto::encrypt(key, data);
  std::string_view decrypted = Crypto::decrypt(key, cipher);
  ASSERT_EQ(data, decrypted);
}

TEST(ReadUntilTest, 1) {
  const auto noop = std::bind([](){});
  boost::asio::io_context ioContext;
  boost::asio::ip::tcp::acceptor
    acceptor(ioContext, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0));
  boost::asio::ip::tcp::socket socketW(ioContext);
  boost::asio::ip::tcp::socket socketR(ioContext);
  // Connect sockets.
  acceptor.async_accept(socketW, noop);
  socketR.async_connect(acceptor.local_endpoint(), noop);
  ioContext.run();
  ioContext.reset();
  std::size_t MAXDATASIZE = std::pow(2, 20);
  std::string data(TestEnvironment::_source, MAXDATASIZE);
  constexpr COMPRESSORS compressor = COMPRESSORS::NONE;
  constexpr CRYPTO encrypted = CRYPTO::ENCRYPTED;
  constexpr DIAGNOSTICS diagnostics = DIAGNOSTICS::NONE;
  HEADER header{
    HEADERTYPE::SESSION, data.size(), compressor, encrypted, diagnostics, STATUS::NONE, 0 };
  char headerBuffer[HEADER_SIZE] = {};
  serialize(header, headerBuffer);
  std::string message(HEADER_SIZE + data.size(), '\0');
  std::size_t offset = 0;
  std::copy(headerBuffer, headerBuffer + HEADER_SIZE, message.begin());
  offset += HEADER_SIZE;
  std::copy(data.begin(), data.end(), message.begin() + offset);
  CryptoPP::SecByteBlock key(CryptoPP::AES::MAX_KEYLENGTH);
  Crypto::_rng.GenerateBlock(key, key.size());
  std::string_view cipher = Crypto::encrypt(key, message);
  ASSERT_TRUE(utility::isEncrypted(cipher));
  const std::string_view delimiter = utility::ENDOFMESSAGE;
  // Write a string from socketW to socketR
  std::vector<boost::asio::const_buffer> buffers{
    boost::asio::buffer(cipher),
    boost::asio::buffer(delimiter) };
  boost::asio::write(socketW, buffers);
  std::string result;
  // Read a result from socketR
  boost::asio::async_read_until(socketR, boost::asio::dynamic_string_buffer(result),
				delimiter,
				[&] (
      const boost::system::error_code& ec,
      std::size_t transferred) {
	 ASSERT_FALSE(ec);
	 ASSERT_EQ(result.size(), transferred);
	 // Extract up to the delimiter
	 result.erase(transferred - delimiter.size());
	 ASSERT_TRUE(utility::isEncrypted(result));
	 std::string_view decrypted = Crypto::decrypt(key, result);
	 HEADER receivedHeader;
	 ASSERT_TRUE(deserialize(receivedHeader, decrypted.data()));
	 ASSERT_EQ(receivedHeader, header);
	 ASSERT_EQ(decrypted, message);
				});
  ioContext.run();
}

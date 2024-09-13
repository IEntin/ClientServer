/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <boost/asio.hpp>

#include "Crypto.h"
#include "IOUtility.h"
#include "TestEnvironment.h"

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
  boost::asio::io_service io_service;
  boost::asio::ip::tcp::acceptor
    acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0));
  boost::asio::ip::tcp::socket socketW(io_service);
  boost::asio::ip::tcp::socket socketR(io_service);
  // Connect sockets.
  acceptor.async_accept(socketW, noop);
  socketR.async_connect(acceptor.local_endpoint(), noop);
  io_service.run();
  io_service.reset();
  constexpr std::string_view element(
	    "abc3456789defghijklmnABCDEFGHIJKLMNOPQRSTUVWZ987654321ab"
	    "c3456789defghijklmnABCDEFGHIJKLMNOPQRSTUVWZ9876543210\n");
  std::string body;
  constexpr std::size_t NUMBERELEMENTS = 10000;
  for (std::size_t i = 0; i < NUMBERELEMENTS; ++i)
    body.append(element);
  constexpr COMPRESSORS compressor = COMPRESSORS::NONE;
  constexpr CRYPTO encrypted = CRYPTO::ENCRYPTED;
  constexpr DIAGNOSTICS diagnostics = DIAGNOSTICS::NONE;
  HEADER header{
    HEADERTYPE::SESSION, body.size(), body.size(), compressor, encrypted, diagnostics, STATUS::NONE, 0 };
  char headerBuffer[HEADER_SIZE] = {};
  serialize(header, headerBuffer);
  std::string message(HEADER_SIZE + body.size(), '\0');
  std::size_t offset = 0;
  std::copy(headerBuffer, headerBuffer + HEADER_SIZE, message.begin());
  offset += HEADER_SIZE;
  std::copy(body.begin(), body.end(), message.begin() + offset);
  CryptoPP::SecByteBlock key(CryptoPP::AES::MAX_KEYLENGTH);
  Crypto::_rng.GenerateBlock(key, key.size());
  std::string_view cipher = Crypto::encrypt(key, message);
  ASSERT_TRUE(utility::isEncrypted(cipher));
  const std::string_view delimiter = ioutility::ENDOFMESSAGE;
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
  io_service.run();
}

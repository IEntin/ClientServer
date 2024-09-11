/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <filesystem>

#include <boost/asio.hpp>

#include "Crypto.h"
#include "IOUtility.h"
#include "Logger.h"
#include "ServerOptions.h"
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
  std::string result;
  constexpr std::string_view element(
	    "abc3456789defghijklmnABCDEFGHIJKLMNOPQRSTUVWZ987654321ab"
	    "c3456789defghijklmnABCDEFGHIJKLMNOPQRSTUVWZ9876543210");
  std::string message;
  for (int i = 0;i < 1000; ++i)
    message.append(element);
  CryptoPP::SecByteBlock key(CryptoPP::AES::MAX_KEYLENGTH);
  std::string_view cipher = Crypto::encrypt(key, message);
  ASSERT_TRUE(utility::isEncrypted(cipher));
  const std::string_view delimiter = ioutility::ENDOFMESSAGE;
  // Write a string from socketW to socketR
  boost::asio::write(socketW, boost::asio::buffer(cipher));
  boost::asio::write(socketW, boost::asio::buffer(std::string(ioutility::ENDOFMESSAGE)));
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
	 ASSERT_EQ(decrypted, message);
				});
  io_service.run();
}

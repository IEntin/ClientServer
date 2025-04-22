
/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <sodium.h>

#include "ClientOptions.h"
#include "CryptoSodium.h"
#include "Header.h"
#include "TestEnvironment.h"
#include "Utility.h"

// for i in {1..10}; do ./testbin --gtest_filter=LibSodiumTest.encryption; done
// for i in {1..10}; do ./testbin --gtest_filter=LibSodiumTest.authentication; done

TEST(LibSodiumTest, authentication) {
  ASSERT_FALSE(sodium_init() < 0);
  // any not less than possible length
  constexpr unsigned MESSAGE_LEN = 23;
  unsigned char MESSAGE[MESSAGE_LEN];
  std::u8string message = utility::generateRawUUIDu8();
  std::copy(message.cbegin(), message.cend(), MESSAGE);
  unsigned char pk[crypto_sign_PUBLICKEYBYTES];
  unsigned char sk[crypto_sign_SECRETKEYBYTES];
  crypto_sign_keypair(pk, sk);
  unsigned char signed_message[crypto_sign_BYTES + MESSAGE_LEN];
  unsigned long long signed_message_len;
  crypto_sign(signed_message, &signed_message_len,
	      MESSAGE, MESSAGE_LEN, sk);
  unsigned char unsigned_message[MESSAGE_LEN];
  unsigned long long unsigned_message_len;
  ASSERT_TRUE(crypto_sign_open(unsigned_message, &unsigned_message_len,
			       signed_message, signed_message_len, pk) == 0);
}

TEST(LibSodiumTest, hashing) {
  ASSERT_FALSE(sodium_init() < 0);
  CryptoSodium crypto;
  std::u8string message = utility::generateRawUUIDu8();
  std::vector<unsigned char> hashed = crypto.hashMessage(message);
  ASSERT_EQ(hashed.size(), crypto_generichash_BYTES);
  std::cout << "generichash:";
  for (auto element : hashed) {
    std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(element);
  }
  std::cout << '\n';
}

TEST(LibSodiumTest, DHkeyExchange) {
  ASSERT_FALSE(sodium_init() < 0);
  unsigned char client_pk[crypto_kx_PUBLICKEYBYTES];
  unsigned char client_sk[crypto_kx_SECRETKEYBYTES];
  unsigned char server_pk[crypto_kx_PUBLICKEYBYTES];
  unsigned char server_sk[crypto_kx_SECRETKEYBYTES];
  unsigned char client_rx[crypto_kx_SESSIONKEYBYTES];
  unsigned char client_tx[crypto_kx_SESSIONKEYBYTES];
  unsigned char server_rx[crypto_kx_SESSIONKEYBYTES];
  unsigned char server_tx[crypto_kx_SESSIONKEYBYTES];
  // Generate key pairs for client and server
  crypto_kx_keypair(client_pk, client_sk);
  crypto_kx_keypair(server_pk, server_sk);
  // Client-side key exchange
  ASSERT_TRUE(crypto_kx_client_session_keys(client_rx, client_tx, client_pk, client_sk, server_pk) == 0);
  // Server-side key exchange
  ASSERT_TRUE(crypto_kx_server_session_keys(server_rx, server_tx, server_pk, server_sk, client_pk) == 0);
  // Verify that the shared secrets match
  ASSERT_TRUE(sodium_memcmp(client_rx, server_tx, crypto_kx_SESSIONKEYBYTES) == 0);
  ASSERT_TRUE(sodium_memcmp(client_tx, server_rx, crypto_kx_SESSIONKEYBYTES) == 0);
}

TEST(LibSodiumTest, encryption) {
  ASSERT_FALSE(sodium_init() < 0);
  ASSERT_TRUE(crypto_aead_aes256gcm_is_available());
  ASSERT_FALSE(sodium_init() < 0);
  HEADER header{ HEADERTYPE::SESSION, 0, HEADER_SIZE + TestEnvironment::_source.size(), ClientOptions::_encryption,
		 COMPRESSORS::NONE, DIAGNOSTICS::NONE, STATUS::NONE, 0 };
  CryptoSodium crypto;
  unsigned char key[crypto_aead_aes256gcm_KEYBYTES];
  crypto_aead_aes256gcm_keygen(key);
  crypto.setTestAesKey(key);
  std::vector<unsigned char> ciphertext;
  std::string input;
  char headerBuffer[HEADER_SIZE] = {};
  input.append(headerBuffer, HEADER_SIZE);
  input.insert(input.end(), TestEnvironment::_source.cbegin(), TestEnvironment::_source.cend());
  unsigned long long ciphertext_len;
  crypto.encrypt(input, header, ciphertext, ciphertext_len);
  ASSERT_TRUE(utility::isEncrypted(ciphertext));
  std::string decrypted;
  ASSERT_TRUE(crypto.decrypt(ciphertext, decrypted));
  ASSERT_FALSE(utility::isEncrypted(decrypted));
  HEADER recoveredHeader;
  deserialize(recoveredHeader, decrypted.data());
  ASSERT_EQ(header, recoveredHeader);
  ASSERT_TRUE(decrypted == input);
}

TEST(LibSodiumTest, publicKeyEncoding) {
  ASSERT_FALSE(sodium_init() < 0);
  CryptoSodium crypto;
  unsigned char public_key[crypto_box_PUBLICKEYBYTES];
  unsigned char secret_key[crypto_box_SECRETKEYBYTES];
  crypto_box_keypair(public_key, secret_key);
  std::vector<unsigned char> original_data(public_key, public_key + crypto_box_PUBLICKEYBYTES);
  std::string encoded = crypto.base64_encode(original_data);
  std::vector<unsigned char> decoded_data = crypto.base64_decode(encoded);
  ASSERT_EQ(original_data, decoded_data);
}

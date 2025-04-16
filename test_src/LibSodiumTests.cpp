
/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <iostream>
#include <stdexcept>
#include <vector>

#include <sodium.h>

#include "Logger.h"
#include "TestEnvironment.h"

// for i in {1..10}; do ./testbin --gtest_filter=LibSodiumTest.encryption; done
// for i in {1..10}; do ./testbin --gtest_filter=LibSodiumTest.authentication; done

TEST(LibSodiumTest, authentication) {
  ASSERT_FALSE(sodium_init() < 0);
  constexpr const unsigned char MESSAGE[] = "This is a message to sign";
  constexpr unsigned MESSAGE_LEN = sizeof(MESSAGE);
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
  std::u8string message(u8"This is a message to hash");
  const unsigned char* input = reinterpret_cast<const unsigned char*>(message.data());
  unsigned char hash[crypto_generichash_BYTES];
  crypto_generichash(hash, sizeof(hash), input, message.size(), nullptr, 0);
  std::u8string u8str(reinterpret_cast<const char8_t*>(&hash[0]), sizeof(hash));
  std::cout << "Hash: ";
  for (unsigned i = 0; i < u8str.size(); ++i)
    std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(u8str[i]);
  std::cout << '\n';
}

std::string base64_encode(const std::vector<unsigned char>& input) {
  size_t encoded_length = sodium_base64_ENCODED_LEN(input.size(), sodium_base64_VARIANT_ORIGINAL);
  std::string encoded_string(encoded_length, '\0');
  if (sodium_bin2base64(encoded_string.data(), encoded_length, input.data(), input.size(),
			sodium_base64_VARIANT_ORIGINAL) == nullptr)
    return {};
  encoded_string.resize(encoded_length - 1);
  return encoded_string;
}

std::vector<unsigned char> base64_decode(const std::string& input) {
  std::size_t decoded_length = input.size(); // Maximum possible decoded length
  std::vector<unsigned char> decoded_data(decoded_length);  
  if (sodium_base642bin(decoded_data.data(), decoded_length, input.data(), input.size(), nullptr,
			&decoded_length, nullptr, sodium_base64_VARIANT_ORIGINAL) != 0)
    return {};
  decoded_data.resize(decoded_length);
  return decoded_data;
}

TEST(LibSodiumTest, base64EncodePublicKey) {
  ASSERT_FALSE(sodium_init() < 0);
  unsigned char public_key[crypto_box_PUBLICKEYBYTES];
  unsigned char secret_key[crypto_box_SECRETKEYBYTES];
  crypto_box_keypair(public_key, secret_key);
  std::vector<unsigned char> original_data(public_key, public_key + crypto_box_PUBLICKEYBYTES);
  std::string encoded = base64_encode(original_data);
  std::vector<unsigned char> decoded_data = base64_decode(encoded);
  ASSERT_EQ(original_data, decoded_data);
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
  unsigned char key[crypto_aead_aes256gcm_KEYBYTES];
  crypto_aead_aes256gcm_keygen(key);
  unsigned char nonce[crypto_aead_aes256gcm_NPUBBYTES];
  randombytes_buf(nonce, sizeof nonce);
  std::string message = TestEnvironment::_source;
  size_t message_len = message.size();
  std::vector<unsigned char> ciphertext(message_len + crypto_aead_aes256gcm_ABYTES);
  unsigned long long ciphertext_len;
  ASSERT_TRUE(crypto_aead_aes256gcm_encrypt(ciphertext.data(), &ciphertext_len,
					    reinterpret_cast<unsigned char*>(message.data()), message_len,
					    nullptr, 0,
					    nullptr, nonce, key) == 0);
  std::string decrypted(message_len, '\0');
  unsigned long long decrypted_len;
  ASSERT_TRUE(crypto_aead_aes256gcm_decrypt(reinterpret_cast<unsigned char*>(decrypted.data()), &decrypted_len,
					    nullptr,
					    ciphertext.data(), ciphertext_len,
					    nullptr, 0,
					    nonce, key) == 0);
  ASSERT_TRUE(decrypted == message);
}

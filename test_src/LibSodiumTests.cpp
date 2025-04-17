
/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <iomanip>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <vector>

#include <sodium.h>

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

TEST(LibSodiumTest, base64Encode) {
  ASSERT_FALSE(sodium_init() < 0);
  constexpr int N = 1234;
  std::vector<unsigned char> original_data(N);
  std::iota(original_data.begin(), original_data.end(), 1);
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
  std::size_t message_len = message.size();
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

std::vector<unsigned char> encodeLength(size_t length) {
  std::vector<unsigned char> encodedLength;
  if (length < 128) {
    encodedLength.push_back(static_cast<unsigned char>(length));
  }
  else {
    size_t numBytes = 0;
    size_t tempLength = length;
    while (tempLength > 0) {
      numBytes++;
      tempLength >>= 8;
    }
    encodedLength.push_back(static_cast<unsigned char>(0x80 | numBytes));
    for (size_t i = numBytes; i > 0; --i) {
      encodedLength.push_back(static_cast<unsigned char>((length >> (8 * (i - 1))) & 0xFF));
    }
  }
  return encodedLength;
}

std::vector<unsigned char> ber_encode_string(const std::string& str) {
  std::vector<unsigned char> berEncoded;
  // Add tag for OCTET STRING (0x04)
  berEncoded.push_back(0x04);
  // Encode and add length
  std::vector<unsigned char> encodedLength = encodeLength(str.length());
  berEncoded.insert(berEncoded.end(), encodedLength.begin(), encodedLength.end());
  // Add string bytes
  for (char c : str) {
    berEncoded.push_back(static_cast<unsigned char>(c));
  }
  return berEncoded;
}

std::string ber_decode_string(const std::vector<unsigned char>& encoded) {
  if (encoded.empty() || encoded[0] != 0x04) {
    return "";
  }
  size_t length = 0;
  size_t data_start_index = 2;
  if ((encoded[1] & 0x80) == 0) {
    // Short form length
    length = encoded[1];
  }
  else {
    int length_bytes = encoded[1] & 0x7F;
    if(length_bytes == 1){
      length = encoded[2];
      data_start_index = 3;
    }
    else{
      return "";
    }
  }
  if (encoded.size() < data_start_index + length)
    return "";
  return std::string(encoded.begin() + data_start_index, encoded.begin() + data_start_index + length);
}

TEST(LibSodiumTest, publicKeyBerEncoding) {
  unsigned char pk[crypto_sign_PUBLICKEYBYTES];
  unsigned char sk[crypto_sign_SECRETKEYBYTES];
  crypto_sign_keypair(pk, sk);
  std::string original(pk, pk + crypto_sign_PUBLICKEYBYTES);
  std::vector<unsigned char> encoded = ber_encode_string(original);
  std::string decoded = ber_decode_string(encoded);
  ASSERT_TRUE(original == decoded);
}

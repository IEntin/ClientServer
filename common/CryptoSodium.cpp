/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoSodium.h"

#include <cstring>
#include <stdexcept>

#include "ClientOptions.h"
#include "ServerOptions.h"
#include "Utility.h"

HandleKey::HandleKey() :
  _size(crypto_aead_aes256gcm_KEYBYTES),
  _obfuscated(false) {
  if (sodium_init() < 0)
    throw std::runtime_error("sodium_init failed");
  randombytes_buf(_obfuscator, _size);
}

void HandleKey::hideKey(unsigned char* key) {
  if (!_obfuscated) {
    // refresh obfuscator
    randombytes(_obfuscator, _size);
    for (unsigned i = 0; i < _size; ++i)
      key[i] ^= _obfuscator[i];
    _obfuscated = true;
  }
}

void HandleKey::recoverKey(unsigned char* key) {
  if (_obfuscated) {
    for (unsigned i = 0; i < _size; ++i)
      key[i] ^= _obfuscator[i];
    _obfuscated = false;
  }
}

CryptoSodium::CryptoSodium(std::u8string_view msg) :
  _msgHash(hashMessage(msg)) {
  if (sodium_init() < 0)
    throw std::runtime_error("sodium_init failed");
  unsigned char publicKeySign[crypto_sign_PUBLICKEYBYTES];
  crypto_sign_keypair(publicKeySign, _secretKeySign);
  _publicKeySign = std::to_array(publicKeySign);
  unsigned char message[crypto_generichash_BYTES];
  std::copy(_msgHash.cbegin(), _msgHash.cend(), message);
  unsigned char signature[crypto_sign_BYTES];
  crypto_sign_detached(signature, nullptr, message, _msgHash.size(), _secretKeySign);
  _signature = std::to_array(signature);
}

void CryptoSodium::setDummyAesKey() {
  unsigned char key[crypto_aead_aes256gcm_KEYBYTES];
  crypto_aead_aes256gcm_keygen(key);
  std::copy(key, key + crypto_aead_aes256gcm_KEYBYTES, _key);
  _keyHandler.hideKey(_key);
}

std::string_view CryptoSodium::encrypt(std::string& buffer,
				       const HEADER& header,
				       std::string_view data) {
  if (!checkAccess())
    return "";
  if (sodium_init() < 0)
    throw std::runtime_error("sodium_init failed");
  buffer.clear();
  std::string input;
  char headerBuffer[HEADER_SIZE] = {};
  input.append(headerBuffer, HEADER_SIZE);
  input.insert(input.end(), data.cbegin(), data.cend());
  unsigned long long ciphertext_len;
  serialize(header, input.data());
  unsigned char nonce[crypto_aead_aes256gcm_NPUBBYTES];
  randombytes_buf(nonce, sizeof nonce);
  std::size_t message_len = input.size();
  buffer.resize(message_len + crypto_aead_aes256gcm_ABYTES);
  unsigned char key[crypto_aead_aes256gcm_KEYBYTES] = {};
  setAESKey(key);
  if (!(crypto_aead_aes256gcm_encrypt(std::bit_cast<unsigned char*>(buffer.data()),
				      &ciphertext_len,
				      std::bit_cast<unsigned char*>(input.data()),
				      message_len, nullptr, 0,
				      nullptr, nonce, key) == 0))
    return "";
  buffer.insert(buffer.end(), nonce, nonce + crypto_aead_aes256gcm_NPUBBYTES);
  return buffer;
}

void CryptoSodium::decrypt(std::string& buffer, std::string& data) {
  if (!checkAccess())
    return;
  if (sodium_init() < 0)
    throw std::runtime_error("sodium_init failed");
  buffer.clear();
  if (utility::isEncrypted(data)) {
    unsigned long long ciphertext_len = data.size() - crypto_aead_aes256gcm_NPUBBYTES;
    unsigned char recoveredNonce[crypto_aead_aes256gcm_NPUBBYTES];
    std::copy(data.end() - crypto_aead_aes256gcm_NPUBBYTES, data.end(), recoveredNonce);
    data.erase(data.end() - crypto_aead_aes256gcm_NPUBBYTES);
    buffer.resize(ciphertext_len);
    unsigned long long decrypted_len;
    unsigned char key[crypto_aead_aes256gcm_KEYBYTES] = {};
    setAESKey(key);
    bool success = crypto_aead_aes256gcm_decrypt(std::bit_cast<unsigned char*>(buffer.data()),
						 &decrypted_len,
						 nullptr,
						 std::bit_cast<unsigned char*>(data.data()), ciphertext_len,
						 nullptr, 0,
						 recoveredNonce, key) == 0;
    if (!success)
      return;
    buffer.resize(decrypted_len);
    data = buffer;
    if (ClientOptions::_showKey)
      showKey();
  }
}

std::string CryptoSodium::base64_encode(const std::vector<unsigned char>& input) {
  size_t encoded_length = sodium_base64_ENCODED_LEN(input.size(), sodium_base64_VARIANT_ORIGINAL);
  std::string encoded_string(encoded_length, '\0');
  if (sodium_bin2base64(encoded_string.data(), encoded_length, input.data(), input.size(),
			sodium_base64_VARIANT_ORIGINAL) == nullptr)
    return "";
  //The sodium_bin2base64 function adds a null terminator, but it is not part of the base64 string itself.
  encoded_string.resize(encoded_length - 1);
  return encoded_string;
}

std::vector<unsigned char> CryptoSodium::base64_decode(const std::string& input) {
  std::size_t decoded_length = input.size(); // Maximum possible decoded length
  std::vector<unsigned char> decoded_data(decoded_length);  
  if (sodium_base642bin(decoded_data.data(), decoded_length,
			input.data(), input.size(),
			nullptr, &decoded_length, nullptr, sodium_base64_VARIANT_ORIGINAL) != 0)
    return {};
  decoded_data.resize(decoded_length);
  return decoded_data;
}

std::array<unsigned char, crypto_generichash_BYTES>
CryptoSodium::hashMessage(std::u8string_view message) {
  constexpr int MESSAGE_LEN = 22;
  unsigned char MESSAGE[crypto_generichash_BYTES];
  std::copy(message.cbegin(), message.cend(), MESSAGE);
  std::array<unsigned char, crypto_generichash_BYTES> hash;
  unsigned char key[crypto_generichash_KEYBYTES];
  randombytes_buf(key, sizeof key);
  crypto_generichash(&hash[0], sizeof hash,
		     MESSAGE, MESSAGE_LEN,
		     key, sizeof key);
  assert(MESSAGE_LEN > message.size());
  return hash;
}

void CryptoSodium::showKey() {
  if (!checkAccess())
    return;
  Logger logger(LOG_LEVEL::INFO, std::clog, false);
  if (logger._level >= Logger::_threshold) {
    logger << "KEY: 0x";
    boost::algorithm::hex(_key, std::ostream_iterator<char> { logger.getStream() });
    logger << '\n';
  }
}

bool CryptoSodium::checkAccess() {
  if (utility::isServerTerminal())
    return _verified;
  else if (utility::isClientTerminal())
    return _signatureSent;
  else if (utility::isTestbinTerminal())
    return true;
  return false;
}

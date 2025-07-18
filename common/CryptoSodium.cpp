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
  randombytes_buf(_obfuscator.data(), _size);
}

void HandleKey::hideKey(std::array<unsigned char, crypto_aead_aes256gcm_KEYBYTES>& key) {
  if (!_obfuscated) {
    // refresh obfuscator
    randombytes(_obfuscator.data(), _size);
    for (unsigned i = 0; i < _size; ++i)
      key[i] ^= _obfuscator[i];
    _obfuscated = true;
  }
}

void HandleKey::recoverKey(std::array<unsigned char, crypto_aead_aes256gcm_KEYBYTES>& key) {
  if (_obfuscated) {
    for (unsigned i = 0; i < _size; ++i)
      key[i] ^= _obfuscator[i];
    _obfuscated = false;
  }
}

// client
CryptoSodium::CryptoSodium(std::u8string_view msg) :
  _msgHash(hashMessage(msg)),
  _signatureWithPubKeySign(crypto_sign_BYTES + crypto_sign_PUBLICKEYBYTES) {
  crypto_kx_keypair(_publicKeyAes.data(), _secretKeyAes.data());
  DebugLog::logBinaryData(BOOST_CURRENT_LOCATION, "_publicKeyAes  ", _publicKeyAes);
  crypto_sign_keypair(_publicKeySign.data(), _secretKeySign.data());
  crypto_sign_detached(_signature.data(), nullptr, _msgHash.data(),
		       _msgHash.size(), _secretKeySign.data());
  
  std::copy(_signature.cbegin(), _signature.cend(), _signatureWithPubKeySign.begin());
  std::copy(_publicKeySign.cbegin(), _publicKeySign.cend(),
	    _signatureWithPubKeySign.begin() + _signature.size());
}
// server
CryptoSodium::CryptoSodium(std::span<const unsigned char> msgHash,
			   std::span<const unsigned char> pubKeyAesClient,
			   std::span<const unsigned char> signatureWithPubKey) {
  DebugLog::logBinaryData(BOOST_CURRENT_LOCATION, "pubKeyAesClient", pubKeyAesClient);
  crypto_kx_keypair(_publicKeyAes.data(), _secretKeyAes.data());
  std::span<const unsigned char>
    signature(signatureWithPubKey.data(), crypto_sign_BYTES);
  std::span<const unsigned char>
    peerPubcicKeySign(signatureWithPubKey.data() + crypto_sign_BYTES, crypto_sign_PUBLICKEYBYTES);
  _verified = crypto_sign_verify_detached(
    signature.data(), msgHash.data(), msgHash.size(), peerPubcicKeySign.data()) == 0;
  if (!_verified)
    throw std::runtime_error("authentication failed");
  // Server-side key exchange
  std::array<unsigned char, crypto_kx_SESSIONKEYBYTES> server_rx;
  std::array<unsigned char, crypto_kx_SESSIONKEYBYTES> server_tx;
  if (crypto_kx_server_session_keys(server_rx.data(), server_tx.data(), _publicKeyAes.data(), _secretKeyAes.data(), pubKeyAesClient.data()) != 0)
    throw std::runtime_error("Server-side key exchange failed");
  std::swap(_key, server_tx);
  DebugLog::logBinaryData(BOOST_CURRENT_LOCATION, "_key ", _key);
  _keyHandler.hideKey(_key);
  if (ServerOptions::_showKey)
    showKey();
  eraseUsedData();
}

void CryptoSodium::setDummyAesKey() {
  unsigned char key[crypto_aead_aes256gcm_KEYBYTES];
  crypto_aead_aes256gcm_keygen(key);
  std::copy(key, key + crypto_aead_aes256gcm_KEYBYTES, _key.data());
  _keyHandler.hideKey(_key);
}

std::string_view CryptoSodium::encrypt(std::string& buffer,
				       const HEADER& header,
				       std::string_view data) {
  if (!checkAccess())
    throw std::runtime_error("access denied");
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
  std::array<unsigned char, crypto_aead_aes256gcm_KEYBYTES> key;
  setAESKey(key);
  if (!(crypto_aead_aes256gcm_encrypt(static_cast<unsigned char*>(static_cast<void*>(buffer.data())),
				      &ciphertext_len,
				      static_cast<unsigned char*>(static_cast<void*>(input.data())),
				      message_len, nullptr, 0,
				      nullptr, nonce, key.data()) == 0))
    throw std::runtime_error("encrypt failed");
  buffer.insert(buffer.end(), nonce, nonce + crypto_aead_aes256gcm_NPUBBYTES);
  return buffer;
}

void CryptoSodium::decrypt(std::string& buffer, std::string& data) {
  if (!checkAccess())
    throw std::runtime_error("access denied");
  buffer.clear();
  if (utility::isEncrypted(data)) {
    unsigned long long ciphertext_len = data.size() - crypto_aead_aes256gcm_NPUBBYTES;
    unsigned char recoveredNonce[crypto_aead_aes256gcm_NPUBBYTES];
    std::copy(data.end() - crypto_aead_aes256gcm_NPUBBYTES, data.end(), recoveredNonce);
    data.erase(data.end() - crypto_aead_aes256gcm_NPUBBYTES);
    buffer.resize(ciphertext_len);
    unsigned long long decrypted_len;
    std::array<unsigned char, crypto_aead_aes256gcm_KEYBYTES> key;
    setAESKey(key);
    bool success = crypto_aead_aes256gcm_decrypt(static_cast<unsigned char*>(static_cast<void*>(buffer.data())),
						 &decrypted_len,
						 nullptr,
						 static_cast<unsigned char*>(static_cast<void*>(data.data())),
						 ciphertext_len,
						 nullptr, 0,
						 recoveredNonce, key.data()) == 0;
    if (!success)
      throw std::runtime_error("decrypt failed");
    buffer.resize(decrypted_len);
    data = buffer;
    if (ClientOptions::_showKey)
      showKey();
  }
}

bool CryptoSodium::clientKeyExchange(std::span<const unsigned char> pubKeyAesServer) {
  std::array<unsigned char, crypto_kx_SESSIONKEYBYTES> client_rx;
  std::array<unsigned char, crypto_kx_SESSIONKEYBYTES> client_tx;
  if (crypto_kx_client_session_keys(client_rx.data(), client_tx.data(), _publicKeyAes.data(), _secretKeyAes.data(), pubKeyAesServer.data()) != 0)
    throw std::runtime_error("Client-side key exchange failed");
  std::swap(_key, client_rx);
  DebugLog::logBinaryData(BOOST_CURRENT_LOCATION, "_key ", _key);
  _keyHandler.hideKey(_key);
  eraseUsedData();
  return true;
}

std::string CryptoSodium::base64_encode(std::span<const unsigned char> input) {
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

std::vector<unsigned char>
CryptoSodium::hashMessage(std::u8string_view message) {
  unsigned char MESSAGE[crypto_generichash_BYTES];
  std::copy(message.cbegin(), message.cend(), MESSAGE);
  std::vector<unsigned char> hash(crypto_generichash_BYTES);
  unsigned char key[crypto_generichash_KEYBYTES];
  randombytes_buf(key, sizeof key);
  crypto_generichash(hash.data(), crypto_generichash_BYTES,
		     MESSAGE, crypto_generichash_BYTES,
		     key, sizeof key);
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

std::array<unsigned char, crypto_aead_aes256gcm_KEYBYTES> CryptoSodium::getAesKey() {
  assert(utility::isTestbinTerminal() && "Only in tests");
  std::array<unsigned char, crypto_aead_aes256gcm_KEYBYTES> key;
  _keyHandler.recoverKey(_key);
  key = _key;
  _keyHandler.hideKey(_key);
  return key;
}

void CryptoSodium::eraseUsedData() {
  std::vector<unsigned char>().swap(_msgHash);
  std::array<unsigned char, crypto_kx_SECRETKEYBYTES>().swap(_secretKeyAes);
}

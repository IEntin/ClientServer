/*
 *  Copyright (C) 2021 Ilya Entin
 */
#include "CryptoSodium.h"

#include <cstring>
#include <stdexcept>

#include "CryptoDefinitions.h"
#include "DebugLog.h"
#include "Utility.h"

HandleKey::HandleKey() :
  _size(crypto_kx_SESSIONKEYBYTES),
  _obfuscated(false) {
  randombytes_buf(_obfuscator.data(), _size);
}

void HandleKey::hideKey(std::array<unsigned char, crypto_kx_SESSIONKEYBYTES>& key) {
  if (!_obfuscated) {
    // refresh obfuscator
    randombytes(_obfuscator.data(), _size);
    for (unsigned i = 0; i < _size; ++i)
      key[i] ^= _obfuscator[i];
    _obfuscated = true;
  }
}

void HandleKey::recoverKey(std::array<unsigned char, crypto_kx_SESSIONKEYBYTES>& key) {
  if (_obfuscated) {
    for (unsigned i = 0; i < _size; ++i)
      key[i] ^= _obfuscator[i];
    _obfuscated = false;
  }
}

void CryptoSodium::setAESKey(SessionKey& key) {
  std::lock_guard lock(_mutex);
  _keyHandler.recoverKey(_key);
  key = _key;
  _keyHandler.hideKey(_key);
}

// client
CryptoSodium::CryptoSodium(std::string_view msg) :
  _msgHash(hashMessage(msg)) {
  crypto_kx_keypair(_pubKeyAes.data(), _privKeyAes.data());
  _encodedPubKeyAes = base64_encode(_pubKeyAes);
  crypto_sign_keypair(_publicKeySign.data(), _secretKeySign.data());
  crypto_sign_detached(_signature.data(),
		       NULL,
		       std::bit_cast<const unsigned char *>(_msgHash.data()),
		       _msgHash.size(),
		       _secretKeySign.data());
  
  _signatureWithPubKeySign.assign(_signature.cbegin(), _signature.cend());
  _signatureWithPubKeySign.append(_publicKeySign.cbegin(), _publicKeySign.cend());
}

CryptoSodium::CryptoSodium(std::string_view msgHash,
			   std::string_view encodedPeerAesPubKey,
			   std::string_view signatureWithPubKeySign) :
  _msgHash(msgHash.data(), msgHash.size()) {
  std::vector<unsigned char> peerAesPubKeyV = base64_decode(encodedPeerAesPubKey);
  std::copy(peerAesPubKeyV.cbegin(), peerAesPubKeyV.cend(), _peerPubKeyAes.begin());
  crypto_kx_keypair(_pubKeyAes.data(), _privKeyAes.data());
  _encodedPubKeyAes = base64_encode(_pubKeyAes);
  unsigned char signature[crypto_sign_BYTES] = {};
  std::copy(signatureWithPubKeySign.begin(), signatureWithPubKeySign.begin() + crypto_sign_BYTES, signature);
  unsigned char peerPubcicKeySign[crypto_sign_PUBLICKEYBYTES] = {};
  std::copy(signatureWithPubKeySign.begin() + crypto_sign_BYTES, signatureWithPubKeySign.end(), std::begin(peerPubcicKeySign));
  _verified = crypto_sign_verify_detached(
    signature,
    std::bit_cast<const unsigned char*>(_msgHash.data()),
    std::ssize(_msgHash),
    peerPubcicKeySign) == 0;
  if (!_verified)
    throw std::runtime_error("authentication failed");
  // Server-side key exchange
  if (crypto_kx_server_session_keys(_key.data(),
				    nullptr,
				    _pubKeyAes.data(),
				    _privKeyAes.data(),
				    _peerPubKeyAes.data()) != 0)
    throw std::runtime_error("Server-side key exchange failed");
  DebugLog::logBinaryData(BOOST_CURRENT_LOCATION, "_key", _key);
  _keyHandler.hideKey(_key);
  eraseUsedData();
}

std::string_view CryptoSodium::encrypt(std::string& buffer,
				       const HEADER& header,
				       std::string_view data) {
  if (!checkAccess())
    throw std::runtime_error("access denied");
  buffer.clear();
  std::string input(HEADER_SIZE, '\0');
  serialize(header, input.data());
  input.append(data.cbegin(), data.cend());
  unsigned long long ciphertext_len;
  unsigned char nonce[crypto_aead_aes256gcm_NPUBBYTES] = {};
  randombytes_buf(nonce, std::ssize(nonce));
  std::size_t message_len = std::ssize(input);
  buffer.resize(message_len + crypto_aead_aes256gcm_ABYTES);
  std::array<unsigned char, crypto_kx_SESSIONKEYBYTES> key;
  setAESKey(key);
  if (!(crypto_aead_aes256gcm_encrypt(std::bit_cast<unsigned char*>(buffer.data()),
				      &ciphertext_len,
				      std::bit_cast<unsigned char*>(input.data()),
				      message_len, nullptr, 0,
				      nullptr, nonce, key.data()) == 0))
    throw std::runtime_error("encrypt failed");
  buffer.insert(buffer.end(), std::cbegin(nonce), std::cend(nonce));
  return buffer;
}

void CryptoSodium::hideKey() {
  _keyHandler.hideKey(_key);
}

void CryptoSodium::decrypt(std::string& buffer, std::string& data) {
  if (!checkAccess())
    throw std::runtime_error("access denied");
  buffer.clear();
  if (cryptodefinitions::isEncrypted(data)) {
    unsigned long long ciphertext_len = std::ssize(data) - crypto_aead_aes256gcm_NPUBBYTES;
    unsigned char recoveredNonce[crypto_aead_aes256gcm_NPUBBYTES] = {};
    std::copy(data.end() - crypto_aead_aes256gcm_NPUBBYTES, data.end(), recoveredNonce);
    data.erase(data.end() - crypto_aead_aes256gcm_NPUBBYTES);
    buffer.resize(ciphertext_len);
    unsigned long long decrypted_len;
    std::array<unsigned char, crypto_kx_SESSIONKEYBYTES> key;
    setAESKey(key);
    bool success = crypto_aead_aes256gcm_decrypt(std::bit_cast<unsigned char*>(buffer.data()),
						 &decrypted_len,
						 nullptr,
						 std::bit_cast<unsigned char*>(data.data()),
						 ciphertext_len,
						 nullptr, 0,
						 recoveredNonce, key.data()) == 0;
    if (!success)
      throw std::runtime_error("decrypt failed");
    buffer.resize(decrypted_len);
    data = buffer;
  }
}

bool CryptoSodium::clientKeyExchange(std::string_view encodedPeerPubKeyAes) {
  std::vector<unsigned char> peerPubKeyAesV = base64_decode(encodedPeerPubKeyAes);
  std::copy(peerPubKeyAesV.cbegin(), peerPubKeyAesV.cend(), _peerPubKeyAes.begin());
  if (crypto_kx_client_session_keys(nullptr,
				    _key.data(),
				    _pubKeyAes.data(),
				    _privKeyAes.data(),
				    _peerPubKeyAes.data()) != 0)
    throw std::runtime_error("Client-side key exchange failed");
  DebugLog::logBinaryData(BOOST_CURRENT_LOCATION, "_key", _key);
  hideKey();
  eraseUsedData();
  return true;
}

std::string CryptoSodium::base64_encode(std::span<unsigned char> input) {
  size_t encoded_length = sodium_base64_ENCODED_LEN(std::ssize(input), sodium_base64_VARIANT_ORIGINAL);
  std::string encoded_string(encoded_length, '\0');
  if (sodium_bin2base64(encoded_string.data(), encoded_length, input.data(), std::ssize(input),
			sodium_base64_VARIANT_ORIGINAL) == nullptr)
    throw std::runtime_error("sodium_bin2base64 failed");
  encoded_string.resize(strlen(encoded_string.data()));
  return encoded_string;
}

std::vector<unsigned char> CryptoSodium::base64_decode(std::string_view encoded) {
  std::size_t decoded_length = std::ssize(encoded); // Maximum possible decoded length
  std::vector<unsigned char> decoded_data(decoded_length);  
  if (sodium_base642bin(decoded_data.data(), decoded_length,
			encoded.data(), std::ssize(encoded),
			nullptr, &decoded_length, nullptr, sodium_base64_VARIANT_ORIGINAL) != 0)
    throw std::runtime_error("sodium_base642bin failed");
  decoded_data.resize(decoded_length);
  return decoded_data;
}

std::string
CryptoSodium::hashMessage(std::string_view message) {
  unsigned char MESSAGE[crypto_generichash_BYTES] = {};
  std::copy(message.cbegin(), message.cend(), MESSAGE);
  unsigned char hash[crypto_generichash_BYTES] = {};
  unsigned char key[crypto_generichash_KEYBYTES] = {};
  randombytes_buf(key, std::ssize(key));
  crypto_generichash(hash, crypto_generichash_BYTES,
		     MESSAGE, crypto_generichash_BYTES,
		     key, std::ssize(key));
  return { std::cbegin(hash), std::cend(hash) };
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

void CryptoSodium::eraseUsedData() {
  std::string().swap(_msgHash);
  std::string().swap(_signatureWithPubKeySign);
}

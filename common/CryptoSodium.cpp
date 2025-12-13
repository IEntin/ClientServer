/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoSodium.h"

#include <cstring>
#include <stdexcept>

#include "DebugLog.h"
#include "Utility.h"

HandleKey::HandleKey() :
  _size(crypto_kx_SESSIONKEYBYTES),
  _obfuscated(false) {}

HandleKey::~HandleKey() {
  sodium_memzero(&*_obfuscator.begin(), _size);
}

void HandleKey::hideKey(std::array<unsigned char, crypto_kx_SESSIONKEYBYTES>& key) {
  if (!_obfuscated) {
    // generate obfuscator
    randombytes_buf(&_obfuscator[0], _size);
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
    sodium_memzero(&_obfuscator[0], _size);
  }
}

const std::string CryptoSodium::_name("CryptoSodium");

void CryptoSodium::setAESKey(SessionKey& key) {
  std::lock_guard lock(_mutex);
  _keyHandler.recoverKey(_key);
  key = _key;
  _keyHandler.hideKey(_key);
}

// client
CryptoSodium::CryptoSodium() :
  _msgHash(hashMessage(buildDateTime)) {
  crypto_kx_keypair(&_pubKeyAes[0], &_privKeyAes[0]);
  _encodedPubKeyAes = base64_encode(_pubKeyAes);
  crypto_sign_keypair(&_publicKeySign[0], &_secretKeySign[0]);
  crypto_sign_detached(&_signature[0],
		       NULL,
		       std::bit_cast<const unsigned char *>(&_msgHash[0]),
		       _msgHash.size(),
		       &_secretKeySign[0]);
  
  _signatureWithPubKeySign.assign(_signature.cbegin(), _signature.cend());
  _signatureWithPubKeySign.insert(_signatureWithPubKeySign.cend(),
				  _publicKeySign.cbegin(),
				  _publicKeySign.cend());
}

// session
CryptoSodium::CryptoSodium(std::string_view encodedPeerAesPubKey,
			   std::string_view signatureWithPubKeySign) :
  _msgHash(hashMessage(buildDateTime)) {
  std::vector<unsigned char> peerAesPubKeyV = base64_decode(encodedPeerAesPubKey);
  std::copy(peerAesPubKeyV.cbegin(), peerAesPubKeyV.cend(), _peerPubKeyAes.begin());
  crypto_kx_keypair(&_pubKeyAes[0], &_privKeyAes[0]);
  _encodedPubKeyAes = base64_encode(_pubKeyAes);
  unsigned char signature[crypto_sign_BYTES] = {};
  std::copy(signatureWithPubKeySign.begin(), signatureWithPubKeySign.begin() + crypto_sign_BYTES, signature);
  unsigned char peerPubcicKeySign[crypto_sign_PUBLICKEYBYTES] = {};
  std::copy(signatureWithPubKeySign.begin() + crypto_sign_BYTES, signatureWithPubKeySign.cend(), std::begin(peerPubcicKeySign));
  _verified = crypto_sign_verify_detached(
    signature,
    std::bit_cast<const unsigned char*>(&_msgHash[0]),
    std::ssize(_msgHash),
    peerPubcicKeySign) == 0;
  if (!_verified)
    throw std::runtime_error("authentication failed");
  // Server-side key exchange
  if (crypto_kx_server_session_keys(&_key[0],
				    nullptr,
				    &_pubKeyAes[0],
				    &_privKeyAes[0],
				    &_peerPubKeyAes[0]) != 0)
    throw std::runtime_error("Server-side key exchange failed");
  sodium_memzero(&_privKeyAes[0], _privKeyAes.size());
  DebugLog::logBinaryData(BOOST_CURRENT_LOCATION, "_key", _key);
  _keyHandler.hideKey(_key);
  eraseUsedData();
}

CryptoSodium::~CryptoSodium() {
  sodium_memzero(&_key[0], _key.size());
}

std::string_view CryptoSodium::encrypt(std::string& buffer,
				       const HEADER& header,
				       std::string_view data) {
  if (!checkAccess())
    throw std::runtime_error("access denied");
  buffer.clear();
  std::string input(serialize(header));
  input += data;
  unsigned long long ciphertext_len;
  unsigned char nonce[crypto_aead_aes256gcm_NPUBBYTES] = {};
  randombytes_buf(nonce, std::ssize(nonce));
  std::size_t message_len = std::ssize(input);
  buffer.resize(message_len + crypto_aead_aes256gcm_ABYTES);
  std::array<unsigned char, crypto_kx_SESSIONKEYBYTES> key;
  setAESKey(key);
  if (!(crypto_aead_aes256gcm_encrypt(std::bit_cast<unsigned char*>(&buffer[0]),
				      &ciphertext_len,
				      std::bit_cast<unsigned char*>(&input[0]),
				      message_len, nullptr, 0,
				      nullptr, nonce, &key[0]) == 0))
    throw std::runtime_error("encrypt failed");
  buffer.insert(buffer.cend(), std::cbegin(nonce), std::cend(nonce));
  return buffer;
}

void CryptoSodium::hideKey() {
  _keyHandler.hideKey(_key);
}

void CryptoSodium::decrypt(std::string& buffer, std::string& data) {
  if (!checkAccess())
    throw std::runtime_error("access denied");
  buffer.clear();
  if (isEncrypted(data)) {
    unsigned long long ciphertext_len = std::ssize(data) - crypto_aead_aes256gcm_NPUBBYTES;
    unsigned char recoveredNonce[crypto_aead_aes256gcm_NPUBBYTES] = {};
    std::copy(data.cend() - crypto_aead_aes256gcm_NPUBBYTES, data.cend(), recoveredNonce);
    data.resize(data.size() - crypto_aead_aes256gcm_NPUBBYTES);
    buffer.resize(ciphertext_len);
    unsigned long long decrypted_len;
    std::array<unsigned char, crypto_kx_SESSIONKEYBYTES> key;
    setAESKey(key);
    bool success = crypto_aead_aes256gcm_decrypt(std::bit_cast<unsigned char*>(&buffer[0]),
						 &decrypted_len,
						 nullptr,
						 std::bit_cast<unsigned char*>(&data[0]),
						 ciphertext_len,
						 nullptr, 0,
						 recoveredNonce, &key[0]) == 0;
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
				    &_key[0],
				    &_pubKeyAes[0],
				    &_privKeyAes[0],
				    &_peerPubKeyAes[0]) != 0)
    throw std::runtime_error("Client-side key exchange failed");
  sodium_memzero(&_privKeyAes[0], _privKeyAes.size());
  DebugLog::logBinaryData(BOOST_CURRENT_LOCATION, "_key", _key);
  hideKey();
  eraseUsedData();
  return true;
}

std::string CryptoSodium::base64_encode(std::span<unsigned char> input) {
  size_t encoded_length = sodium_base64_ENCODED_LEN(std::ssize(input), sodium_base64_VARIANT_ORIGINAL);
  std::string encoded_string(encoded_length, '\0');
  if (sodium_bin2base64(&encoded_string[0], encoded_length, &input[0], std::ssize(input),
			sodium_base64_VARIANT_ORIGINAL) == nullptr)
    throw std::runtime_error("sodium_bin2base64 failed");
  encoded_string.resize(strlen(&encoded_string[0]));
  return encoded_string;
}

std::vector<unsigned char> CryptoSodium::base64_decode(std::string_view encoded) {
  std::size_t decoded_length = std::ssize(encoded); // Maximum possible decoded length
  std::vector<unsigned char> decoded_data(decoded_length);  
  if (sodium_base642bin(&decoded_data[0], decoded_length,
			&encoded[0], std::ssize(encoded),
			nullptr, &decoded_length, nullptr, sodium_base64_VARIANT_ORIGINAL) != 0)
    throw std::runtime_error("sodium_base642bin failed");
  decoded_data.resize(decoded_length);
  return decoded_data;
}

std::string CryptoSodium::hashMessage(std::string_view message) {
  unsigned char MESSAGE[crypto_generichash_BYTES] = {};
  std::copy(message.cbegin(), message.cend(), MESSAGE);
  unsigned char hash[crypto_generichash_BYTES] = {};
  crypto_generichash(hash, crypto_generichash_BYTES,
		     MESSAGE, crypto_generichash_BYTES,
		     NULL, 0);
  return { std::cbegin(hash), std::cend(hash) };
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
  sodium_memzero(&_msgHash[0], _msgHash.size());
  sodium_memzero(&_signatureWithPubKeySign[0], _signatureWithPubKeySign.size());
}

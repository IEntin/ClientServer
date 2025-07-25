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

// client
CryptoSodium::CryptoSodium(std::u8string_view msg) :
  _msgHash(hashMessage(msg)),
  _signatureWithPubKeySign(crypto_sign_BYTES + crypto_sign_PUBLICKEYBYTES) {
  crypto_kx_keypair(_publicKeyAes.data(), _secretKeyAes.data());
  DebugLog::logBinaryData(BOOST_CURRENT_LOCATION, "_publicKeyAes", _publicKeyAes);
  _serializedPubKey = base64_encode(_publicKeyAes);
  crypto_sign_keypair(_publicKeySign.data(), _secretKeySign.data());
  crypto_sign_detached(_signature.data(), nullptr,
		       static_cast<unsigned char*>(static_cast<void*>(_msgHash.data())),
		       std::ssize(_msgHash), _secretKeySign.data());
  
  std::copy(_signature.cbegin(), _signature.cend(), _signatureWithPubKeySign.begin());
  std::copy(_publicKeySign.cbegin(), _publicKeySign.cend(),
	    _signatureWithPubKeySign.begin() + std::ssize(_signature));
}
// server
CryptoSodium::CryptoSodium(std::span<unsigned char> msgHash,
			   std::span<unsigned char> serializedPubKeyAesClient,
			   std::span<unsigned char> signatureWithPubKey) :
  _serializedPeerPubKeyAes(static_cast<const char*>(static_cast<void*>(serializedPubKeyAesClient.data())), serializedPubKeyAesClient.size()),
  _msgHash(static_cast<const char*>(static_cast<void*>(msgHash.data())), msgHash.size())
{
  auto pubKeyAesClient = CryptoSodium::base64_decode(_serializedPeerPubKeyAes);
  DebugLog::logBinaryData(BOOST_CURRENT_LOCATION, "pubKeyAesClient", pubKeyAesClient);
  crypto_kx_keypair(_publicKeyAes.data(), _secretKeyAes.data());
  _serializedPubKey = base64_encode(_publicKeyAes);
  DebugLog::logBinaryData(BOOST_CURRENT_LOCATION, "_publicKeyAes", _publicKeyAes);
  std::span<unsigned char>
    signature(signatureWithPubKey.data(), crypto_sign_BYTES);
  std::span<unsigned char>
    peerPubcicKeySign(signatureWithPubKey.data() + crypto_sign_BYTES, crypto_sign_PUBLICKEYBYTES);
  _verified = crypto_sign_verify_detached(
    signature.data(), static_cast<unsigned char*>(static_cast<void*>(msgHash.data())),
    std::ssize(msgHash),
    peerPubcicKeySign.data()) == 0;
  if (!_verified)
    throw std::runtime_error("authentication failed");
  // Server-side key exchange
  unsigned char server_rx[crypto_kx_SESSIONKEYBYTES];
  unsigned char server_tx[crypto_kx_SESSIONKEYBYTES];
  
  if (crypto_kx_server_session_keys(server_rx, server_tx, _publicKeyAes.data(), _secretKeyAes.data(), pubKeyAesClient.data()) != 0)
    throw std::runtime_error("Server-side key exchange failed");
  DebugLog::logBinaryData(BOOST_CURRENT_LOCATION, "server_rx", server_rx);
  DebugLog::logBinaryData(BOOST_CURRENT_LOCATION, "server_tx", server_tx);
  std::copy(std::cbegin(server_tx), std::cend(server_tx), _key.begin());
  DebugLog::logBinaryData(BOOST_CURRENT_LOCATION, "_key", _key);
  _keyHandler.hideKey(_key);
  if (ServerOptions::_showKey)
    showKey();
  eraseUsedData();
}

CryptoSodiumPtr CryptoSodium::createSodiumServer() {
  std::vector<unsigned char> msgHash(_msgHash.size());
  std::memcpy(msgHash.data(), _msgHash.data(), _msgHash.size());
  std::vector<unsigned char> serializedPubKey(_serializedPubKey.size());
  std::memcpy(serializedPubKey.data(), _serializedPubKey.data(), _serializedPubKey.size());
  return std::make_shared<CryptoSodium>(msgHash, serializedPubKey, _signatureWithPubKeySign);
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
  std::array<unsigned char, crypto_aead_aes256gcm_NPUBBYTES> nonce;
  randombytes_buf(nonce.data(), std::ssize(nonce));
  DebugLog::logBinaryData(BOOST_CURRENT_LOCATION, "nonce", nonce);
  std::size_t message_len = std::ssize(input);
  buffer.resize(message_len + crypto_aead_aes256gcm_ABYTES);
  std::array<unsigned char, crypto_kx_SESSIONKEYBYTES> key;
  setAESKey(key);
  if (!(crypto_aead_aes256gcm_encrypt(static_cast<unsigned char*>(static_cast<void*>(buffer.data())),
				      &ciphertext_len,
				      static_cast<unsigned char*>(static_cast<void*>(input.data())),
				      message_len, nullptr, 0,
				      nullptr, nonce.data(), key.data()) == 0))
    throw std::runtime_error("encrypt failed");
  buffer.insert(buffer.end(), nonce.data(), nonce.data() + crypto_aead_aes256gcm_NPUBBYTES);
  return buffer;
}

void CryptoSodium::decrypt(std::string& buffer, std::string& data) {
  if (!checkAccess())
    throw std::runtime_error("access denied");
  buffer.clear();
  if (utility::isEncrypted(data)) {
    unsigned long long ciphertext_len = std::ssize(data) - crypto_aead_aes256gcm_NPUBBYTES;
    std::array<unsigned char, crypto_aead_aes256gcm_NPUBBYTES> recoveredNonce;
    std::copy(data.end() - crypto_aead_aes256gcm_NPUBBYTES, data.end(), recoveredNonce.data());
    DebugLog::logBinaryData(BOOST_CURRENT_LOCATION, "recoveredNonce", recoveredNonce);
    data.erase(data.end() - crypto_aead_aes256gcm_NPUBBYTES);
    buffer.resize(ciphertext_len);
    unsigned long long decrypted_len;
    std::array<unsigned char, crypto_kx_SESSIONKEYBYTES> key;
    setAESKey(key);
    bool success = crypto_aead_aes256gcm_decrypt(static_cast<unsigned char*>(static_cast<void*>(buffer.data())),
						 &decrypted_len,
						 nullptr,
						 static_cast<unsigned char*>(static_cast<void*>(data.data())),
						 ciphertext_len,
						 nullptr, 0,
						 recoveredNonce.data(), key.data()) == 0;
    if (!success)
      throw std::runtime_error("decrypt failed");
    buffer.resize(decrypted_len);
    data = buffer;
    if (ClientOptions::_showKey)
      showKey();
  }
}

bool CryptoSodium::clientKeyExchange(std::span<unsigned char> pubKeyAesServer) {
  DebugLog::logBinaryData(BOOST_CURRENT_LOCATION, "pubKeyAesServer", pubKeyAesServer);
  unsigned char client_rx[crypto_kx_SESSIONKEYBYTES];
  unsigned char client_tx[crypto_kx_SESSIONKEYBYTES];
  if (crypto_kx_client_session_keys(client_rx, client_tx, _publicKeyAes.data(), _secretKeyAes.data(), pubKeyAesServer.data()) != 0)
    throw std::runtime_error("Client-side key exchange failed");
  DebugLog::logBinaryData(BOOST_CURRENT_LOCATION, "client_rx", client_rx);
  DebugLog::logBinaryData(BOOST_CURRENT_LOCATION, "client_tx", client_tx);
  std::copy(std::cbegin(client_rx), std::cend(client_rx), _key.begin());
  DebugLog::logBinaryData(BOOST_CURRENT_LOCATION, "_key", _key);
  _keyHandler.hideKey(_key);
  eraseUsedData();
  return true;
}

std::string CryptoSodium::base64_encode(std::span<unsigned char> input) {
  size_t encoded_length = sodium_base64_ENCODED_LEN(std::ssize(input), sodium_base64_VARIANT_ORIGINAL);
  std::string encoded_string(encoded_length, '\0');
  if (sodium_bin2base64(encoded_string.data(), encoded_length, input.data(), std::ssize(input),
			sodium_base64_VARIANT_ORIGINAL) == nullptr)
    throw std::runtime_error("sodium_bin2base64 failed");
  //The sodium_bin2base64 function might add a null terminator.
  if (encoded_string.back() == '\0')
    encoded_string.pop_back();
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
CryptoSodium::hashMessage(std::u8string_view message) {
  unsigned char MESSAGE[crypto_generichash_BYTES];
  std::copy(message.cbegin(), message.cend(), MESSAGE);
  std::vector<unsigned char> hash(crypto_generichash_BYTES);
  unsigned char key[crypto_generichash_KEYBYTES];
  randombytes_buf(key, std::ssize(key));
  crypto_generichash(hash.data(), crypto_generichash_BYTES,
		     MESSAGE, crypto_generichash_BYTES,
		     key, std::ssize(key));
  return { static_cast<const char*>(static_cast<void*>(hash.data())), hash.size() };
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
  std::array<unsigned char, crypto_kx_SECRETKEYBYTES>().swap(_secretKeyAes);
}

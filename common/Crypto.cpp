/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Crypto.h"

#include <filesystem>

#include <boost/algorithm/hex.hpp>
#include <cryptopp/files.h>
#include <cryptopp/filters.h>
#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>

#include "ClientOptions.h"
#include "Logger.h"
#include "ServerOptions.h"
#include "Utility.h"

unsigned CryptoKey::_cryptoKeySize;
CryptoPP::SecByteBlock CryptoKey::_key;
bool CryptoKey::_valid;

void CryptoKey::showKey() {
  Logger logger(LOG_LEVEL::ALWAYS, std::clog, false);
  logger << "KEY SIZE: " << _key.size() << '\n' << "KEY: ";
  boost::algorithm::hex(_key, std::ostream_iterator<char> { logger.getStream(), "" });
  logger << '\n';
}

bool CryptoKey::recover() {
  if (!std::filesystem::exists(CRYPTO_KEY_FILE_NAME)) {
    LogError << "\n\nfile " << '\'' << CRYPTO_KEY_FILE_NAME << '\'' << " not found\n"
	     << "run './copyCryptoKey.sh <number clients>' << on server site\n\n";
    return false;
  }
  std::string keyStrRecovered;
  utility::readFile(CRYPTO_KEY_FILE_NAME, keyStrRecovered);
  _key = { reinterpret_cast<const unsigned char*>(keyStrRecovered.data()), keyStrRecovered.size() };
  if (_key.empty())
    throw std::runtime_error("empty key");
  _valid = true;
  return _valid;
}

bool CryptoKey::initialize() {
  try {
    _cryptoKeySize = ServerOptions::_cryptoKeySize;
    _key.resize(_cryptoKeySize);
    CryptoPP::AutoSeededRandomPool prng;
    prng.GenerateBlock(_key, _key.size());
    if (keepKey()) {
      _valid = recover();
      return _valid;
    }
    else {
      std::ofstream stream;
      stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
      stream.open(CRYPTO_KEY_FILE_NAME, std::ios::binary);
      std::filesystem::permissions(
	CRYPTO_KEY_FILE_NAME,
	std::filesystem::perms::owner_read |
	std::filesystem::perms::owner_write |
	std::filesystem::perms::group_read,
	std::filesystem::perm_options::replace);
      if (stream) {
	stream.write(reinterpret_cast<const char*>(_key.data()), _key.size());
	_valid = true;
      }
    }
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
    return false;
  }
  return _valid;
}

bool CryptoKey::keepKey() {
  try {
    if (ServerOptions::_invalidateKey)
      return false;
    if (!std::filesystem::exists(CRYPTO_KEY_FILE_NAME))
      return false;
    if (std::filesystem::file_size(CRYPTO_KEY_FILE_NAME) == ServerOptions::_cryptoKeySize)
      return true;
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
    return false;
  }
  return false;
}

// class Crypto

std::string_view Crypto::encrypt(std::string_view data) {
  const auto& key = CryptoKey::_key;
  CryptoPP::AutoSeededRandomPool prng;
  CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);
  prng.GenerateBlock(iv, iv.size());
  bool showKey = false;
  if (ServerOptions::_parsed)
    showKey = ServerOptions::_showKey;
  else if (ClientOptions::_parsed)
    showKey = ClientOptions::_showKey;
  if (showKey)
    static bool showOnce [[maybe_unused]] = showIv(iv);
  CryptoPP::AES::Encryption aesEncryption(key.data(), key.size());
  CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv.data());
  static thread_local std::string cipher;
  cipher.erase(cipher.begin(), cipher.end());
  //LogAlways << "\t### " << cipher.capacity() << '\n';
  CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(cipher));
  stfEncryptor.Put(reinterpret_cast<const unsigned char*>(data.data()), data.size());
  stfEncryptor.MessageEnd();
  cipher.insert(cipher.cend(), iv.begin(), iv.end());
  return cipher;
}

std::string_view Crypto::decrypt(std::string_view data) {
  const auto& key = CryptoKey::_key;
  CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);
  auto beg = reinterpret_cast<const unsigned char*>(data.data()) + data.size() - iv.size();
  std::copy(beg, beg + iv.size(), iv.data());
  static thread_local std::string decrypted;
  decrypted.erase(decrypted.begin(), decrypted.end());
  //LogAlways << "\t### " << decrypted.capacity() << '\n';
  try {
    CryptoPP::AES::Decryption aesDecryption(key.data(), key.size());
    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv.data());
    CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decrypted));
    stfDecryptor.Put(reinterpret_cast<const unsigned char*>(data.data()), data.size() - iv.size());
    stfDecryptor.MessageEnd();
  }
  catch (const std::exception& e) {
    std::string error(e.what());
    error.append("\n\n\tMake sure crypto key file on client site is current!\n").
      append("\trun 'copyCryptoKey.sh <number clients>' on server site.\n");
    throw std::runtime_error(error);
  }
  return decrypted;
}

bool Crypto::showIv(const CryptoPP::SecByteBlock& iv) {
  Logger logger(LOG_LEVEL::ALWAYS, std::clog, false);
  logger << "IV : ";
  boost::algorithm::hex(iv, std::ostream_iterator<char> { logger.getStream(), "" });
  logger << '\n';
  return true;
}

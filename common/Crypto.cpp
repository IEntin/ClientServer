/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Crypto.h"

#include <boost/algorithm/hex.hpp>
#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>

#include "Logger.h"

void Crypto::showKey(const CryptoPP::SecByteBlock& key) {
  Logger logger(LOG_LEVEL::INFO, std::clog, false);
  logger << "KEY SIZE: " << key.size() << '\n' << "KEY: ";
  boost::algorithm::hex(key, std::ostream_iterator<char> { logger.getStream(), "" });
  logger << '\n';
}

std::string_view Crypto::encrypt(const CryptoPP::SecByteBlock& key,
				 std::string_view data) {
  CryptoPP::AutoSeededRandomPool prng;
  CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);
  prng.GenerateBlock(iv, iv.size());
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

std::string_view Crypto::decrypt(const CryptoPP::SecByteBlock& key,
				 std::string_view data) {
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
    throw std::runtime_error(e.what());
  }
  return decrypted;
}

bool Crypto::showIv(const CryptoPP::SecByteBlock& iv) {
  Logger logger(LOG_LEVEL::INFO, std::clog, false);
  logger << "IV : ";
  boost::algorithm::hex(iv, std::ostream_iterator<char> { logger.getStream(), "" });
  logger << '\n';
  return true;
}

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Crypto.h"

#include <boost/algorithm/hex.hpp>
#include <cryptopp/modes.h>

#include "ClientOptions.h"
#include "Logger.h"
#include "ServerOptions.h"

CryptoPP::OID Crypto::_curve = CryptoPP::ASN1::secp256r1();
CryptoPP::AutoSeededX917RNG<CryptoPP::AES> Crypto::_rng;

void Crypto::showKeyIv(const CryptoPP::SecByteBlock& key,
		       const CryptoPP::SecByteBlock& iv) {
  if (LOG_LEVEL::INFO >= Logger::_threshold) {
    Logger logger(LOG_LEVEL::INFO, std::clog, false);
    logger << "KEY SIZE: " << key.size() << '\n' << "KEY: 0x";
    boost::algorithm::hex(key, std::ostream_iterator<char> { logger.getStream() });
    logger << '\n' << "IV : 0x";
    boost::algorithm::hex(iv, std::ostream_iterator<char> { logger.getStream() });
    logger << '\n';
  }
}

std::string_view Crypto::encrypt(const CryptoPP::SecByteBlock& key,
				 std::string_view data) {
  CryptoPP::AutoSeededRandomPool prng;
  CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);
  prng.GenerateBlock(iv, iv.size());
  CryptoPP::AES::Encryption aesEncryption(key.data(), key.size());
  CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv.data());
  static thread_local std::string cipher;
  cipher.erase(0);
  //LogAlways << "\t### " << cipher.capacity() << '\n';
  CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(cipher));
  stfEncryptor.Put(reinterpret_cast<const CryptoPP::byte*>(data.data()), data.size());
  stfEncryptor.MessageEnd();
  cipher.insert(cipher.cend(), iv.begin(), iv.end());
  if (ClientOptions::_showKey)
    showKeyIv(key, iv);
  return cipher;
}

std::string_view Crypto::decrypt(const CryptoPP::SecByteBlock& key,
				 std::string_view data) {
  CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);
  std::copy(data.cend() - iv.size(), data.cend(), iv.data());
  static thread_local std::string decrypted;
  decrypted.erase(0);
  //LogAlways << "\t### " << decrypted.capacity() << '\n';
  CryptoPP::AES::Decryption aesDecryption(key.data(), key.size());
  CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv.data());
  CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decrypted));
  stfDecryptor.Put(reinterpret_cast<const CryptoPP::byte*>(data.data()), data.size() - iv.size());
  stfDecryptor.MessageEnd();
  if (ServerOptions::_showKey)
    showKeyIv(key, iv);
  return decrypted;
}

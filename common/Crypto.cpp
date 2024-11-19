/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Crypto.h"

#include <boost/algorithm/hex.hpp>

#include "ClientOptions.h"
#include "Logger.h"
#include "ServerOptions.h"

const CryptoPP::OID Crypto::_curve = CryptoPP::ASN1::secp256r1();

Crypto::Crypto(const CryptoPP::SecByteBlock& pubB) :
  _dh(_curve),
  _privKey(_dh.PrivateKeyLength()),
  _pubKey(_dh.PublicKeyLength()),
  _key(_dh.AgreedValueLength()) {
  generateKeyPair(_dh, _privKey, _pubKey);
  bool rtnA = _dh.Agree(_key, _privKey, pubB);
  if(!rtnA)
    throw std::runtime_error("Failed to reach shared secret (A)");
}

Crypto::Crypto() :
  _dh(_curve),
  _privKey(_dh.PrivateKeyLength()),
  _pubKey(_dh.PublicKeyLength()),
  _key(_dh.AgreedValueLength()) {
  generateKeyPair(_dh, _privKey, _pubKey);
}

void Crypto::showKeyIv(const CryptoPP::SecByteBlock& iv) {
  if (LOG_LEVEL::INFO >= Logger::_threshold) {
    Logger logger(LOG_LEVEL::INFO, std::clog, false);
    logger << "KEY SIZE: " << _key.size() << '\n' << "KEY: 0x";
    boost::algorithm::hex(_key, std::ostream_iterator<char> { logger.getStream() });
    logger << '\n' << "IV : 0x";
    boost::algorithm::hex(iv, std::ostream_iterator<char> { logger.getStream() });
    logger << '\n';
  }
}

std::string_view Crypto::encrypt(bool encrypt, std::string_view data) {
  static std::string cipher;
  cipher.erase(0);
  //LogAlways << "\t### " << cipher.capacity() << '\n';
  if (!encrypt) {
    cipher.insert(cipher.cend(), data.cbegin(), data.cend());
    cipher.insert(cipher.cend(), endTag.begin(), endTag.end());
    return cipher;
  }
  CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);
  {
    _rng.GenerateBlock(iv, iv.size());
  }
  CryptoPP::AES::Encryption aesEncryption(_key.data(), _key.size());
  CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv.data());
  CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(cipher));
  stfEncryptor.Put(reinterpret_cast<const CryptoPP::byte*>(data.data()), data.size());
  stfEncryptor.MessageEnd();
  cipher.insert(cipher.cend(), iv.begin(), iv.end());
  if (ClientOptions::_showKey)
    showKeyIv(iv);
  return cipher;
}

std::string_view Crypto::decrypt(std::string_view data) {
  if (data.ends_with(reinterpret_cast<const char*>(endTag.data()))) {
    data.remove_suffix(endTag.size());
    return data;
  }
  CryptoPP::SecByteBlock iv(reinterpret_cast<const CryptoPP::byte*>(data.cend() - CryptoPP::AES::BLOCKSIZE),
			    CryptoPP::AES::BLOCKSIZE);
  static thread_local std::string decrypted;
  decrypted.erase(0);
  //LogAlways << "\t### " << decrypted.capacity() << '\n';
  CryptoPP::AES::Decryption aesDecryption(_key.data(), _key.size());
  CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv.data());
  CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decrypted));
  stfDecryptor.Put(reinterpret_cast<const CryptoPP::byte*>(data.data()), data.size() - iv.size());
  stfDecryptor.MessageEnd();
  if (ServerOptions::_showKey)
    showKeyIv(iv);
  return decrypted;
}

bool Crypto::handshake(const CryptoPP::SecByteBlock& pubAreceived) {
  return _dh.Agree(_key, _privKey, pubAreceived);
}

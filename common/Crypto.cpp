/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Crypto.h"

#include <boost/algorithm/hex.hpp>

#include "ClientOptions.h"
#include "Logger.h"
#include "ServerOptions.h"

const CryptoPP::OID Crypto::_curve = CryptoPP::ASN1::secp256r1();
CryptoPP::AutoSeededX917RNG<CryptoPP::AES> Crypto::_rng;
const CryptoPP::SecByteBlock Crypto::_endTag = createEndTag();
const std::string Crypto::_endTagString(Crypto::_endTag.begin(), Crypto::_endTag.end());

CryptoPP::SecByteBlock Crypto::createEndTag() {
  static CryptoPP::SecByteBlock secBlock(CryptoPP::AES::BLOCKSIZE);
  _rng.GenerateBlock(secBlock, secBlock.size());
  return secBlock;
}

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

std::string_view Crypto::encrypt(bool encrypt,
				 const CryptoPP::SecByteBlock& key,
				 std::string_view data) {
  static std::string cipher;
  cipher.erase(0);
  //LogAlways << "\t### " << cipher.capacity() << '\n';
  if (!encrypt) {
    cipher.insert(cipher.cend(), data.cbegin(), data.cend());
    cipher.insert(cipher.cend(), _endTag.begin(), _endTag.end());
    return cipher;
  }
  CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);
  _rng.GenerateBlock(iv, iv.size());
  CryptoPP::AES::Encryption aesEncryption(key.data(), key.size());
  CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv.data());
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
  CryptoPP::SecByteBlock iv(reinterpret_cast<const CryptoPP::byte*>(data.cend() - CryptoPP::AES::BLOCKSIZE),
			    CryptoPP::AES::BLOCKSIZE);
  if (iv == _endTag) {
    data.remove_suffix(iv.size());
    return data;
  }
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

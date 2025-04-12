/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoPlPl.h"

#include <boost/algorithm/hex.hpp>

#include <cryptopp/hex.h>

#include "ClientOptions.h"
#include "IOUtility.h"
#include "ServerOptions.h"
#include "Utility.h"

const CryptoPP::OID CryptoPlPl::_curve = CryptoPP::ASN1::secp256r1();

KeyHandler::KeyHandler(unsigned size) : _size(size),
   _obfuscator(_size) {
  _rng.GenerateBlock(_obfuscator, _size);
}

void KeyHandler::hideKey(CryptoPP::SecByteBlock& key) {
  if (!_obfuscated) {
    // refresh obfuscator
    _rng.GenerateBlock(_obfuscator, _size);
    for (unsigned i = 0; i < _size; ++i)
      key[i] ^= _obfuscator[i];
    _obfuscated = true;
  }
}

void KeyHandler::recoverKey(CryptoPP::SecByteBlock& key) {
  if (_obfuscated) {
    for (unsigned i = 0; i < _size; ++i)
      key[i] ^= _obfuscator[i];
    _obfuscated = false;
  }
}

// session
CryptoPlPl::CryptoPlPl(std::string_view msgHash,
		       const CryptoPP::SecByteBlock& pubB,
		       std::string_view signatureWithPubKey) :
  _msgHash(msgHash),
  _dh(_curve),
  _privKey(_dh.PrivateKeyLength()),
  _pubKey(_dh.PublicKeyLength()),
  _key(_dh.AgreedValueLength()),
  _signatureWithPubKey(signatureWithPubKey),
  _keyHandler(_key.size()) {
  generateKeyPair(_dh, _privKey, _pubKey);
  if(!_dh.Agree(_key, _privKey, pubB))
    throw std::runtime_error("Failed to reach shared secret (A)");
  _rsaPrivKey.GenerateRandomWithKeySize(_rng, RSA_KEY_SIZE);
  _rsaPubKey.AssignFrom(_rsaPrivKey);
}

// client
CryptoPlPl::CryptoPlPl(std::string_view msgHash) :
  _msgHash(sha256_hash(msgHash)),
  _dh(_curve),
  _privKey(_dh.PrivateKeyLength()),
  _pubKey(_dh.PublicKeyLength()),
  _key(_dh.AgreedValueLength()),
  _keyHandler(_key.size()) {
  generateKeyPair(_dh, _privKey, _pubKey);
  _rsaPrivKey.GenerateRandomWithKeySize(_rng, RSA_KEY_SIZE);
  _rsaPubKey.AssignFrom(_rsaPrivKey);
  auto [success, encodedStr] = encodeRsaPublicKey(_rsaPrivKey);
  if (!success)
    throw std::runtime_error("rsa key encode failed");    
  _serializedRsaPubKey.swap(encodedStr);
}

bool CryptoPlPl::generateKeyPair(CryptoPP::ECDH<CryptoPP::ECP>::Domain& dh,
				 CryptoPP::SecByteBlock& priv,
				 CryptoPP::SecByteBlock& pub) {
  dh.GenerateKeyPair(_rng, priv, pub);
  return true;
}

void CryptoPlPl::showKey() {
  if (!checkAccess())
    return;
  Logger logger(LOG_LEVEL::INFO, std::clog, false);
  if (logger._level >= Logger::_threshold) {
    logger << "KEY: 0x";
    boost::algorithm::hex(_key, std::ostream_iterator<char> { logger.getStream() });
    logger << '\n';
  }
}

std::string_view CryptoPlPl::encrypt(std::string& buffer,
				     const HEADER& header,
				     std::string_view data) {
  if (!checkAccess())
    return data;
  buffer.clear();
  CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);
  _rng.GenerateBlock(iv, iv.size());
  CryptoPP::AES::Encryption aesEncryption;
  setAESvariable(aesEncryption);
  CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv.data());
  CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(buffer));
  char headerBuffer[HEADER_SIZE] = {};
  serialize(header, headerBuffer);
  stfEncryptor.Put(reinterpret_cast<const CryptoPP::byte*>(headerBuffer), HEADER_SIZE);
  stfEncryptor.Put(reinterpret_cast<const CryptoPP::byte*>(data.data()), data.size());
  stfEncryptor.MessageEnd();
  buffer.append(iv.begin(), iv.end());
  return buffer;
}

void CryptoPlPl::decrypt(std::string& buffer, std::string& data) {
  if (!checkAccess())
    return;
  if (utility::isEncrypted(data)) {
    buffer.clear();
    CryptoPP::SecByteBlock
      iv(reinterpret_cast<const CryptoPP::byte*>(data.data() + data.size() - CryptoPP::AES::BLOCKSIZE),
	 CryptoPP::AES::BLOCKSIZE);
    CryptoPP::AES::Decryption aesDecryption;
    setAESvariable(aesDecryption);
    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv.data());
    CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(buffer));
    stfDecryptor.Put(reinterpret_cast<const CryptoPP::byte*>(data.data()), data.size() - iv.size());
    stfDecryptor.MessageEnd();
    data.resize(buffer.size());
    std::memcpy(data.data(), buffer.data(), buffer.size());
    if (ClientOptions::_showKey)
      showKey();
  }
}

bool CryptoPlPl::handshake(const CryptoPP::SecByteBlock& pubAreceived) {
  bool result = _dh.Agree(_key, _privKey, pubAreceived);
  erasePubPrivKeys();
  hideKey();
  return result;
}

void CryptoPlPl::signMessage() {
  CryptoPP::RSASSA_PKCS1v15_SHA256_Signer signer(_rsaPrivKey);
  CryptoPP::StringSource ss(_msgHash.data(),
  true,
  new CryptoPP::SignerFilter(_rng,
      signer,
      new CryptoPP::StringSink(_signatureWithPubKey))
  );
  _signatureWithPubKey.append(_serializedRsaPubKey);
}

std::pair<bool, std::string>
CryptoPlPl::encodeRsaPublicKey(const CryptoPP::RSA::PrivateKey& privateKey) {
  std::string serialized;
  try {
    CryptoPP::StringSink sink{ serialized };
    CryptoPP::RSA::PublicKey(privateKey).DEREncode(sink);
    return { true, serialized };
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
    return { false, "" };
  }
}

bool CryptoPlPl::decodeRsaPublicKey(std::string_view serializedKey,
				    CryptoPP::RSA::PublicKey& publicKey) {
  try {
    CryptoPP::StringSource pubKeySource({ serializedKey.data(), serializedKey.size() }, true);
    publicKey.Load(pubKeySource);
    return true;
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
    return false;
  }
}

void CryptoPlPl::decodePeerRsaPublicKey(std::string_view rsaPubBserialized) {
  if (!decodeRsaPublicKey(rsaPubBserialized, _peerRsaPubKey))
    throw std::runtime_error("rsa key decode failed");
}

bool CryptoPlPl::verifySignature(std::string_view signature) {
  CryptoPP::RSASSA_PKCS1v15_SHA256_Verifier verifier(_peerRsaPubKey);
  _verified = verifier.VerifyMessage(
    reinterpret_cast<const CryptoPP::byte*>(_msgHash.data()), _msgHash.length(),
    reinterpret_cast<const CryptoPP::byte*>(signature.data()), signature.length());
  if (!_verified)
    throw std::runtime_error("Failed to verify signature");
  eraseRSAKeys();
  return true;
}
// for tesing message is based on uuid
// in real usage it may be a combination of user name and/or password
std::string CryptoPlPl::sha256_hash(std::string_view message) {
  CryptoPP::SHA256 hash;
  std::string digest;
  hash.Update(reinterpret_cast<const unsigned char*>(message.data()), message.size());
  digest.resize(hash.DigestSize());
  hash.Final((unsigned char*)&digest[0]);
  CryptoPP::HexEncoder encoder;
  std::string output;
  encoder.Attach(new CryptoPP::StringSink(output));
  encoder.Put((unsigned char*)&digest[0], digest.size());
  encoder.MessageEnd();
  return output;
}

void CryptoPlPl::eraseRSAKeys() {
  std::string().swap(_msgHash);
  _rsaPrivKey = CryptoPP::RSA::PrivateKey();
  _rsaPubKey = CryptoPP::RSA::PublicKey();
  _peerRsaPubKey = CryptoPP::RSA::PublicKey();
  std::string().swap(_serializedRsaPubKey);
  std::string().swap(_signatureWithPubKey);
}

void CryptoPlPl::erasePubPrivKeys() {
  CryptoPP::SecByteBlock().swap(_privKey);
  CryptoPP::SecByteBlock().swap(_pubKey);
}

bool CryptoPlPl::checkAccess() {
  if (utility::isServerTerminal())
    return _verified;
  else if (utility::isClientTerminal())
    return _signatureSent;
  else if (utility::isTestbinTerminal())
    return true;
  return false;
}

void CryptoPlPl::hideKey() {
  _keyHandler.hideKey(_key);
}

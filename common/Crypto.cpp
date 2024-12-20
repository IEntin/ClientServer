/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Crypto.h"

#include <boost/algorithm/hex.hpp>

#include <cryptopp/hex.h>

#include "ClientOptions.h"
#include "Logger.h"
#include "ServerOptions.h"

const CryptoPP::OID Crypto::_curve = CryptoPP::ASN1::secp256r1();
//std::string TEST_MESSAGE = Crypto::hashMessage();

// server
Crypto::Crypto(const CryptoPP::SecByteBlock& pubB) :
  _dh(_curve),
  _privKey(_dh.PrivateKeyLength()),
  _pubKey(_dh.PublicKeyLength()),
  _key(_dh.AgreedValueLength()),
  _message(Crypto::hashMessage()) {
  generateKeyPair(_dh, _privKey, _pubKey);
  if(!_dh.Agree(_key, _privKey, pubB))
    throw std::runtime_error("Failed to reach shared secret (A)");
  _rsaPrivKey.GenerateRandomWithKeySize(_rng, rsaKeySize);
  _rsaPubKey.AssignFrom(_rsaPrivKey);
}

// client
Crypto::Crypto() :
  _dh(_curve),
  _privKey(_dh.PrivateKeyLength()),
  _pubKey(_dh.PublicKeyLength()),
  _key(_dh.AgreedValueLength()),
  _message(Crypto::hashMessage()) {
  generateKeyPair(_dh, _privKey, _pubKey);
  _rsaPrivKey.GenerateRandomWithKeySize(_rng, rsaKeySize);
  _rsaPubKey.AssignFrom(_rsaPrivKey);
  auto [success, encodedStr] = encodeRsaPublicKey(_rsaPrivKey);
  if (!success)
    throw std::runtime_error("rsa key encode failed");    
  _serializedRsaPubKey.swap(encodedStr);
}

Crypto::~Crypto() {
  Trace << '\n';
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

void Crypto::encrypt(std::string& buffer, bool encrypt, std::string& data) {
  buffer.erase(buffer.cbegin(), buffer.cend());
  if (!encrypt)
    data.insert(data.cend(), endTag.cbegin(), endTag.cend());
  else {
    CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);
    _rng.GenerateBlock(iv, iv.size());
    CryptoPP::AES::Encryption aesEncryption(_key.data(), _key.size());
    CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv.data());
    CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(buffer));
    stfEncryptor.Put(reinterpret_cast<const CryptoPP::byte*>(data.data()), data.size());
    stfEncryptor.MessageEnd();
    buffer.insert(buffer.cend(), iv.begin(), iv.end());
    if (ClientOptions::_showKey)
      showKeyIv(iv);
    data.resize(buffer.size());
    std::memcpy(data.data(), buffer.data(), buffer.size());
  }
}

void Crypto::decrypt(std::string& buffer, std::string& data) {
  buffer.erase(buffer.cbegin(), buffer.cend());
  if (data.ends_with(reinterpret_cast<const char*>(endTag.data())))
    data.erase(data.size() - endTag.size());
  else {
    CryptoPP::SecByteBlock
      iv(reinterpret_cast<const CryptoPP::byte*>(data.data() + data.size() - CryptoPP::AES::BLOCKSIZE),
	 CryptoPP::AES::BLOCKSIZE);
    CryptoPP::AES::Decryption aesDecryption(_key.data(), _key.size());
    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv.data());
    CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(buffer));
    stfDecryptor.Put(reinterpret_cast<const CryptoPP::byte*>(data.data()), data.size() - iv.size());
    stfDecryptor.MessageEnd();
    if (ServerOptions::_showKey)
      showKeyIv(iv);
    data.resize(buffer.size());
    std::memcpy(data.data(), buffer.data(), buffer.size());
  }
}

bool Crypto::handshake(const CryptoPP::SecByteBlock& pubAreceived) {
  return _dh.Agree(_key, _privKey, pubAreceived);
}

void Crypto::signMessage() {
  CryptoPP::RSASSA_PKCS1v15_SHA256_Signer signer(_rsaPrivKey);
  CryptoPP::StringSource ss(_message.data(),
    true,
    new CryptoPP::SignerFilter(_rng,
      signer,
      new CryptoPP::StringSink(_signatureWithPubKey))
  );
  _signatureWithPubKey.insert(_signatureWithPubKey.cend(),
			      _serializedRsaPubKey.cbegin(),
			      _serializedRsaPubKey.cend());
}

std::pair<bool, std::string>
Crypto::encodeRsaPublicKey(const CryptoPP::RSA::PrivateKey& privateKey) {
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

bool Crypto::decodeRsaPublicKey(std::string_view serializedKey,
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

void Crypto::decodePeerRsaPublicKey(std::string_view rsaPubBserialized) {
  if (!decodeRsaPublicKey(rsaPubBserialized, _peerRsaPubKey))
    throw std::runtime_error("rsa key decode failed");
}

bool Crypto::verifySignature(const std::string& signature) {
  CryptoPP::RSASSA_PKCS1v15_SHA256_Verifier verifier(_peerRsaPubKey);
  bool verified = verifier.VerifyMessage(
    reinterpret_cast<const CryptoPP::byte*>(_message.data()), _message.length(),
    reinterpret_cast<const CryptoPP::byte*>(signature.data()), signature.length());
  if (!verified)
    throw std::runtime_error("Failed to verify signature");
  return true;
}

std::string Crypto::hashMessage() {
  const std::string message = "This message will be hashed!";
  CryptoPP::SHA256 hash;
  std::string digest;
  hash.Update(reinterpret_cast<const CryptoPP::byte*>(message.data()), message.length());
  digest.resize(hash.DigestSize());
  hash.Final(reinterpret_cast<CryptoPP::byte*>(digest.data()));
  CryptoPP::HexEncoder encoder;
  std::string output;
  encoder.Attach(new CryptoPP::StringSink(output));
  encoder.Put(reinterpret_cast<const CryptoPP::byte*>(digest.data()), digest.size());
  encoder.MessageEnd();
  return output;
}

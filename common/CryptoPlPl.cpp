/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoPlPl.h"

#include <boost/algorithm/hex.hpp>

#include <cryptopp/base64.h>
#include <cryptopp/hex.h>

#include "ClientOptions.h"
#include "CryptoDefinitions.h"
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
		       std::string_view encodedPubKeyAesClient,
		       std::string_view signatureWithPubKey) :
  _dh(_curve),
  _privKeyAes(_dh.PrivateKeyLength()),
  _pubKeyAes(_dh.PublicKeyLength()),
  _key(_dh.AgreedValueLength()),
  _keyHandler(_key.size()),
  _msgHash({ msgHash.data(), msgHash.size() }),
  _signatureWithPubKeySign(signatureWithPubKey.data(), signatureWithPubKey.size()) {
  generateKeyPair(_dh, _privKeyAes, _pubKeyAes);
  _encodedPubKeyAes = base64_encode(_pubKeyAes);
  std::vector<unsigned char> pubBDecoded = base64_decode(encodedPubKeyAesClient);
  const CryptoPP::SecByteBlock& pubB { pubBDecoded.data(), pubBDecoded.size() };
  if(!_dh.Agree(_key, _privKeyAes, pubB))
    throw std::runtime_error("Failed to reach shared secret (A)");
  _rsaPrivKey.GenerateRandomWithKeySize(_rng, RSA_KEY_SIZE);
  _rsaPubKey.AssignFrom(_rsaPrivKey);
  std::string signature(_signatureWithPubKeySign.data(), RSA_KEY_SIZE >> 3);
  std::string rsaPubKeySerialized = _signatureWithPubKeySign.substr(RSA_KEY_SIZE >> 3);
  decodePeerRsaPublicKey(rsaPubKeySerialized);
  if (!verifySignature(signature))
    throw std::runtime_error("signature verification failed.");
  hideKey();
  if (ServerOptions::_showKey)
    showKey();
  eraseRSAKeys();
}

// client
CryptoPlPl::CryptoPlPl(std::string_view msg) :
  _dh(_curve),
  _privKeyAes(_dh.PrivateKeyLength()),
  _pubKeyAes(_dh.PublicKeyLength()),
  _key(_dh.AgreedValueLength()),
  _keyHandler(_key.size()),
  _msgHash(sha256_hash(msg)) {
  generateKeyPair(_dh, _privKeyAes, _pubKeyAes);
  _encodedPubKeyAes = base64_encode(_pubKeyAes);
  _rsaPrivKey.GenerateRandomWithKeySize(_rng, RSA_KEY_SIZE);
  _rsaPubKey.AssignFrom(_rsaPrivKey);
  auto [success, encodedStr] = encodeRsaPublicKey(_rsaPrivKey);
  if (!success)
    throw std::runtime_error("rsa key encode failed");   
  _serializedRsaPubKey.swap(encodedStr);
  signMessage();
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
  setAESmodule(aesEncryption);
  CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv.data());
  CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(buffer));
  char headerBuffer[HEADER_SIZE] = {};
  serialize(header, headerBuffer);
  stfEncryptor.Put(std::bit_cast<CryptoPP::byte*>(&headerBuffer[0]), HEADER_SIZE);
  stfEncryptor.Put(std::bit_cast<CryptoPP::byte*>(data.data()), data.size());
  stfEncryptor.MessageEnd();
  buffer.append(iv.begin(), iv.end());
  return buffer;
}

void CryptoPlPl::decrypt(std::string& buffer, std::string& data) {
  if (!checkAccess())
    return;
  if (cryptodefinitions::isEncrypted(data)) {
    buffer.clear();
    CryptoPP::SecByteBlock
      iv(std::bit_cast<CryptoPP::byte*>(data.data() + data.size() - CryptoPP::AES::BLOCKSIZE),
	 CryptoPP::AES::BLOCKSIZE);
    CryptoPP::AES::Decryption aesDecryption;
    setAESmodule(aesDecryption);
    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv.data());
    CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(buffer));
    stfDecryptor.Put(std::bit_cast<CryptoPP::byte*>(data.data()), data.size() - iv.size());
    stfDecryptor.MessageEnd();
    data = buffer;
    if (ClientOptions::_showKey)
      showKey();
  }
}

bool CryptoPlPl::clientKeyExchange(std::string_view encodedPeerPubKeyAes) {
  std::vector<unsigned char> vect = base64_decode(encodedPeerPubKeyAes);
  const CryptoPP::SecByteBlock& pubKeyAesServer { vect.data(), vect.size() };
  if (!_dh.Agree(_key, _privKeyAes, pubKeyAesServer))
    throw std::runtime_error("Client-side key exchange failed");
  hideKey();
  erasePubPrivKeys();
  return true;
}

void CryptoPlPl::signMessage() {
  CryptoPP::RSASSA_PKCS1v15_SHA256_Signer signer(_rsaPrivKey);
  CryptoPP::StringSource ss(_msgHash.data(),
  true,
  new CryptoPP::SignerFilter(_rng,
      signer,
      new CryptoPP::StringSink(_signatureWithPubKeySign))
  );
  _signatureWithPubKeySign.append(_serializedRsaPubKey);
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
  _verified = verifier.VerifyMessage(std::bit_cast<CryptoPP::byte*>(_msgHash.data()), _msgHash.length(),
				     std::bit_cast<CryptoPP::byte*>(signature.data()), signature.length());
  if (!_verified)
    throw std::runtime_error("Failed to verify signature");
  eraseRSAKeys();
  return true;
}
// The message is based on uuid for testing purposes,
// in real usage it may be a combination of user name and/or password
std::string CryptoPlPl::sha256_hash(std::string_view message) {
  CryptoPP::SHA256 hash;
  std::string digest;
  hash.Update(std::bit_cast<unsigned char*>(message.data()), message.size());
  digest.resize(hash.DigestSize());
  hash.Final(std::bit_cast<unsigned char*>(digest.data()));
  CryptoPP::HexEncoder encoder;
  std::string output;
  encoder.Attach(new CryptoPP::StringSink(output));
  encoder.Put(std::bit_cast<unsigned char*>(digest.data()), digest.size());
  encoder.MessageEnd();
  return output;
}

void CryptoPlPl::eraseRSAKeys() {
  std::string().swap(_msgHash);
  _rsaPrivKey = CryptoPP::RSA::PrivateKey();
  _rsaPubKey = CryptoPP::RSA::PublicKey();
  _peerRsaPubKey = CryptoPP::RSA::PublicKey();
  std::string().swap(_serializedRsaPubKey);
  std::string().swap(_signatureWithPubKeySign);
}

void CryptoPlPl::erasePubPrivKeys() {
  CryptoPP::SecByteBlock().swap(_privKeyAes);
  CryptoPP::SecByteBlock().swap(_pubKeyAes);
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

std::string CryptoPlPl::base64_encode(std::span<unsigned char> binary) {
  std::string encoded;
  try {
    CryptoPP::StringSource ss(
      binary.data(),
      std::ssize(binary),
      true,
      new CryptoPP::Base64Encoder(new CryptoPP::StringSink(encoded)));
   return encoded;
  }
  catch (const std::exception& e) {
    throw std::runtime_error(e.what());
  }
}

std::vector<unsigned char> CryptoPlPl::base64_decode(std::string_view encoded) {
  std::vector<unsigned char> decoded;
  try {
    CryptoPP::Base64Decoder decoder;
    decoder.Attach(new CryptoPP::VectorSink(decoded));
    decoder.Put(std::bit_cast<CryptoPP::byte*>(encoded.data()), encoded.size());
    decoder.MessageEnd();
  }
  catch (const CryptoPP::Exception& e) {
    throw std::runtime_error(e.what());
  }
  return decoded;
}

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Crypto.h"

#include <boost/algorithm/hex.hpp>

#include <cryptopp/hex.h>

#include <botan/ber_dec.h>
#include <botan/der_enc.h>
#include <botan/hash.h>
#include <botan/hex.h>
#include <botan/pk_keys.h>
#include <botan/pubkey.h>

#include "ClientOptions.h"
#include "Logger.h"
#include "ServerOptions.h"

const CryptoPP::OID Crypto::_curve = CryptoPP::ASN1::secp256r1();

// server
Crypto::Crypto(const CryptoPP::SecByteBlock& pubB) :
  _dh(_curve),
  _privKey(_dh.PrivateKeyLength()),
  _pubKey(_dh.PublicKeyLength()),
  _key(_dh.AgreedValueLength()),
  _privRSAkey(_botanRng, RSA_KEY_SIZE),
  _pubRSAKey(_privRSAkey),
  _message(Crypto::hashMessage(TEST_MESSAGE)) {
  generateKeyPair(_dh, _privKey, _pubKey);
  if(!_dh.Agree(_key, _privKey, pubB))
    throw std::runtime_error("Failed to reach shared secret (A)");
}

// client
Crypto::Crypto() :
  _dh(_curve),
  _privKey(_dh.PrivateKeyLength()),
  _pubKey(_dh.PublicKeyLength()),
  _key(_dh.AgreedValueLength()),
  _privRSAkey(_botanRng, RSA_KEY_SIZE),
  _pubRSAKey(_privRSAkey),
  _message(Crypto::hashMessage(TEST_MESSAGE)) {
  generateKeyPair(_dh, _privKey, _pubKey);
  if (!encodeRsaPublicKey())
    throw std::runtime_error("rsa key encode failed");
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

std::string Crypto::hashMessage(const std::string& msg) {
  const auto hash = Botan::HashFunction::create_or_throw("SHA-256");
  hash->update(reinterpret_cast<const uint8_t*>(msg.data()), msg.length());
  return Botan::hex_encode(hash->final());
}

void Crypto::signMessage() {
  Botan::PK_Signer signer(_privRSAkey, _botanRng, "EMSA4(SHA-256)");
  signer.update(_message);
  std::vector<uint8_t> signature = signer.signature(_botanRng);
  _signatureWithPubKey.insert(_signatureWithPubKey.cend(),
			      signature.cbegin(),
			      signature.cend());
  _signatureWithPubKey.insert(_signatureWithPubKey.cend(),
			      _serializedRsaPubKey.cbegin(),
			      _serializedRsaPubKey.cend());
}

bool Crypto::encodeRsaPublicKey() {
  try {
    std::vector<uint8_t> encoded;
    Botan::DER_Encoder der(encoded);
    der.start_sequence().encode(_pubRSAKey.get_n()).encode(_pubRSAKey.get_e()).end_cons();
    _serializedRsaPubKey.insert(_serializedRsaPubKey.cend(), encoded.cbegin(), encoded.cend());
    return true;
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
    return false;
  }
}

std::pair<bool, std::vector<uint8_t>>
Crypto::encodeRsaPublicKey(Botan::RSA_PublicKey& publicKey) {
   std::vector<uint8_t> encoded;
   Botan::DER_Encoder der(encoded);
   der.start_sequence().encode(publicKey.get_n()).encode(publicKey.get_e()).end_cons();
   return { true, encoded };
}

std::unique_ptr<Botan::RSA_PublicKey>
Crypto::deserializeRsaPublicKey(std::span<const uint8_t> keyBits) {
  try {
    Botan::BigInt n;
    Botan::BigInt e;
    Botan::BER_Decoder(keyBits).start_sequence().decode(n).decode(e).end_cons();
    return std::make_unique<Botan::RSA_PublicKey>(std::move(n), std::move(e));
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
    throw std::runtime_error("Failed to decode key");
  }
}

bool Crypto::verifySignature(
     std::span<uint8_t> signature,
     const Botan::RSA_PublicKey& rsaPeerPublicKey) {
  try {
    Botan::PK_Verifier verifier(rsaPeerPublicKey, "EMSA4(SHA-256)");
    // Update the verifier with the data
    verifier.update(_message);
    // Check the signature
    if (verifier.check_signature(signature))
      return true;
    else {
      std::cout << "Signature is invalid!\n";
      return false;
    }
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return false;
  }
  return true;
}

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <cryptopp/aes.h>
#include <cryptopp/asn.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/modes.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>
#include <cryptopp/rsa.h>

constexpr std::u8string_view endTag = u8"r2345ufg5432105t";
constexpr std::size_t RSA_KEY_SIZE = 2048;

using CryptoPtr = std::shared_ptr<class Crypto>;

using CryptoWeakPtr = std::weak_ptr<class Crypto>;

struct KeyHandler {
  KeyHandler(unsigned size);
  const unsigned _size;
  CryptoPP::AutoSeededX917RNG<CryptoPP::AES> _rng;
  CryptoPP::SecByteBlock _obfuscator;
  void hideKey(CryptoPP::SecByteBlock& key);
  void recoverKey(CryptoPP::SecByteBlock& key);
  std::atomic<bool> _obfuscated = false;
};

class Crypto {
  std::string _msgHash;
  CryptoPP::AutoSeededX917RNG<CryptoPP::AES> _rng;
  CryptoPP::ECDH<CryptoPP::ECP>::Domain _dh;
  CryptoPP::SecByteBlock _privKey;
  CryptoPP::SecByteBlock _pubKey;
  CryptoPP::SecByteBlock _key;
  CryptoPP::RSA::PrivateKey _rsaPrivKey;
  CryptoPP::RSA::PublicKey _rsaPubKey;
  CryptoPP::RSA::PublicKey _peerRsaPubKey;
  std::string _serializedRsaPubKey;
  std::string _signatureWithPubKey;
  static const CryptoPP::OID _curve;
  KeyHandler _keyHandler;
  bool _verified = false;
  bool _signatureSent = false;
  std::mutex _mutex;
  bool generateKeyPair(CryptoPP::ECDH<CryptoPP::ECP>::Domain& dh,
		       CryptoPP::SecByteBlock& priv,
		       CryptoPP::SecByteBlock& pub);
public:
  Crypto(std::string_view msgHash,
	 const CryptoPP::SecByteBlock& pubB,
	 std::string_view );
  Crypto(unsigned salt);
  ~Crypto() = default;
  void showKey();
  void encrypt(std::string& buffer, bool encrypt, std::string& data);
  void decrypt(std::string& buffer, std::string& data);
  const CryptoPP::SecByteBlock& getPubKey() const { return _pubKey; }
  std::string_view getSignatureWithPubKey() const { return _signatureWithPubKey; }
  std::string getMsgHash() const { return _msgHash; }
  bool handshake(const CryptoPP::SecByteBlock& pubAreceived);
  std::pair<bool, std::string>
  encodeRsaPublicKey(const CryptoPP::RSA::PrivateKey& privateKey);
  void decodePeerRsaPublicKey(std::string_view rsaPubBserialized);
  void signMessage();
  void signatureSent() {
    _signatureSent = true;
  }
  bool verifySignature(const std::string& signature);
  bool decodeRsaPublicKey(std::string_view serializedKey,
			  CryptoPP::RSA::PublicKey& publicKey);
  std::string hashMessage(unsigned message);
  void eraseRSAKeys();
  void erasePubPrivKeys();
  bool checkAccess();
  void hideKey();
};

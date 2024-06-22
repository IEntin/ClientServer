/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "DHKeyExchange.h"

#include <sstream>

#include <cryptopp/osrng.h>
#include <cryptopp/nbtheory.h>
#include <cryptopp/secblock.h>

namespace {

// 3072 bit safe prime.
inline constexpr const char* Pstr =
"0xbfef6a7e54ba72ba09873e7ad1f3dbac8e4ae5e55534974948d67c38e07baa8036f321b6372"
"9469c02f70089426b6dcbf3b1f0c3574fc73f2509995e32b5a04e983d4363145f7ba599e760e85"
"50991d21716f9b298a636b9a929c68a5ccc9af4f3cf1a8815002026df2853efbc28e9e1d7cc75b"
"a53af6b618bed49b4d447100c4ddfd086d89f90a145ab912e7f335a595715eccb71409f3eaca38"
"6db495d7c9e054001a514c2cb15c4438ca2e573defed0aaa76a4068bc1b2953094713a48f21168"
"fbd30020fbe6e977e6d6908a25bd2983ff5e7e1237cf80b3b5f2f99dde22c63977718e073e7f2f"
"44fea12d82ff53c031874958d6f6d81d156cc78b7a20ef8d31860f0dca106feacc7a2b609dd259"
"3b1b1c1bc0d116a1ed2472f7507059f8108b0b6a3f69f3661e352c546c523ac85908e32f35155e"
"9f4f0abbb70c83e122e51a4fee3184c45ecdfd50b5da59fefefc1f0d441074ed2269dc4b274e59"
"1a8464d2c34f9c2e8cd1db308088503e9bbdf325f67b4830833ca10baae2fd1de7b57";

inline constexpr const char*  Gstr = "0x2";

inline const CryptoPP::Integer P(Pstr);
inline const CryptoPP::Integer G(Gstr);
}

std::string DHKeyExchange::step1(CryptoPP::Integer& priv, CryptoPP::Integer& pub) {
  CryptoPP::AutoSeededRandomPool prng;
  CryptoPP::Integer(prng, CryptoPP::AES::MAX_KEYLENGTH * 8).swap(priv);
  ModularExponentiation(G, priv, P).swap(pub);
  std::stringstream stream;
  stream << "0x" << std::hex << pub;
  return stream.str();
}

void DHKeyExchange::step2(const CryptoPP::Integer& priv,
			  const CryptoPP::Integer& crossPub,
			  CryptoPP::SecByteBlock& key) {
  CryptoPP::Integer keyInteger;
  ModularExponentiation(crossPub, priv, P).swap(keyInteger);
  keyInteger.Encode(key.BytePtr(), CryptoPP::AES::MAX_KEYLENGTH, CryptoPP::Integer::UNSIGNED);
}

/*
https://math.gordon.edu/ntic/ntic2020.pdf
Diffie-Hellman Key Exchange
Because they were used in the original description of the algorithm,
Diffie-Hellman key exchange is usually described assuming that Alice and
Bob want to use a symmetric cipher and so need to exchange a private key.
1 Alice and Bob agree on two numbers g and p with 0 < g < p. These
numbers are not private and can be known by anyone.
2 Alice picks a private number 0 < a and computes α = g a mod p.
Alice sends α to Bob.
3 Meanwhile, Bob picks a private number 0 < b and computes
β = g b mod p. He then sends β to Alice.
4 Alice computes k = βa mod p and Bob computes k = αb mod p.
Both of them obtain the same number k which can then be used as
the secret key.
*/

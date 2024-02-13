/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "DHKeyExchange.h"

#include <cassert>
#include <sstream>

#include <cryptopp/osrng.h>
#include <cryptopp/nbtheory.h>
#include <cryptopp/secblock.h>

#include "CommonConstants.h"

std::string DHKeyExchange::step1(CryptoPP::Integer& priv, CryptoPP::Integer& pub) {
  CryptoPP::SecByteBlock sec(KEY_LENGTH);
  CryptoPP::AutoSeededRandomPool prng;
  prng.GenerateBlock(sec, sec.size());
  priv = { sec.BytePtr(), sec.size() };
  pub = ModularExponentiation(G, priv, P);
  std::stringstream stream;
  stream << "0x" << std::hex << pub;
  return stream.str();
}

void DHKeyExchange::step2(const CryptoPP::Integer& priv,
			  const CryptoPP::Integer& crossPub,
			  CryptoPP::SecByteBlock& key) {
  CryptoPP::Integer keyInteger = ModularExponentiation(crossPub, priv, P);
  keyInteger.Encode(key.BytePtr(), KEY_LENGTH, CryptoPP::Integer::UNSIGNED);
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

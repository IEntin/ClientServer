#include "DHKeyExchange.h"

#include <cassert>
#include <iostream>
#include <sstream>

#include <cryptopp/osrng.h>
#include <cryptopp/nbtheory.h>
#include <cryptopp/secblock.h>

#include "CommonConstants.h"
#include "ServerOptions.h"

std::string DHKeyExchange::step1(CryptoPP::Integer& priv, CryptoPP::Integer& pub) {
  CryptoPP::SecByteBlock sec;
  sec.resize(KEY_LENGTH);
  CryptoPP::AutoSeededRandomPool prng;
  prng.GenerateBlock(sec, sec.size());
  priv = { sec.BytePtr(), sec.size() };
  pub = ModularExponentiation(G, priv, P);
  std::stringstream stream;
  stream << std::hex << pub;
  return stream.str();
}

CryptoPP::SecByteBlock DHKeyExchange::step2(CryptoPP::Integer& priv, CryptoPP::Integer& crossPub) {
  CryptoPP::Integer keyInteger = ModularExponentiation(crossPub, priv, P);
  CryptoPP::SecByteBlock key;
  key.resize(KEY_LENGTH);
  keyInteger.Encode(key.BytePtr(), KEY_LENGTH, CryptoPP::Integer::UNSIGNED);
  return key;
}

/*
The simplest and the original implementation,[2] later formalized as Finite Field Diffie-Hellman in RFC 7919,[9] of the protocol uses the multiplicative group of integers modulo p, where p is prime, and g is a primitive root modulo p. These two values are chosen in this way to ensure that the resulting shared secret can take on any value from 1 to p–1. Here is an example of the protocol, with non-secret values in blue, and secret values in red.

    Alice and Bob publicly agree to use a modulus p = 23 and base g = 5 (which is a primitive root modulo 23).
    Alice chooses a secret integer a = 4, then sends Bob A = g^a mod p
        A = 54 mod 23 = 4 (in this example both A and a have the same value 4, but this is usually not the case)
    Bob chooses a secret integer b = 3, then sends Alice B = g^b mod p
        B = 53 mod 23 = 10
    Alice computes s = B^a mod p
        s = 104 mod 23 = 18
    Bob computes s = A^b mod p
        s = 43 mod 23 = 18
    Alice and Bob now share a secret (the number 18).

Both Alice and Bob have arrived at the same values because under mod p,

    A b mod p = g a b mod p = g b a mod p = B a mod p {\displaystyle {\color {Blue}A}^{\color {Red}b}{\bmod {\color {Blue}p}}={\color {Blue}g}^{\color {Red}ab}{\bmod {\color {Blue}p}}={\color {Blue}g}^{\color {Red}ba}{\bmod {\color {Blue}p}}={\color {Blue}B}^{\color {Red}a}{\bmod {\color {Blue}p}}}

More specifically,

    ( g a mod p ) b mod p = ( g b mod p ) a mod p {\displaystyle ({\color {Blue}g}^{\color {Red}a}{\bmod {\color {Blue}p}})^{\color {Red}b}{\bmod {\color {Blue}p}}=({\color {Blue}g}^{\color {Red}b}{\bmod {\color {Blue}p}})^{\color {Red}a}{\bmod {\color {Blue}p}}}

Only a and b are kept secret. All the other values – p, g, ga mod p, and gb mod p – are sent in the clear. The strength of the scheme comes from the fact that gab mod p = gba mod p take extremely long times to compute by any known algorithm just from the knowledge of p, g, ga mod p, and gb mod p. Such a function that is easy to compute but hard to invert is called a one-way function. Once Alice and Bob compute the shared secret they can use it as an encryption key, known only to them, for sending messages across the same open communications channel.


    g, public (primitive root) base, known to Alice, Bob, and Eve. g = 5
    p, public (prime) modulus, known to Alice, Bob, and Eve. p = 23
    a, Alice's private key, known only to Alice. a = 6
    b, Bob's private key known only to Bob. b = 15
    A, Alice's public key, known to Alice, Bob, and Eve. A = ga mod p = 8
    B, Bob's public key, known to Alice, Bob, and Eve. B = gb mod p = 19
*/

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

#include <cryptopp/integer.h>

// implements Diffie-Hellman key exchange

struct DHKeyExchange {

  DHKeyExchange() = delete;
  ~DHKeyExchange() = delete;

  static std::string step1(CryptoPP::Integer& priv, CryptoPP::Integer& pub);
  static CryptoPP::SecByteBlock step2(CryptoPP::Integer& priv, CryptoPP::Integer& crossPub);

};

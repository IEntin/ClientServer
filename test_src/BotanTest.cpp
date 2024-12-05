/*
 *  Copyright (C) 2021 Ilya Entin
 */

// Replace the “AES-128/CBC/PKCS7”  with “AES-128/GCM” for authenticated encryption

#include <ranges>

#include <botan/auto_rng.h>
#include <botan/cipher_mode.h>
#include <botan/hex.h>
#include <botan/rng.h>

#include "Logger.h"
#include "TestEnvironment.h"

bool encrypt(std::string& input) {
  try {
    Botan::AutoSeeded_RNG rng;
    std::vector<uint8_t> key(32); 
    rng.randomize(key.data(), key.size());
    const auto enc = Botan::Cipher_Mode::create_or_throw("AES-256/CBC/PKCS7", Botan::Cipher_Dir::Encryption);
    enc->set_key(key);
    // generate fresh nonce (IV)
    Botan::secure_vector<uint8_t> iv = rng.random_vec(enc->default_nonce_length());
    // Copy input data to a buffer that will be encrypted
    Botan::secure_vector<uint8_t> pt(input.data(), input.data() + input.length());
    enc->start(iv);
    enc->finish(pt);
    return true;
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
    return false;
  }
}

TEST(BotanTest, encrypt) {
  // must be a copy
  std::string input = TestEnvironment::_source;
  ASSERT_TRUE(encrypt(input));
}

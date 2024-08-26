/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <filesystem>

#include "Crypto.h"
#include "Logger.h"
#include "ServerOptions.h"
#include "TestEnvironment.h"

// for i in {1..10}; do ./testbin --gtest_filter=CryptoTest*; done

TEST(CryptoTest, 1) {
  CryptoPP::SecByteBlock key(CryptoPP::AES::MAX_KEYLENGTH);
  Crypto::_rng.GenerateBlock(key, key.size());
  std::string_view data = TestEnvironment::_source;
  std::string_view cipher = Crypto::encrypt(key, data);
  std::string_view decrypted = Crypto::decrypt(key, cipher);
  ASSERT_EQ(data, decrypted);
}


/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <sys/random.h>

#include "monocypher.h"
#include "TestEnvironment.h"

TEST(Monocipher_EncryptDecrypt, 1) {
  std::string message = TestEnvironment::_source;
  const size_t msgSize = TestEnvironment::_source.size();

    uint8_t key[32]; 
    if (getrandom(&key[0], sizeof(key), 0) == -1)
      std::exit(3);
     uint8_t nonce[24];
     if (getrandom(&nonce[0], sizeof(nonce), 0) == -1)
       std::exit(7);

    std::vector<uint8_t> encrypted(msgSize);
    uint8_t mac[16]; 

    crypto_aead_lock(
        encrypted.data(),
        mac,
        key,
        nonce,
        nullptr, 0,
        reinterpret_cast<const uint8_t*>(message.data()),
        msgSize);

    std::vector<uint8_t> decrypted(msgSize);

    int status = crypto_aead_unlock(
        decrypted.data(),
        mac,
        key,
        nonce,
        nullptr, 0,
        encrypted.data(),
        msgSize);

    if (status == 0) {
        std::string decrypted_str(decrypted.begin(), decrypted.end());
	ASSERT_EQ(decrypted_str, TestEnvironment::_source);
    }
    crypto_wipe(key, sizeof(key));
}

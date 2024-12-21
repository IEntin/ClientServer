/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <botan/auto_rng.h>
#include <botan/cipher_mode.h>
#include <botan/hex.h>
#include <botan/pk_keys.h>
#include <botan/pubkey.h>
#include <botan/sym_algo.h>

#include "Logger.h"
#include "TestEnvironment.h"

bool encryptAndDecrypt(std::string& input) {
  try {
    Botan::AutoSeeded_RNG rng;
    Botan::secure_vector<uint8_t> key = rng.random_vec(32);// 256-bit key for AES-256
    // Choose a cipher mode
    std::string cipher_mode = "AES-256/CBC/PKCS7";
    auto enc = Botan::Cipher_Mode::create_or_throw(cipher_mode, Botan::Cipher_Dir::Encryption);
    enc->set_key(key);
    // Generate a random nonce (IV)
    Botan::secure_vector<uint8_t> iv = rng.random_vec(enc->default_nonce_length());
    // Encrypt the data
    Botan::secure_vector<uint8_t> cipher(input.cbegin(), input.cend());
    enc->start(iv);
    enc->finish(cipher);
    // look at cipher
    std::string cipherString(reinterpret_cast<const char*>(cipher.data()), cipher.size());
    Botan::secure_vector<uint8_t> decrypted(cipher.cbegin(), cipher.cend());
    // Create the cipher object for decryption
    auto dec = Botan::Cipher_Mode::create_or_throw(cipher_mode, Botan::Cipher_Dir::Decryption);
    dec->set_key(key);
    dec->start(iv);
    dec->finish(decrypted);
    // look at decrypted
    std::string decryptedString(reinterpret_cast<const char*>(decrypted.data()), decrypted.size());
    return decryptedString == input;
  }
  catch (const Botan::Exception& e) {
    LogError << "Error: " << e.what() << '\n';
    return false;
  }
}

TEST(BotanTest, encrypt) {
  std::string input = TestEnvironment::_source;
  ASSERT_TRUE(encryptAndDecrypt(input));
}

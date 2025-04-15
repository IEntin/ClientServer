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

// for i in {1..10}; do ./testbin --gtest_filter=BotanTest*; done

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
/*

#include <botan/auto_rng.h>
#include <botan/hex.h>
#include <botan/mac.h>

#include <assert.h>

//clang++ -std=c++2b botanAuthentication.cpp /usr/local/lib/botan/libbotan.a
//The following example computes an HMAC with a random key then verifies the tag.
namespace {

std::string compute_mac(std::string_view msg, std::span<const uint8_t> key) {
   auto hmac = Botan::MessageAuthenticationCode::create_or_throw("HMAC(SHA-256)");

   hmac->set_key(key);
   hmac->update(msg);

   return Botan::hex_encode(hmac->final());
}

}  // namespace

int main() {
   Botan::AutoSeeded_RNG rng;

   const auto key = rng.random_vec(32);  // 256 bit random key

   // "Message" != "Mussage" so tags will also not match
   std::string tag1 = compute_mac("Message", key);
   std::string tag2 = compute_mac("Mussage", key);
   assert(tag1 != tag2);

   // Recomputing with original input message results in identical tag
   std::string tag3 = compute_mac("Message", key);
   assert(tag1 == tag3);

   return 0;
}
 */

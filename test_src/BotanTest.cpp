/*
 *  Copyright (C) 2021 Ilya Entin
 */

// for i in {1..10}; do ./testbin --gtest_filter=BotanTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=AuthenticationTest*; done

#include <vector>

#include <botan/ber_dec.h>
#include <botan/cipher_mode.h>
#include <botan/der_enc.h>
#include <botan/hash.h>
#include <botan/hex.h>
#include <botan/pk_keys.h>
#include <botan/pubkey.h>

#include "Crypto.h"
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

TEST(AuthenticationTest, Botan) {
  // Generate RSA key pair
  Botan::AutoSeeded_RNG rng;
  Botan::RSA_PrivateKey privateKey(rng, RSA_KEY_SIZE);
  Botan::RSA_PublicKey publicKey(privateKey);
  Crypto crypto;
  std::string hashedMessage(crypto.hashMessage(TEST_MESSAGE));
  // Sign the message
  Botan::PK_Signer signer(privateKey, rng, "EMSA4(SHA-256)");
  std::span<const uint8_t> span(reinterpret_cast<const uint8_t*>(hashedMessage.data()), hashedMessage.size());
  std::vector<uint8_t> signature = signer.sign_message(span, rng);
  ASSERT_EQ(signature.size(), SIGNATURE_SIZE);
  // Verify signature
  Botan::PK_Verifier verifier(publicKey, "EMSA4(SHA-256)");
  bool verified = verifier.verify_message(span, signature);
  ASSERT_TRUE(verified);
  // Transfer the key and the signature
  auto [encoded, encodedRsaPublicKey] = crypto.encodeRsaPublicKey(publicKey);
  ASSERT_TRUE(encoded);
  auto receivedPublicKey = crypto.deserializeRsaPublicKey(encodedRsaPublicKey);
  Botan::PK_Verifier verifierTr(*receivedPublicKey, "EMSA4(SHA-256)");
  bool verifiedTr = verifierTr.verify_message(span, signature);
  ASSERT_TRUE(verifiedTr);
  signature.erase(signature.cbegin() + 3, signature.cbegin() + 4);
  verifiedTr = verifierTr.verify_message(span, signature);
  ASSERT_FALSE(verifiedTr);
}

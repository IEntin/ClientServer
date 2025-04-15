// clang++ -std=c++20 sodiumAuthentication.cpp /usr/lib/x86_64-linux-gnu/libsodium.a -o authentication
// sudo apt-get install libsodium-dev
// /usr/lib/x86_64-linux-gnu/libsodium.a  (libsodium.so)

#include <iostream>
#include <sodium.h>
#include <stdexcept>

int main() {
  if (sodium_init() != 0)
    throw std::runtime_error("Failed to initialize libsodium");
  unsigned char publicKey[crypto_sign_PUBLICKEYBYTES];
  unsigned char secretKey[crypto_sign_SECRETKEYBYTES];
  crypto_sign_keypair(publicKey, secretKey);
  const unsigned char message[] = "This is a test message";
  constexpr std::size_t messageLength = sizeof(message) - 1;
  unsigned char signature[crypto_sign_BYTES];
  unsigned long long signatureLength;
  if (crypto_sign_detached(signature, &signatureLength, message, messageLength, secretKey) != 0)
    throw std::runtime_error("Failed to sign the message");
  unsigned char verifiedMessage[messageLength];
  unsigned long long verifiedMessageLength;
  if (crypto_sign_verify_detached(signature, message, messageLength, publicKey) != 0)
    throw std::runtime_error("Invalid signature");
  std::cout << "Signature verified successfully.\n";
  return 0;
}

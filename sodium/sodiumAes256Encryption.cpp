
// clang++ -std=c++20 -g sodiumAes256Encryption.cpp /usr/lib/x86_64-linux-gnu/libsodium.a -o encryption

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <sodium.h>

std::vector<unsigned char> encrypt_aes256gcm(const std::vector<unsigned char>& message, const std::vector<unsigned char>& key) {
  if (key.size() != crypto_aead_aes256gcm_KEYBYTES)
    throw std::invalid_argument("Key must be 32 bytes");
  std::vector<unsigned char> nonce(crypto_aead_aes256gcm_NPUBBYTES);
  randombytes_buf(nonce.data(), nonce.size());
  std::vector<unsigned char> ciphertext(message.size() + crypto_aead_aes256gcm_ABYTES);
  unsigned long long ciphertext_len;
  if (crypto_aead_aes256gcm_encrypt(ciphertext.data(), &ciphertext_len, message.data(), message.size(),
				    nullptr, 0, nullptr, nonce.data(), key.data()) != 0)
    throw std::runtime_error("Encryption failed");
  std::vector<unsigned char> result;
  result.insert(result.end(), nonce.begin(), nonce.end());
  result.insert(result.end(), ciphertext.begin(), ciphertext.begin() + ciphertext_len);
  return result;
}
std::vector<unsigned char> decrypt_aes256gcm(const std::vector<unsigned char>& ciphertext_with_nonce, const std::vector<unsigned char>& key) {
  if (key.size() != crypto_aead_aes256gcm_KEYBYTES)
    throw std::invalid_argument("Key must be 32 bytes");
  if (ciphertext_with_nonce.size() < crypto_aead_aes256gcm_NPUBBYTES + crypto_aead_aes256gcm_ABYTES)
    throw std::invalid_argument("Ciphertext is too short");
  std::vector<unsigned char> nonce(ciphertext_with_nonce.begin(), ciphertext_with_nonce.begin() + crypto_aead_aes256gcm_NPUBBYTES);
  std::vector<unsigned char> ciphertext(ciphertext_with_nonce.begin() + crypto_aead_aes256gcm_NPUBBYTES, ciphertext_with_nonce.end());
  std::vector<unsigned char> decrypted(ciphertext.size() - crypto_aead_aes256gcm_ABYTES);
  unsigned long long decrypted_len;
  if (crypto_aead_aes256gcm_decrypt(decrypted.data(), &decrypted_len, nullptr,
				    ciphertext.data(), ciphertext.size(), nullptr, 0,
				    nonce.data(), key.data()) != 0)
    throw std::runtime_error("Decryption failed: invalid ciphertext or key");
  return decrypted;
}

int main() {
  if (sodium_init() < 0) {
    std::cerr << "Failed to initialize libsodium" << '\n';
    return 1;
  }
  std::vector<unsigned char> key(crypto_aead_aes256gcm_KEYBYTES);
  randombytes_buf(key.data(), key.size());
  std::string message_str = "This is a secret message!";
  std::vector<unsigned char> message(message_str.begin(), message_str.end());
  std::vector<unsigned char> encrypted = encrypt_aes256gcm(message, key);
  std::vector<unsigned char> decrypted = decrypt_aes256gcm(encrypted, key);
  std::cout << "Original message: " << message_str << '\n';
  std::cout << "Encrypted message (with nonce): ";
  for (unsigned char c : encrypted)
    std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
  std::cout << '\n';
  std::cout << "Decrypted message: " << std::string(decrypted.begin(), decrypted.end()) << '\n';
  return 0;
}

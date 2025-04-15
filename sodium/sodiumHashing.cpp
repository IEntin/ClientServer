// clang++ -std=c++20 sodiumHashing.cpp /usr/lib/x86_64-linux-gnu/libsodium.a -o hashing

//d29855be382b253c2f485c0b6a729b21c382bb3ad414d7ffe6cf274e1aa59d42

#include <sodium.h>
#include <iostream>
#include <string>
#include <iomanip>

int main() {
  if (sodium_init() < 0) {
    std::cerr << "Failed to initialize libsodium" << std::endl;
    return 1;
  }
  std::string message("This is a test message");
  const unsigned char* input = reinterpret_cast<const unsigned char*>(message.data());
  unsigned char hash[crypto_generichash_BYTES];
  crypto_generichash(hash, sizeof(hash), input, message.size(), nullptr, 0);
  std::u8string u8str(reinterpret_cast<const char8_t*>(&hash[0]), sizeof(hash));
  std::cout << "Hash: ";
  for (int i = 0; i < u8str.size(); ++i)
    std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(u8str[i]);
  std::cout << '\n';
  return 0;
}

// clang++ -std=c++20 sodiumHashing.cpp /usr/lib/x86_64-linux-gnu/libsodium.a -o hashing;clang++ -std=c++20 sodiumAuthentication.cpp /usr/lib/x86_64-linux-gnu/libsodium.a -o authentication;clang++ -std=c++20 sodiumAes256Encryption.cpp /usr/lib/x86_64-linux-gnu/libsodium.a -o encryption;clang++ -std=c++20 -g sodiumDHaes256.cpp /usr/lib/x86_64-linux-gnu/libsodium.a -o dhAes256
